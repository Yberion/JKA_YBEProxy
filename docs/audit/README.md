# JKA_YBEProxy audit

Audit done on 2026-10-24 against version `0.19.0 Beta` (commit `57d9a59`).

## Files

- [fixes.md](fixes.md): bugs and security issues to fix, ranked by severity (F1 to F10).
- [improvements.md](improvements.md): robustness, simplification, full OpenJK support, build (A1 to A7).
- [feeling.md](feeling.md): why a recompiled engine (`linuxjampded`) or game module (`jampgame`) changes the combat "feeling", leads and a method to find the cause.

## Scope

- Read in full: everything under `src/JKA_YBEProxy`, except `Proxy_Translate_SystemCalls.cpp` (skimmed) and `LengthDisassembler` (HDE, not reviewed).
- Read in part: `src/sdk` (`server.hpp`, `sv_client.cpp`, `q_string.cpp`).
- Nothing was compiled or run. The addresses of the original binaries were not checked.

Facts checked against the original sources:

- jediacademy `codemp/server/sv_client.cpp`: call order in `SV_DirectConnect`, body of `SV_UpdateUserinfo_f`.
- jediacademy `codemp/server/sv_main.cpp`: `SV_ConnectionlessPacket`; `SVC_RemoteCommand` (global 500 ms timer, `remaining` built from `Cmd_Argv`); no rate limiting at all on `getstatus`/`getinfo`.
- jediacademy `codemp/server/sv_init.cpp`: `sv_gametype = Cvar_Get ("g_gametype", ...)` and `sv_fps = Cvar_Get ("sv_fps", "20", CVAR_TEMP )`.
- jediacademy `codemp/server/sv_game.cpp` and `codemp/qcommon/vm.cpp`: the game DLL is unloaded and reloaded on every map change and every `map_restart` (`VM_Free` calls `Sys_UnloadDll`; `VM_Restart` calls `VM_Free` then `VM_Create`). `VM_Call` sets `currentVM`, which `VM_DllSyscall` uses.
- OpenJK `codemp/server/sv_gameapi.cpp`: when `GetModuleAPI` returns `NULL`, the engine calls `Com_Error( ERR_FATAL, "GetGameAPI failed on %s", dllName )`.
- OpenJK `codemp/qcommon/vm.cpp`: the `vm_legacy` cvar only exists in `_DEBUG` builds.

## Verdict

The approach is the right one. Going through `vmMain` and the syscalls keeps the retail `jampgamex86.dll` untouched, so gameplay stays bit-identical while security issues get fixed. The code is readable. The `// Proxy -->` / `// Proxy <--` markers inside rewritten engine functions make diffing against the original easy. Addresses are centralised in one header, and the `std::variant` hook table is clean.

The risk sits in the engine-patching layer (hooks and rewritten engine functions). Most of the bugs are there, including the cause of the `rcon map` crash.

## Suggested order

1. F1: crash on `rcon map` / `rcon map_restart`.
2. F2: userinfo not sanitised on connect.
3. F3: anti-cheat false positive.
4. F5: wrong cvars read.
5. F6: rcon rate limiting and getstatus/getinfo amplification.
6. F4, then A2: OpenJK support (confirmed: OpenJK must stay supported).
