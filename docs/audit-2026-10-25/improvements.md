# Improvements

Not bugs as such: robustness, simplification, missing features and build hygiene. Paths are relative to the repository root. The verification statuses are the same as in [fixes.md](fixes.md).

## A1. Hook robustness

### Signature check

Status: confirmed, **not done**: the author considers that the `version` check is enough (a signature check, a generated table of the first bytes of every address from the retail binaries, and a stop on a difference were written, then removed). The only check before patching is the `version` cvar string ([Proxy_Main.cpp:83](../../src/JKA_YBEProxy/Proxy_Main.cpp#L83)).

Fix:

1. Next to each address in [Proxy_Engine_Wrappers.hpp](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Wrappers.hpp), store the first bytes expected at that address, per platform. Read them from the binaries once.
2. Before `Proxy_Engine_Inline_Patches` and `Proxy_Engine_Attach_Patches`, compare every hook and every inline patch address with `memcmp`.
3. On any mismatch, patch nothing, print which address failed, and run like on an unknown engine (`isOriginalEngine = false`).

### Relative instructions

Status: **applied** with the move from HDE to Zydis (git submodule `external/zydis`) and the rewrite of `HookUtils`. Built without warnings on MSVC 19.51, GCC 12 and GCC 16, and tested on the original Windows and Linux engines. The 16 hooks were also checked offline with capstone on `jampDed.exe` and `linuxjampded`: all are accepted, and only `Com_WriteConfig_f` on Windows needs the relocation of a `call`.

The problem: `GetTramp` copied the first bytes of the target as they were, so a `call`, `jmp` or `jcc` with a relative operand jumped to the wrong place from the trampoline, and `GetLen` ignored the HDE errors. Checked with capstone on the retail binaries: none of the 18 hooks at the time had a relative instruction in its copied bytes, but the hook of F7 on Windows does (`Com_WriteConfig_f` starts with `sub esp, 0x40; call Cmd_Argc`). F7 itself rewrites the whole function and never runs the trampoline.

Applied, in [HookUtils.cpp](../../src/JKA_YBEProxy/RuntimePatch/HookUtils/HookUtils.cpp):

- `Analyze` decodes the instructions with Zydis until they cover 5 bytes (x86 or x64 mode, from the size of a pointer). A decoding error, or a `ret` before 5 bytes (function too short), fails the hook.
- A relative instruction (`ZYDIS_ATTRIB_IS_RELATIVE`) other than `call`/`jmp rel32` without prefix (`e8`, `e9`) fails the hook. That covers `jcc`, short `jmp`, `loop` and, on x64, RIP-relative addressing.
- A `call`/`jmp rel32` is relocated in the trampoline: its destination is computed from the original address, then the offset is rewritten from the trampoline address.
- Every offset is checked before the engine is touched. On x86 a rel32 reaches every address; on x64 an offset that does not fit in 32 bits fails the hook (`Status::OutOfRange`).
- `HookBase` keeps the original bytes at `Attach`, and `Detach` restores from that copy, not from the trampoline (where a rel32 was relocated).

Not covered: a jump from elsewhere in the function to the middle of the overwritten bytes. It would land in the `jmp` or in the `nop` padding; only a full analysis of the function would find it.

### Partial failure

Status: **applied**.

There was never a null call: when `GetTramp` failed, `Attach` returned before writing the detour, and each `Original_X` is only called from its own `Proxy_X`. The real problem was a server running with part of the fixes missing, and one line in the console saying so (without a final `
`).

Applied: `Attach` returns a `Status` with the reason, and any failure of a hook, of the filter stub of `SV_PacketEvent`, or of an inline patch calls `Proxy_FatalHookError`: the reason is printed and the server exits (`exit(EXIT_FAILURE)`, as when the original game module cannot be loaded). `Detach` of a hook that was never attached does nothing, and a failure to unhook is printed with its reason.

### Memory protection

Status: **applied**, except the optional items below.

`UnProtect` and `ReProtect` are replaced by a `CodeWriter` object: it makes the pages of the written range writable (all the pages, not only the first and the last), puts the protection back (the original on Windows, read and execute on Linux, where the targets are always code), and flushes the instruction cache. `AllocateMemory` and `ReleaseMemory` take the same exact size (the hidden `+ 5` is gone). The jump offsets are written as `int32_t`: they used `uintptr_t`, which wrote 8 bytes on x64.

Left, optional:

- Put all trampolines in one page. Today each one is a `VirtualAlloc`, which reserves 64 KB of address space.
- Switch the trampoline memory to RX once written, instead of keeping it RWX. The filter stub of `SV_PacketEvent` is written after `Attach`, so it needs a change too.

### Struct layout

Status: confirmed, **not done**, by decision of the author (`sizeof(client_t)` is `0x51478` in both retail engines, if it is ever wanted). There is no `static_assert` on the engine structures, although the addresses of the fields are written as comments, Windows then Linux ([server.hpp:186-195](../../src/sdk/server/server.hpp#L186-L195)).

Fix: turn them into offsets. For example, `serverStatic_t` starts at `0x606218` and `clients` is at `0x606224`, so:

```cpp
static_assert(offsetof(serverStatic_t, clients) == 0x0C);
```

Add a `sizeof(client_t)` check too, per platform.

## A2. Bridge for OpenJK with the retail jampgame

Status: **corrected**. A simpler option exists.

- The facts still hold: the `vm_legacy` cvar only exists in `_DEBUG` builds of OpenJK ([`codemp/qcommon/vm.cpp:157-160`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/qcommon/vm.cpp#L157-L160)), and both halves of a bridge exist in OpenJK [`codemp/server/sv_gameapi.cpp`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/server/sv_gameapi.cpp) (the legacy branch of the `GVM_*` functions, and `SV_GameSystemCalls`).
- But the bridge is only needed because the proxy exports `GetModuleAPI`. Without that export, OpenJK loads the proxy through the legacy API, and the proxy loads the retail jampgame as it does on the original engine. See F4 option A in [fixes.md](fixes.md).

Recommended: F4 option A. Keep the bridge only if the proxy must keep its `GetModuleAPI` export:

- an export table whose functions call `originalVmMain(GAME_*, ...)`;
- a syscall dispatcher, passed to the original `dllEntry`, that turns each `G_*` syscall into a call to `gi->X`;
- the float conventions (`PASSFLOAT` / `VMF`) to respect when translating.

It is essentially `Proxy_Translate_SystemCalls` in reverse. The licence is GPL, like this project.

### SDK headers: one set is enough

Question from the author: should there be one set of headers for the original engine and another one for OpenJK? Not needed.

- [g_public.hpp](../../src/sdk/game/g_public.hpp) is OpenJK's [`codemp/game/g_public.h`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_public.h), with 22 lines changed (includes, and the `Vehicle_t` typedef). OpenJK keeps the legacy ABI identical to retail on purpose, so that retail mods still load: the 367 entries of the syscall and `GAME_*` enums are in the same order as in [`jediacademy/codemp/game/g_public.h`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_public.h).
- The engine structures ([server.hpp](../../src/sdk/server/server.hpp), [sys_public.hpp](../../src/sdk/sys/sys_public.hpp)) are only used on the original engine, and follow the retail layout: the comments give the retail addresses, and `netadr_t` keeps the retail `ipx` field.
- The only OpenJK-only parts are `gameImport_t`/`gameExport_t`, used by the `GetModuleAPI` path. With F4 option A they go away. With option B they must stay in sync with OpenJK's `g_public.h`, as they are today (`GAME_API_VERSION` 1 on both).

What would help instead: the `static_assert`s of A1 on the retail layouts (`client_t`, `serverStatic_t`, `sharedEntity_t`, `playerState_t`), so that a header change that breaks the layout fails to compile.

## A3. Simplification

Status: confirmed.

- The F1 filter in [fixes.md](fixes.md) removes about 180 lines (the rewrites of `SV_PacketEvent` and `SV_ConnectionlessPacket`, the `SVC_RemoteCommand` wrapper, and the `SVC_Status`/`SVC_Info` hooks).
- F4 option A removes the whole OpenJK API layer.
- [Proxy_CVars.cpp](../../src/JKA_YBEProxy/Proxy_CVars.cpp):
  - `Proxy_UpdateCvars` and `Proxy_OriginalEngine_UpdateCvars` have the same body, with a different table ([Proxy_CVars.cpp:79-135](../../src/JKA_YBEProxy/Proxy_CVars.cpp#L79-L135));
  - the two registration functions too, apart from two prints ([Proxy_CVars.cpp:31-71](../../src/JKA_YBEProxy/Proxy_CVars.cpp#L31-L71)).

  Fix: one `static void Proxy_RegisterCvarTable(cvarTable_t* table, std::size_t size)` and one `static void Proxy_UpdateCvarTable(cvarTable_t* table, std::size_t size)`, called with each table.

#### Applied

Status: **applied**. The F1 and F4 parts were removed with F1 and F4. The two cvar tables now go through `Proxy_RegisterCvarTable` and `Proxy_UpdateCvarTable` in [Proxy_CVars.cpp](../../src/JKA_YBEProxy/Proxy_CVars.cpp). The two prints of the original-engine registration are kept. No behaviour change, so no entry in the patch notes. Not compiled when written.

## A4. Tokenizer hardening

Status: **applied**, see O7 in [other-engines.md](other-engines.md).

[Proxy_cmd.cpp:34-36](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon/Proxy_cmd.cpp#L34-L36) copies `text_in` into `cmd_cmd`, then tokenizes `text_in`, which is unbounded, instead of the bounded copy. The output buffer `cmd_tokenized` is `BIG_INFO_STRING + MAX_STRING_TOKENS` bytes ([`qcommon/cmd_common.cpp:292`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/cmd_common.cpp#L292)). Tokenizing `cmd_cmd` guarantees no overflow.

The original engine also tokenizes `text_in` ([`qcommon/cmd_common.cpp:409-410`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/cmd_common.cpp#L409-L410)), and so do ioq3 and OpenJK. No caller found in the source passes more than `BIG_INFO_STRING` characters. This is hardening, not a regression.

Fix, as Quake3e does ([`code/qcommon/cmd.c:630-632`](https://github.com/ec-/Quake3e/blob/3173cef096dd0cb2fea26d91bce71049061f5a13/code/qcommon/cmd.c#L630-L632), `text = cmd_cmd; // read from safe-length buffer`):

```cpp
text = cmd_cmd;
```

Side effect: a text longer than `BIG_INFO_STRING - 1` characters is truncated instead of overflowing.

## A5. Build

Status: **applied** (CMake rewrite, includes, comparison of the SDK). Two optional items are left, at the end.

Problems found in the old build:

- the MSVC flags `/EHsc /sdl /Ot /Gy /TP /W4` were added to `CMAKE_C_FLAGS` only, and the project has no `.c` file, so only `/W4` and `/sdl` were really missing for C++ (CMake's default C++ flags already contain `/EHsc`, `/O2` implies `/Ot` and `/Gy`, `/TP` means nothing for C++ files);
- `-std=c++11` contradicted `CMAKE_CXX_STANDARD 20`;
- dead weight: `winmm`, `GetGitRevisionDescription` (the SHA was never used), Apple/Intel/PowerPC/BSD branches, MinGW and `linux-i686` toolchains, the `MSVCRuntime` macro.

Applied (CMake rewrite), based on [cpp-best-practices/cmake_template](https://github.com/cpp-best-practices/cmake_template) without tests, dependencies, WebAssembly, sanitizers, packaging and fuzzing:

- Layout: `CMakeLists.txt`, `ProjectOptions.cmake`, `cmake/*.cmake` (warnings, hardening, IPO, ccache, static analyzers, in-source build guard), `cmake/toolchains/linux-x86.cmake`, `CMakePresets.json` (replaces the `CreateVisualStudio*.bat` and `-DTARGET_ARCH=x86`), `src/CMakeLists.txt`. `CMakeModules/` is gone.
- One target, sources by `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`. The length disassembler is chosen with `CMAKE_SIZEOF_VOID_P`.
- Project flags are on the target (`target_compile_options`, `target_compile_definitions`), not in `CMAKE_*_FLAGS`. Same flags as before otherwise: `-fsigned-char`, `-masm=intel`, `-msse2` (32-bit), `-mstackrealign -mfpmath=sse` (GCC), static libgcc/libstdc++ and `--exclude-libs,ALL`, `-fvisibility=hidden`, static MSVC runtime (`MSVC_RUNTIME_LIBRARY`), `/arch:SSE2` (32-bit), same defines (`WIN64` dropped: `_WIN64` already does it).
- Warnings follow the template (`/W4` + its list, `-Wall -Wextra -Wshadow -Wconversion`...), as warnings only. `-Wno-invalid-offsetof -Wno-write-strings -Wno-comment` are kept, and `-Wold-style-cast` and `-Wuseless-cast` (GCC) are turned off: the engine code is written with C casts, and the `intptr_t` casts are useless on 32-bit only. `/permissive-` is not set: it would turn the `char*` from string literal conversions into errors.
- Warnings fixed after the first builds: the `disablewarnings.hpp` pragmas for the MSVC warnings 4136, 4284 and 4663 (they no longer exist), `= { 0 }` on structures (now `= {}`), unused parameters, `float` to `double` in `printf`, int/float conversions in the timenudge computation, and the `HookEntry` constructor parameters that shadowed members. No warning left on MSVC 19.51, GCC 12 (Debian 12) and GCC 16 (Fedora 44).
- Hardening and IPO (on by default in the template) are options, off. Hardening changes function bodies and the hooks patch those at runtime: not tested. IPO/LTO (Release builds only) was tested with the hooks on Windows and Linux and works, but the GCC build is about 40% bigger and no gain was measured.
- `CMAKE_CXX_EXTENSIONS` stays `ON` (the template turns it off): the copied engine code may use GNU extensions. Try `OFF` later.
- Built and tested on Windows (VS 2026), Debian 12 (GCC 12) and Fedora 44 in a container (GCC 16).

Includes and SDK:

- Includes of the proxy code (`src/JKA_YBEProxy/`) are now all rooted at `src/` (`"JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Wrappers.hpp"`, `"sdk/qcommon/q_shared.hpp"`). The files in `src/sdk/` keep the includes of their original (OpenJK and retail: bare names of the same folder, `.h` renamed `.hpp`), so a new diff against the original stays small. In `sdk/game/g_public.hpp` the `icarus/*.hpp` includes do not resolve, but they are inside `#if !defined(_GAME)`, never compiled.
- The folder keeps the name `sdk`. Compared on 2026-10-25 with OpenJK ([`260c59c`](https://github.com/JACoders/OpenJK/commit/260c59c2907187af555a676fe0cc798893bf7757)), ignoring `.h` to `.hpp` and CRLF (read only, nothing changed):
  - Identical to OpenJK: `ai`, `anims`, `bg_weapons`, `b_public`, `g_team`, `surfaceflags`, `tags`; `bg_vehicles` too, except for an include path.
  - Small, deliberate differences: `g_public.hpp` has the `Vehicle_t` typedef commented out; `teams.hpp` gives the enums a fixed type (`int32_t`) and includes `q_shared`; `g_local.hpp` has the `ClientConnect`/`ClientUserinfoChanged`/`ClientDisconnect`/`ClientBegin` declarations commented out (the proxy defines its own), and defines `GAMEVERSION`; `q_shared.hpp` uses `<cxxx>` headers and C++ aliases for `byte`, `qboolean`..., defines `cvar_t` (OpenJK's is commented out) and `BASE_COMPAT`; `disablewarnings.hpp` no longer disables the warnings 4136, 4284 and 4663 that OpenJK still lists.
  - `bg_public.hpp` lacks OpenJK's legacy fixes block (`CS_LEGACY_FIXES`, `legacyFixes_t`, `BG_FixSaberMoveData`, `BG_FixWeaponAttackAnim`) and the final `EV_NUM_ENTITY_EVENTS`. Nothing in the proxy uses them. The `BASE_COMPAT` guards around `PW_QUAD`, `PW_BATTLESUIT`, `EV_POWERUP_QUAD` and `EV_POWERUP_BATTLESUIT` are active because `q_shared.hpp` defines `BASE_COMPAT`, so the enum values are the same as the retail ones.
  - Not comparable: `qcommon.hpp`, `server.hpp` and `sys_public.hpp` are cut down to the structures the proxy needs, with the retail layout; `files.cpp`, `sv_client.cpp` (`ClientCleanName`) and `q_string.cpp` are short extracts.
  - Up to date on 2026-10-25: the local clones of OpenJK, JK2MV, ioq3 and Quake3e are at the head of their upstream. TaystJK has one newer commit, a cgame-only change ([`21044fb`](https://github.com/taysta/TaystJK/commit/21044fb2)).
  - Functions copied into `sdk` (`va`, `COM_CompareExtension`, `COM_DefaultExtension`, `Info_ValueForKey`, `Q_CleanStr`, `Q_strcat`, `Q_stricmpn`, `Q_StripColor`, `FS_CheckDirTraversal`) and `q_color.hpp`, `q_math.hpp`: same logic as OpenJK and ioq3 (only `std::size_t` instead of `int`/`size_t`). `Proxy_ClientCleanName` matches OpenJK's `ClientCleanName` and TaystJK's `SV_ClientCleanName`; TaystJK only strips the leading `*` on a retail game module. OpenJK has two things the SDK lacks and the proxy does not use: the `arm64` macOS branch of `q_platform.h` and `Q_PrintStrLenTo`.

Optional, left open:

- Add `.clang-format` / `.clang-tidy` once a style is chosen (the template has both).
- Check the sizes of `playerState_t`, `entityState_t` and `usercmd_t` with a `static_assert` (see the engine structures item above).
- Try `CMAKE_CXX_EXTENSIONS OFF`, `/permissive-` and the hardening option.

## A6. Pinned loader and reloadable plugin

Comes out of the F1 discussion: a pinned loader that holds the hooks and trampolines, plus a reloadable DLL with the logic. That allows hot reload even for code that must sit on the call stack. It matches the todo item "Create a plugin system", but it is more work than the F1 filter.

## A7. Minor observations

- **Confirmed, source only.** [Proxy_OriginalAPI_Wrappers.cpp:53-69](../../src/JKA_YBEProxy/Wrappers/Proxy_OriginalAPI_Wrappers.cpp#L53-L69): `Proxy_OriginalAPI_VM_Call` calls the proxy's `vmMain` directly and bypasses the engine's `VM_Call`, which sets `currentVM`. `VM_DllSyscall` uses `currentVM` ([`qcommon/vm.cpp:377`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/vm.cpp#L377)).
  - `VM_Free` sets `currentVM` to `NULL` ([`qcommon/vm.cpp:624`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/vm.cpp#L624)).
  - The next `VM_Call` (`GAME_INIT`) sets it to the game VM and never restores `NULL`, because it only restores a non-null previous value ([`qcommon/vm.cpp:800, 826-827`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/vm.cpp#L800)).
  - On a dedicated server, where only the game VM exists, `currentVM` is therefore always the game VM when the proxy runs. It works, but it would break with a second VM (listen server).
- **Confirmed, lower impact.** [Proxy_sv_client.cpp:671-819](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_client.cpp#L671-L819): `Proxy_SV_VerifyPaks_f` checks for duplicates in O(n²), and flood protection only applies from `CS_ACTIVE`. That flood rule is inherited from the original ([`server/sv_client.cpp:1620-1623`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp#L1620-L1623)).
  - The loop only runs when `sv_pure` is on and the cgame/ui checksums match.
  - A list that fails gets the client dropped. To repeat the command, a client must send a valid list, and its size is bounded by the server's pak count, because every checksum must be unique and known to the server.
- **Confirmed.** [Proxy_sv_client.cpp:31-32](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_client.cpp#L31-L32): the message "lost %i clientCommands" prints `seq - lastClientCommand + 1` (should be `- 1`). Cosmetic, same in the original ([`server/sv_client.cpp:1607-1608`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp#L1607-L1608)).
- **Refuted.** [Proxy_Engine_ClientCommand.cpp:76](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_ClientCommand.cpp#L76): "a name containing `\"` can break the `print` command".
  - The tokenizer ends a token at `"` ([Proxy_cmd.cpp:73-99](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon/Proxy_cmd.cpp#L73-L99)). Neither the connect packet nor the `userinfo` command can put a `"` in the userinfo, and `cl->name` comes from the userinfo.
  - A quoted token can contain a newline. A name with a newline only breaks the layout of the `netStatus` table. Cosmetic.

#### Applied

Status: **partly applied**. Only the "lost %i clientCommands" count was changed: it prints `seq - lastClientCommand - 1`, as Quake3e does. The retail source, ioq3, OpenJK, JK2MV, TaystJK, CleanJK and Quake III Arena all keep the `+ 1`. The other items need nothing: the `currentVM` case only matters with a second VM (a listen server, which the proxy does not support), and the `Proxy_SV_VerifyPaks_f` loop is bounded by the server's pak count. Not compiled when written.

## A8. Cvar to disable the welcome message

Request of the author. Today every human player gets the proxy name and version on their first connection, with no way to turn it off.

New cvar `proxy_sv_disableWelcomeMessage`: archived, any engine, default `0` (message sent, as today), `1` disables it.

- [Proxy_Header.hpp](../../src/JKA_YBEProxy/Proxy_Header.hpp), `Proxy_CVars_t`: add `vmCvar_t proxy_sv_disableWelcomeMessage;`.
- [Proxy_CVars.cpp](../../src/JKA_YBEProxy/Proxy_CVars.cpp), `proxyCVarTable`: add `{ &proxy.cvars.proxy_sv_disableWelcomeMessage, "proxy_sv_disableWelcomeMessage", "0", CVAR_ARCHIVE, 0, qfalse },`.
- [Proxy_SharedAPI.cpp](../../src/JKA_YBEProxy/Proxy_SharedAPI.cpp), `Proxy_SharedAPI_ClientBegin`: still clear `sendWelcomeMessage`, but only send the message when the cvar is `0`:

  ```cpp
  if (currentClientData->sendWelcomeMessage)
  {
  	currentClientData->sendWelcomeMessage = false;

  	if (!proxy.cvars.proxy_sv_disableWelcomeMessage.integer)
  	{
  		proxy.trap->SendServerCommand(clientNum, va("print \"^5%s (^7%s^5) %s^7\n\"", YBEPROXY_NAME, YBEPROXY_VERSION, YBEPROXY_BY_AUTHOR));
  	}
  }
  ```

- Document the cvar in [features.md](../features.md), next to the other cvars and in the "Welcome message" section.

Proxy cvars are read about once per second (`Proxy_UpdateAllCvars` in `GAME_RUN_FRAME`), which is enough here.

#### Applied

Status: **applied**, as described above. Not compiled when written.

## A9. Stateless challenges (optional)

Not required: the F6 rate limiting covers the floods that come from one address. This only matters for a flood of spoofed `getchallenge` packets from many different addresses.

The problem, in the retail engine ([`server/sv_client.cpp:31-73`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp#L31-L73)):

- `SV_GetChallenge` stores each new address in `svs.challenges`, 1024 slots, and overwrites the oldest one when the table is full.
- `SV_DirectConnect` only accepts a `connect` whose challenge is still in the table.
- A spoofed flood of more than about 1024 new addresses in the time a player takes between `getchallenge` and `connect` evicts the player's challenge: the player gets `No or bad challenge for address.`. The F6 per-address limit does not help, since every spoofed packet comes from a new address.

What the other engines do:

| Engine | Challenges |
| --- | --- |
| ioq3, JK2MV | Table of 2048 slots, and one address cannot take more than half of them (`MAX_CHALLENGES_MULTI`). Harder to flood, still possible. |
| OpenJK, TaystJK | Stateless: the challenge is an HMAC-MD5 of the address and of a timestamp that changes every 16.384 s, with a secret key from the OS random generator. Nothing is stored, so nothing can be evicted ([`codemp/server/sv_challenge.cpp`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/server/sv_challenge.cpp), PR [#832](https://github.com/JACoders/OpenJK/pull/832)). |
| Quake3e | Stateless, backported from OpenJK ([`code/server/sv_client.c:30-80`](https://github.com/ec-/Quake3e/blob/3173cef096dd0cb2fea26d91bce71049061f5a13/code/server/sv_client.c#L30-L80)). |

Sketch for the proxy, in the F1 filter, without rewriting `SV_DirectConnect`:

1. `getchallenge`: the filter answers `challengeResponse <HMAC challenge>` itself, then drops the packet. The retail client sends no argument and only reads the challenge number.
2. `connect`: the filter decompresses a **copy** of the packet (the original decompresses it in place, see the F1 pitfalls), reads `challenge` from the userinfo, and checks the HMAC for the current and the previous timestamp. When it is valid, it writes the entry into `svs.challenges` (address, challenge, `connected = qfalse`, times) just before the original `SV_DirectConnect` runs and looks it up.
3. Things to keep: the ping measured between the challenge and the connect (`challenge->pingTime`), which `SV_DirectConnect` uses for `sv_minPing` / `sv_maxPing`. A stateless challenge loses it, so the time must travel in the timestamp or be measured another way.
4. Needs an MD5/HMAC implementation (OpenJK's `qcommon/md5.cpp`) and a random secret per server start.
5. LAN addresses: the retail `SV_GetChallenge` answers them at once, keep that.

Cost: no new hook, but a non-trivial change on the connection path, to test with the retail client on both platforms.
