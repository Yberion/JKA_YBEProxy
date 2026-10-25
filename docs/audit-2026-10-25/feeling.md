# The combat "feeling" problem

## Context

- The combat "feels" different as soon as a recompiled binary is used, and nobody knows why.
  - **Detected first with the engine:** a recompiled `linuxjampded` running the original `jampgame` does not feel like the retail `linuxjampded`.
  - **The same problem exists with the game module:** a recompiled `jampgame` does not feel like the retail one either.
- On original servers, the game module is always the original `jampgame`.
- That is why the proxy exists: it fixes things without replacing any original binary. Keep that approach.
- Retail Linux engine: compiled with ICC (2003), according to the author. Retail Windows binaries: probably MSVC 7.x (not verified).
- Already tried: comparing the assembly of the original and recompiled executables, analysed with Sonnet (medium effort). Nothing found.

> [!WARNING]
> The released source (`jedis/jediacademy`) and its `codemp/unix/makefile` are **not** guaranteed to match the retail binaries. Everything below that comes from the source is a lead to check against the retail binary (disassembly or measurement), not a fact.

## Why diffing the whole binary does not work

Between ICC 2003 and a modern compiler, register allocation, inlining, instruction scheduling and floating-point code generation all differ. Almost every function looks different, so the noise hides the real cause. It works better to test precise leads and to measure behaviour differences directly.

## Why the engine can change the feeling

Even with the original `jampgame`, part of what decides movement and combat runs inside the engine:

