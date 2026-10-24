# The combat "feeling" problem

## Context

- Recompiling jampgame from the released source changes how combat feels, and nobody knows why. That is the main reason this proxy exists: it fixes things without recompiling the game.
- Linux retail build: compiled with ICC (2003), according to the author.
- Windows retail build: probably MSVC 7.x. Not verified.
- Already tried: comparing the assembly of the original and recompiled binaries, analysed with Sonnet (medium effort). Nothing found.
- The proxy approach avoids the problem entirely, because the retail DLL runs unchanged. Keep it.

## Why diffing the whole binary does not work

Between ICC 2003 and a modern compiler, register allocation, inlining, instruction scheduling and floating-point code generation all differ. Almost every function looks different, so the noise hides the real cause. It works better to test precise hypotheses and to measure behaviour differences directly (see "Method" below).

## Hypotheses, most likely first

### H1. Float to int conversion

- ICC has `-rcd`, and MSVC 7.x had `/QIfist`. Both make `(int)f` round to nearest (current FPU rounding mode) instead of truncating as the C standard requires. Retail builds of that era often used them for speed.
- If the retail game used one of them, every `(int)` cast on a float behaves differently from a recompiled build. In the game this affects:
  - `SnapVector` (entity positions, projectile velocities, `BG_PlayerStateToEntityState(..., snap)`);
  - every computation like `(int)(dmg * x)`.
- Known precedent in Quake 3: jump height differs between QVM and native DLL builds because `SnapVector` rounds differently.
- **Test, about 10 minutes:**
  - In the retail `.so`: look at the `fistp` instructions. If they are preceded by an `fldcw` that switches to truncation, the conversion is C-conformant. Bare `fistp` means `-rcd`.
  - In the retail `.dll`: `call __ftol` means normal truncation. Bare `fistp` means `/QIfist`.

### H2. x87 80-bit versus SSE

- This repository's CMake, copied from OpenJK, forces `-msse2 -mfpmath=sse`, and modern compilers use SSE by default.
- ICC 2003 kept intermediate results in x87 80-bit registers. Its default floating-point mode was also "fast", which allows value-changing optimisations such as replacing a division with a multiplication by the reciprocal.
- This changes the accumulations in pmove and the borderline comparisons in saber code, for example the `DotProduct > threshold` tests in blocking.
- **Test:** rebuild the game with `-mfpmath=387 -mno-sse2` and check whether the feeling gets closer to retail.

### H3. Undefined behaviour in the 2003 source

- Uninitialised variables: their value depends on whatever is on the stack, so on the compiler. Build `w_saber`, `bg_saber`, `bg_pmove` and `g_combat` with `-Wall -Wmaybe-uninitialized` and review every warning.
- Unspecified evaluation order: when several `Q_irand()` / `random()` calls appear in one expression, the order of the random draws changes between compilers.

### H4. The released source does not match the retail 1.01 code

Compare data tables extracted from the retail binary with the source: `saberMoveData`, animation timings, `weaponData`. Comparing data is easy, unlike comparing code.

## Method: differential replay using the proxy

Use the proxy as a test bench instead of reading assembly:

1. Fix the inputs:
   - the `randomSeed` argument of `GAME_INIT` (the proxy can override it);
   - `sv_fps`;
   - the map.
2. Record the usercmds at the `GetUsercmd` interception point, keyed by `serverTime`.
3. Replay the recording with the retail DLL, then with the recompiled DLL.
4. On every frame, dump `playerState` / `entityState` through `g_clients` (from `LocateGameData`).
5. Diff the two dumps. The first frame and the first field that diverge point to the function at fault.

The proxy already has most of the pieces: it intercepts `GetUsercmd` and `LocateGameData`.

To validate before relying on it (not tested):

- `ClientThink` runs once per usercmd received from the network, so its timing depends on packet arrival. The replay has to be driven by the recorded `serverTime`, not by real packets.
- Ways to explore: inject the recorded usercmds inside `GetUsercmd`, use `g_synchronousClients 1`, or drive bots as puppets.

## Needed to go further

- The retail binaries: `jampgamex86.dll` and `jampgamei386.so`.
- The recompiled build that shows the different feeling.
- The exact build command (compiler, version, flags) and the source used (raw jediacademy or OpenJK).
- The platform where the difference is felt (Windows, Linux or both).

With these, the H1 test can be run directly.

## Not verified

- No disassembly was done. H1 to H4 are hypotheses.
- The replay method has not been prototyped.
