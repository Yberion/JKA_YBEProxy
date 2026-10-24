# JKA_YBEProxy audit

Audit done on 2026-10-24 against version `0.19.0 Beta` (commit [`57d9a59`](https://github.com/Yberion/JKA_YBEProxy/commit/57d9a59)). Second review on the same day: every claim was checked again against the sources, then corrected. Each item now has a verification status. A third pass corrected the F9 fix and added a point to F10. F11 (`dumpuser` crash) was added on 2026-10-24, confirmed by observation.

## Files

- [fixes.md](fixes.md): bugs and security issues to fix, ranked by severity (F1 to F11).
- [improvements.md](improvements.md): robustness, simplification, full OpenJK support, build (A1 to A9).
- [other-engines.md](other-engines.md): comparison with Quake 3, ioquake3, Quake3e, OpenJK, JK2MV and TaystJK (EternalJK). Fixes missing from the proxy (O1 to O11), and differences that need nothing.
- [feeling.md](feeling.md): why a recompiled engine (`linuxjampded`) or game module (`jampgame`) changes the combat "feeling", leads and a method to find the cause.

## Scope

- Read in full: everything under `src/JKA_YBEProxy`, except `Proxy_Translate_SystemCalls.cpp` (skimmed) and `LengthDisassembler` (HDE: only its flags were checked).
- Read in part: `src/sdk` (`server.hpp`, `sv_client.cpp`, `q_string.cpp`, `sys_public.hpp`, `q_shared.hpp`).
- Nothing was compiled or run. The retail binaries were only read (disassembled with capstone) for the checks listed below. The other addresses in [Proxy_Engine_Wrappers.hpp](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Wrappers.hpp) were not checked one by one.

## Sources

Multiplayer code only (`codemp`). Each repository was read at the commit below, and the links in the audit files are pinned to it:

| Repository | Commit read | Notes |
| --- | --- | --- |
| [jedis/jediacademy](https://github.com/jedis/jediacademy) | [`4bebb8e`](https://github.com/jedis/jediacademy/commit/4bebb8ec23200ee150a9aa566cea6122c19eba44) | Raven's JKA source. Close to the retail binaries, but not identical: a claim that only this source supports is marked "source only", still to confirm on the binary. |
| [id-Software/Quake-III-Arena](https://github.com/id-Software/Quake-III-Arena) | [`dbe4ddb`](https://github.com/id-Software/Quake-III-Arena/commit/dbe4ddb10315479fc00086f08e25d968b4b43c49) | id's Quake 3 1.32 |
| [JACoders/OpenJK](https://github.com/JACoders/OpenJK) | [`260c59c`](https://github.com/JACoders/OpenJK/commit/260c59c2907187af555a676fe0cc798893bf7757) | |
| [ioquake/ioq3](https://github.com/ioquake/ioq3) | [`87a99d1`](https://github.com/ioquake/ioq3/commit/87a99d18206e878a20289f05ac3c16005dcae5c3) | |
| [ec-/Quake3e](https://github.com/ec-/Quake3e) | [`3173cef`](https://github.com/ec-/Quake3e/commit/3173cef096dd0cb2fea26d91bce71049061f5a13) | |
| [mvdevs/jk2mv](https://github.com/mvdevs/jk2mv) | [`7d60145`](https://github.com/mvdevs/jk2mv/commit/7d601454c3db68492289d4d4e3dc30bff39e4246) | |
| [taysta/TaystJK](https://github.com/taysta/TaystJK) | [`7d99efb`](https://github.com/taysta/TaystJK/commit/7d99efb4a77a174d1a7853f2a8024daf78fa47b7) | Fork of EternalJK |

A path without a repository root, such as `server/sv_client.cpp:520`, is in jediacademy `codemp`.

## Main facts checked against the sources

Retail binaries (`jampDed.exe` and `linuxjampded`, read with capstone, nothing executed; versions and SHA-1 in F1):

- The packet path makes real calls (`SV_PacketEvent` → `SV_ConnectionlessPacket` → `SVC_RemoteCommand`/`SVC_Status`/`SVC_Info`, and `SV_ExecuteClientMessage`).
- `SV_PacketEvent` is cdecl, with the 20-byte `netadr_t` on the stack, and no relative instruction in its first bytes.
- None of the 18 current hooks copies a relative instruction into its trampoline.
- `Com_WriteConfig_f` is at `0x410680` (Windows) and `0x8074d64` (Linux). The Windows one starts with a relative `call`, which needs the A1 relocation before hooking.

Engine (jediacademy):

- `SV_DirectConnect` saves the userinfo, calls `GAME_CLIENT_CONNECT`, then `SV_UserinfoChanged` ([`server/sv_client.cpp:517-530`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp#L517-L530)). The game's `ClientConnect` calls `ClientUserinfoChanged` directly ([`game/g_client.c:2347`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_client.c#L2347)).
- Reconnects on map change go through `GAME_CLIENT_CONNECT` with `firstTime = qfalse` ([`server/sv_init.cpp:700`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_init.cpp#L700), [`server/sv_ccmds.cpp:326`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_ccmds.cpp#L326)).
- The game DLL is unloaded and reloaded on every map change and `map_restart` (`VM_Free` calls `Sys_UnloadDll`, `VM_Restart` calls `VM_Free` then `VM_Create`). `VM_Call` sets `currentVM`, which `VM_DllSyscall` uses, and only restores a non-null previous value.
- `SVC_RemoteCommand`: global 500 ms timer before the password check; `remaining` built from `Cmd_Argv`; `Com_EndRedirect` only after `Cmd_ExecuteString` returns ([`server/sv_main.cpp:490-533`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_main.cpp#L490-L533)). No rate limit at all on `getstatus`/`getinfo`.
- `Com_Printf` returns early during a redirect, without the console or the log.
- `sv_gametype = Cvar_Get("g_gametype", ...)`; `sv_fps` registered with `CVAR_TEMP`; `Cvar_Get` ORs the flags into an existing cvar and applies a pending latched value; `trap_Cvar_Set` is a forced set that ignores the latch.
- `Com_WriteConfig_f` only adds `.cfg` when the name has no extension; `FS_FOpenFileWrite` checks neither the extension nor `..`.
- `sv_fps` below 1 is reset to 10 at the start of `SV_Frame`.
- Bots never reach `SV_UserMove`: their usercmds go through `BOTLIB_USER_COMMAND` straight to `SV_ClientThink` (`server/sv_game.cpp:977-979`).
- The engine never maintains the `ip` userinfo key after the connect (see O1).
- Windows `Sys_UnpackDLL` extracts the DLL only when it is inside a pk3; Linux loads the `.so` from `fs_basepath`.

Game (jediacademy):

- The forcepowers crash is server-side: `BG_LegalizedForcePowers` uses uninitialised `final_Powers[]` entries as an array index ([`game/bg_misc.c:439-563`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/bg_misc.c#L439-L563)).
- The game compares the command names and arguments that the proxy filters whole, with `Q_stricmp` (see F10).
- `target_level_change` runs `map` with `EXEC_NOW` from inside the game ([`game/g_target.c:949`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_target.c#L949)).

Other repositories:

- OpenJK: `GetModuleAPI` returning `NULL` gives `ERR_FATAL`; `vm_legacy` only exists in `_DEBUG` builds; a DLL without `GetModuleAPI` is loaded through the legacy API; the OpenJK game keeps the `import` pointer and also exports `vmMain`/`dllEntry`.
- JK2MV: origin of the proxy's forcepowers fix, ping fix (including the `== -1` test, commit [`0301bea`](https://github.com/mvdevs/jk2mv/commit/0301bea)) and `SV_SvEntityForGentity` change.
- TaystJK (EternalJK): the same engine-side fixes as the proxy in its `sv_legacyFixes`, plus a fix for the movement event lag exploit (O9).
- The proxy's SDK `g_public.hpp` is OpenJK's `g_public.h`. Its syscall and `GAME_*` enums have the same order as the retail ones (367 entries compared).
- Repository: `Q_stricmpn(..., 0)` returns 0 (`src/sdk/qcommon/q_string.cpp`).

## Verdict

The approach is the right one. Going through `vmMain` and the syscalls keeps the retail `jampgamex86.dll` untouched, so gameplay stays bit-identical while security issues get fixed. The code is readable. Addresses are centralised in one header, and the `std::variant` hook table is clean. Some rewritten engine functions mark their changes with `// Proxy -->` / `// Proxy <--`, but not all of them: for example `Proxy_SV_UpdateUserinfo_f` (empty argument check and flood check moved before the copy, from OpenJK), the tokenizer and `Proxy_SV_Status_f` differ from the original without markers. So the markers do not list every change: comparing with the original source is still needed.

The risk sits in two places:

- the engine-patching layer (hooks and rewritten engine functions), which causes the `rcon map` crash;
- the connection path, where the game sees the client's userinfo before any proxy check, which bypasses the name, model and `forcepowers` fixes (F2).

## Suggested order

1. F2, then F3: userinfo sanitised on connect, and the anti-cheat fixed in the same function. Small changes. F2 closes the bypass of the name, model and `forcepowers` fixes on connect. Add the checks of O8 (duplicate keys, failed `Info_SetValueForKey`) and O1 (`ip`) at the same time: without O8, a long userinfo still gets through.
2. F9: a local variable in `netStatus`, and one test deleted in `Proxy_SV_UserMove`. Today a display command changes the snapshot rate of real players.
3. F1: `rcon map` crash. Both checks on the binaries are done: the design works as is.
4. F6, with F7: rcon and getstatus/getinfo rate limiting in the F1 filter, and `writeconfig`.
5. F5: wrong cvars.
6. F4: option A (no `GetModuleAPI` export). It covers OpenJK with the retail jampgame and OpenJK with an OpenJK jampgame.
7. O9: movement event lag (the todo's ffa3 exploit).
8. The rest: F8, F10, A1, A3 to A5, O5, O7, O10, O11.

## Decisions of the author

- F8: the address-based index in `SV_SvEntityForGentity` is the JK2MV player ghosting fix. Keep it, add the bounds checks.
- F3: kick every model that starts with `darksidetools`.
- F4: OpenJK must keep working, including an OpenJK jampgame on an OpenJK server. Option A covers it.
- F5: the proxy only reads `g_gametype` and `sv_fps`: register them with flags 0.
- F3: the wider rule for `rancor`, `wampa` and `jedi_` is accepted.
- F10 `/myratio`: the current rule is intended (color from `kill - death`, a suicide shows `0` in red). Only clamp the red number to 0.
- O9: fix it. The author knows the bug and has reproduced it.
- O5: add a `proxy_sv_maxClientsPerIP` cvar (default 3, 0 disables).
- O8: port OpenJK's whole userinfo validation, compatible with the retail client (checked against its userinfo cvars).

## Open questions for the author

None left.