- **Velocity snapping.** `PmoveSingle` calls `trap_SnapVector( pm->ps->velocity )` on every pmove ([bg_pmove.c:11126](https://github.com/jedis/jediacademy/blob/master/codemp/game/bg_pmove.c#L11126) in the released source). That syscall reaches the engine's `Sys_SnapVector` (`sv_game.cpp:681`).
- **Collision.** Every `trap_Trace` from pmove (sliding, stepping, ground detection) goes to the engine's collision model (`qcommon/cm_trace.cpp`).
- **Ghoul2.** Bone and bolt matrices and Ghoul2 traces, which decide where the saber blade is and what it hits, are computed by the engine (`codemp/ghoul2/`, `renderer/tr_ghoul2.cpp`, both linked into the dedicated server).
- **Timing.** Server frame timing, snapshot pacing and user command processing are all engine code.
- **FPU state.** The engine and the game module run in the same process, so they share the x87 FPU control word.

## Engine leads

### E1. `Sys_SnapVector` rounding

- In the released source, the Linux dedicated server links `unix/snapvector.nasm`. It sets the FPU control word to `0x037F` (round to nearest even, extended precision), then does `fistp`/`fild` on each component.
- The Windows engine (`win32/win_shared.cpp`) does `fld`/`fistp` with the current control word, so round to nearest by default.
- A recompiled engine with any other implementation snaps the velocity differently on every pmove frame:
  - truncation such as `(int)v`;
  - `roundf`, which rounds halves away from zero;
  - a C fallback.
- **Test:** log the input and output of `G_SNAPVECTOR` in the proxy's syscall handler (`Proxy_OriginalAPI_VM_DllSyscall`) under both engines, then compare. Values ending in `.5` are the most telling.

### E2. Collision precision

- The released makefile compiles the engine with ICC 5.0 for x87 (`-O2 -unroll`, no `-mp`). Intermediate results stay in 80-bit registers, and value-changing optimisations are allowed.
- A modern build either uses SSE (32-bit float everywhere) or uses x87 with different spill points. Trace fractions and end positions can then differ in borderline cases: sliding along walls, steps, ground detection.
- **Test:**
  1. Under the retail engine, record the inputs and outputs of `G_TRACE` / `G_TRACECAPSULE` in the proxy.
  2. Under the recompiled engine, replay the same queries on the same map and compare fractions, end positions and plane normals.
  3. Traces are pure functions of the map and the inputs, so this is deterministic.

### E3. Ghoul2 precision

- Same mechanism as E2, applied to bone and bolt matrices and Ghoul2 traces, so to saber positions and hits.
- The `_M_IX86` branches in `tr_ghoul2.cpp` only handle the byte order of the model format, not the math, so they are not a lead.
- **Test:** the same record-and-compare approach on `G_G2TRACE` and on the bolt matrix syscalls. This is harder than E2, because the result depends on the animation state and the time.

### E4. FPU control word

- On Linux, glibc starts processes with the x87 control word at `0x037F` (extended precision).
- If the recompiled engine, or one of its libraries, changes the precision or rounding mode, the retail `jampgame` x87 code is affected too, since it runs in the same process.
- **Test:** in the proxy, read the x87 control word (`fnstcw`) and MXCSR at `GAME_INIT` and on a few `GAME_RUN_FRAME` calls, under both engines. This is cheap and conclusive.

### E5. Frame timing and snapshot pacing

- The released makefile does not tell how the recompiled engine was built. If it derives from ioq3 or OpenJK, the server frame loop (`Com_Frame`, `SV_Frame`), `Sys_Milliseconds`, `SV_RateMsec` and snapshot scheduling may behave differently. That changes how often and how regularly clients get snapshots.
- **Test:** in the proxy, log the `levelTime` deltas from `GAME_RUN_FRAME` and the `serverTime` of the user commands from `G_GET_USERCMD`, under both engines, then compare the distributions.

### E6. User command processing

- Differences in `SV_UserMove` / `SV_ClientThink` (duplicated commands, `serverTime` checks, clamping) change what pmove receives.
- **Test:** log the user commands the game receives through `G_GET_USERCMD` under both engines, with the same client and settings.

### Compiler flags in the released makefile

From `codemp/unix/makefile`, to confirm against the binary:

- `BASE_CFLAGS = -D_JK2 -D_M_IX86 -Kc++ -w`
- release: `-DNDEBUG -O2 -unroll`
- dedicated objects: `-DDEDICATED -DC_ONLY`, some with `-use_msasm`
- `CC` is ICC 5.0 (`/opt/intel/compiler50`), and `CXX` is `g++` for some rules
- no `-rcd`
- `-D_M_IX86`, `-DC_ONLY` and `id386` select different code paths. A modern build that does not define the same macros compiles different branches.

## Game module leads

These still apply when `jampgame` is recompiled.

### G1. Float to int conversion

- ICC has `-rcd`, and MSVC 7.x had `/QIfist`. Both make `(int)f` round to nearest instead of truncating. The released Linux makefile does not use `-rcd`, but the retail Windows `jampgamex86.dll` could have used `/QIfist`.
- The game's own `SnapVector` already differs by platform in the released source ([q_shared.h:1403-1431](https://github.com/jedis/jediacademy/blob/master/codemp/game/q_shared.h#L1403-L1431)): Linux uses a `(int)` truncation macro, Windows an inline version.
- **Test:**
  - in the retail `.so`, check whether `fistp` is preceded by an `fldcw` to truncation, or is bare;
  - in the retail `.dll`, look for `call __ftol` (truncation) versus bare `fistp` (`/QIfist`).

### G2. x87 80-bit versus SSE

- This repository's CMake, copied from OpenJK, forces `-msse2 -mfpmath=sse`, and modern compilers use SSE by default.
- That changes accumulations in pmove and borderline comparisons in saber code, such as the `DotProduct > threshold` tests in blocking.
- **Test:** rebuild with `-mfpmath=387 -mno-sse2` and check whether the feeling gets closer to retail.

### G3. Undefined behaviour in the 2003 source

- Uninitialised variables depend on what is on the stack, so on the compiler. Build `w_saber`, `bg_saber`, `bg_pmove` and `g_combat` with `-Wall -Wmaybe-uninitialized` and review every warning.
- When several `Q_irand()` / `random()` calls appear in one expression, the order of the draws is unspecified and varies between compilers.

### G4. The released source does not match retail 1.01

Compare data tables extracted from the retail binary with the source: `saberMoveData`, animation timings, `weaponData`. Comparing data is easy, unlike comparing code.

## Method: the proxy as a measurement tool

The proxy's original API path works with any engine that uses `vmMain`/`dllEntry`, including a recompiled `jediacademy` engine. Without engine patches, it still sees every syscall. That makes it a neutral probe for the engine leads:

1. Run the retail engine with the original `jampgame` and the proxy in logging mode. Record:
   - `G_SNAPVECTOR` inputs and outputs (E1);
   - `G_TRACE` / `G_TRACECAPSULE` queries and results (E2);
   - the FPU control word and MXCSR (E4);
   - frame and user command timing (E5, E6).
2. Run the recompiled engine with the same game module, map and settings.
3. Replay the recorded trace queries on it, and compare every log.
4. The first lead that shows a difference points to the cause. E1 and E4 are the cheapest to test, so start with them.

For the game module leads, use full differential replay:

1. Fix the `randomSeed` argument of `GAME_INIT`, `sv_fps` and the map.
2. Record the user commands at `GetUsercmd`, keyed by `serverTime`.
3. Replay them with the retail and the recompiled `jampgame`.
4. Dump `playerState` / `entityState` each frame and diff.

`ClientThink` runs once per user command received from the network, so the replay must be driven by the recorded `serverTime`. Not prototyped yet.

## Needed to go further

- How the recompiled `linuxjampded` was built: which source (raw jediacademy, OpenJK, other), compiler and version, flags.
- The retail binaries: `linuxjampded`, `jampgamei386.so`, and the Windows ones if relevant.
- The recompiled binaries.

## Not verified

- No disassembly was done. Every lead comes from the released source or from compiler knowledge.
- The measurement method has not been prototyped.
