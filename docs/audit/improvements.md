# Improvements

Not bugs as such: robustness, simplification, missing features and build hygiene. Paths are relative to the repository root.

## A1. Hook robustness

- **Signature check.** The only check before patching is the `version` cvar string ([Proxy_Main.cpp:83](../../src/JKA_YBEProxy/Proxy_Main.cpp#L83)). Before patching, check the expected bytes at every hook address. Abort everything on a mismatch.
- **Relative instructions.** `GetTramp` copies the first bytes of the target as they are ([HookUtils.hpp:79](../../src/JKA_YBEProxy/RuntimePatch/HookUtils/HookUtils.hpp#L79)). An `E8`/`E9`/`Jcc`/`EB` with a relative operand would jump to the wrong place once copied into the trampoline. HDE sets `F_RELATIVE` for these: refuse the hook, or relocate the operand.
- **Partial failure.** If `Attach` fails for one hook ([Proxy_Engine_Patch.cpp:55-65](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Patch.cpp#L55-L65)), its `Original_X` stays `nullptr` while the other hooks are installed. The first call through it is a null call. Roll back every hook instead.
- **Struct layout.** There is no `static_assert` on the engine structures, although the offsets are already written as comments ([server.hpp:186-195](../../src/sdk/server/server.hpp#L186-L195)). Add for example `static_assert(offsetof(serverStatic_t, clients) == 0x0C)` and a `sizeof(client_t)` check, per platform.
- **Optional:**
  - put all trampolines in one page instead of one `VirtualAlloc` each (64 KB granularity per call);
  - switch the trampoline memory to RX once written instead of keeping it RWX;
  - call `FlushInstructionCache` after patching.

## A2. Bridge for OpenJK with the retail jampgame

This is the most useful OpenJK case for this project, since the whole point is to keep the retail game DLL (see [feeling.md](feeling.md)). Today it ends in `ERR_FATAL` (see F4 in [fixes.md](fixes.md)).

- No configuration workaround exists: the `vm_legacy` cvar, which would force the legacy API, only exists in `_DEBUG` builds of OpenJK (checked).
- Solution: a bridge inside `GetModuleAPI`:
  - an export table whose functions call `originalVmMain(GAME_*, ...)`;
  - a syscall dispatcher, passed to the original `dllEntry`, that translates each `G_*` syscall into a call to `gi->X`.
- Both halves already exist in OpenJK `codemp/server/sv_gameapi.cpp`:
  - the legacy branch of the `GVM_*` functions (`VM_Call` with `GAME_*`);
  - `SV_GameSystemCalls` (a switch on `G_*`).
  The licence is GPL, like this project, so it is mechanical adaptation. It is essentially `Proxy_Translate_SystemCalls` in reverse.
- Watch the float conventions (`PASSFLOAT` / `VMF`) when translating.

## A3. Simplification

- The F1 filter in [fixes.md](fixes.md) removes about 200 lines (rewrites of `SV_PacketEvent`, `SV_ConnectionlessPacket`, the `SVC_RemoteCommand` wrapper, and the `SVC_Status`/`SVC_Info` hooks).
- [Proxy_CVars.cpp](../../src/JKA_YBEProxy/Proxy_CVars.cpp): `Proxy_UpdateCvars` and `Proxy_OriginalEngine_UpdateCvars` are identical except for the table, and so are the two registration functions. One function taking the table and its size is enough for each.

## A4. Tokenizer hardening

[Proxy_cmd.cpp:34-36](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon/Proxy_cmd.cpp#L34-L36) tokenizes `text_in`, which is unbounded, instead of the bounded copy `cmd_cmd`. `cmd_tokenized` is `BIG_INFO_STRING + MAX_STRING_TOKENS` bytes, so tokenizing `cmd_cmd` guarantees no overflow.

The original engine behaves the same way, so this is not a regression, only hardening.

## A5. Build

- [CMakeLists.txt:141](../../CMakeLists.txt#L141): the MSVC flags `/EHsc /sdl /Ot /Gy /TP /W4` are added to `CMAKE_C_FLAGS` only. The project has no `.c` file, so they are not applied to the C++ code. Move them to `CMAKE_CXX_FLAGS` or `target_compile_options`.
- [CMakeLists.txt:186](../../CMakeLists.txt#L186): `-std=c++11` contradicts `CMAKE_CXX_STANDARD 20`, and the code needs at least C++17 (`std::variant`, class template argument deduction). Remove it.
- Related todo item: "Redo the CMake project".

## A6. Pinned loader and reloadable plugin

Comes out of the F1 discussion: a pinned loader that holds the hooks and trampolines, plus a reloadable DLL with the logic. That allows hot reload even for code that must sit on the call stack. It matches the todo item "Create a plugin system", but it is more work than the F1 filter.

## A7. Minor observations (not raised during the review discussion)

- [Proxy_OriginalAPI_Wrappers.cpp:53-69](../../src/JKA_YBEProxy/Wrappers/Proxy_OriginalAPI_Wrappers.cpp#L53-L69): `Proxy_OriginalAPI_VM_Call` calls the proxy's `vmMain` directly and bypasses the engine's `VM_Call`, which sets `currentVM` (used by `VM_DllSyscall`). It works on a dedicated server, where only the game VM exists, but it is fragile.
- [Proxy_sv_client.cpp:671-819](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_client.cpp#L671-L819): `Proxy_SV_VerifyPaks_f` checks for duplicates in O(n²), and flood protection only applies from `CS_ACTIVE`. A connecting client can spam `cp` with hundreds of arguments. Low impact.
- [Proxy_sv_client.cpp:31-32](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_client.cpp#L31-L32): the message "lost %i clientCommands" prints `seq - lastClientCommand + 1` (should be `- 1`). Cosmetic, inherited from Q3.
- [Proxy_Engine_ClientCommand.cpp:76](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_ClientCommand.cpp#L76): `netStatus` prints the raw `cl->name` inside `print "..."`. A crafted name containing `"` can break the server command string. Cosmetic.
