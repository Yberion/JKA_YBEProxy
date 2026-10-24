# Improvements

Not bugs as such: robustness, simplification, missing features and build hygiene. Paths are relative to the repository root. The verification statuses are the same as in [fixes.md](fixes.md).

## A1. Hook robustness

### Signature check

Status: confirmed. The only check before patching is the `version` cvar string ([Proxy_Main.cpp:83](../../src/JKA_YBEProxy/Proxy_Main.cpp#L83)).

Fix:

1. Next to each address in [Proxy_Engine_Wrappers.hpp](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Wrappers.hpp), store the first bytes expected at that address, per platform. Read them from the binaries once.
2. Before `Proxy_Engine_Inline_Patches` and `Proxy_Engine_Attach_Patches`, compare every hook and every inline patch address with `memcmp`.
3. On any mismatch, patch nothing, print which address failed, and run like on an unknown engine (`isOriginalEngine = false`).

### Relative instructions

Status: confirmed. HDE32 sets `F_RELATIVE` ([hde32.hpp:34](../../src/JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/x86/hde32.hpp#L34), set in [hde32.cpp:268, 306, 310](../../src/JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/x86/hde32.cpp#L268)).

`GetTramp` copies the first bytes of the target as they are ([HookUtils.hpp:79](../../src/JKA_YBEProxy/RuntimePatch/HookUtils/HookUtils.hpp#L79)). A `call`, `jmp` or `jcc` with a relative operand jumps to the wrong place once copied into the trampoline. `GetLen` also ignores `F_ERROR` ([HookUtils.cpp:74-88](../../src/JKA_YBEProxy/RuntimePatch/HookUtils/HookUtils.cpp#L74-L88)).

Checked on the retail binaries with capstone (files listed in F1): none of the 18 current hooks has a relative instruction in its copied bytes, on Windows or Linux. But the hook F7 needs does: on Windows, `Com_WriteConfig_f` starts with `sub esp, 0x40; call Cmd_Argc`, 8 bytes copied with an `e8 rel32` inside.

Fix:

1. In `GetTramp`, after the copy, walk the copied instructions with HDE. For an `e8` (call) or `e9` (jmp) with a 32-bit relative operand, keep the same target:

   ```cpp
   // The copied call/jmp is now at pTramp + offset instead of originalFunctionAddr + offset:
   // shift its rel32 by the distance between the two copies so that it still reaches the same target
   *(int32_t*)(pTramp + offset + 1) += (int32_t)(hookEntry.originalFunctionAddr - pTramp);
   ```

2. For any other instruction with `F_RELATIVE` (short `jcc`/`jmp`, `loop`), or with `F_ERROR`, refuse the hook: `Attach` returns false.

x86 only: on 64-bit, the trampoline may be too far for a 32-bit offset.

### Partial failure

Status: **corrected**. There is no null call.

- When `GetTramp` fails, `Attach` returns before it writes the detour ([HookUtils.hpp:100-105](../../src/JKA_YBEProxy/RuntimePatch/HookUtils/HookUtils.hpp#L100-L105)). The engine keeps calling the unpatched function.
- Each `Original_X` is only called from its own `Proxy_X`, which is only reached through that detour. Checked: [Proxy_sv_client.cpp:477, 498, 593, 939](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_client.cpp#L477) and [Proxy_sv_main.cpp:100, 114, 146](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_main.cpp#L100).

The real problems:

- The server runs with part of the fixes missing, and only one line in the console says so.
- `Detach` then prints "Could not release memory" for the hook that never attached.
- Both messages lack a final `\n` ([Proxy_Engine_Patch.cpp:61, 82](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Patch.cpp#L61)).

Fix: if any `Attach` fails, detach the hooks already attached and print one clear error. Low priority: `VirtualAlloc`/`mmap` of a few bytes practically never fails.

### Struct layout

Status: confirmed. There is no `static_assert` on the engine structures, although the addresses of the fields are written as comments, Windows then Linux ([server.hpp:186-195](../../src/sdk/server/server.hpp#L186-L195)).

Fix: turn them into offsets. For example, `serverStatic_t` starts at `0x606218` and `clients` is at `0x606224`, so:

```cpp
static_assert(offsetof(serverStatic_t, clients) == 0x0C);
```

Add a `sizeof(client_t)` check too, per platform.

### Optional

- Put all trampolines in one page. Today each one is a `VirtualAlloc`, which reserves 64 KB of address space.
- Switch the trampoline memory to RX once written, instead of keeping it RWX.
- Call `FlushInstructionCache` after patching. x86 does not need it in practice, but Windows documents it.

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

## A4. Tokenizer hardening

Status: confirmed.

[Proxy_cmd.cpp:34-36](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon/Proxy_cmd.cpp#L34-L36) copies `text_in` into `cmd_cmd`, then tokenizes `text_in`, which is unbounded, instead of the bounded copy. The output buffer `cmd_tokenized` is `BIG_INFO_STRING + MAX_STRING_TOKENS` bytes ([`qcommon/cmd_common.cpp:292`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/cmd_common.cpp#L292)). Tokenizing `cmd_cmd` guarantees no overflow.

The original engine also tokenizes `text_in` ([`qcommon/cmd_common.cpp:409-410`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/cmd_common.cpp#L409-L410)), and so do ioq3 and OpenJK. No caller found in the source passes more than `BIG_INFO_STRING` characters. This is hardening, not a regression.

Fix, as Quake3e does ([`code/qcommon/cmd.c:630-632`](https://github.com/ec-/Quake3e/blob/3173cef096dd0cb2fea26d91bce71049061f5a13/code/qcommon/cmd.c#L630-L632), `text = cmd_cmd; // read from safe-length buffer`):

```cpp
text = cmd_cmd;
```

Side effect: a text longer than `BIG_INFO_STRING - 1` characters is truncated instead of overflowing.

## A5. Build

Status: **corrected**. From CMake's documented behaviour, not observed in a generated project.

- [CMakeLists.txt:141](../../CMakeLists.txt#L141): the MSVC flags `/EHsc /sdl /Ot /Gy /TP /W4` are added to `CMAKE_C_FLAGS` only, and the project has no `.c` file. Smaller impact than first written:
  - CMake's default C++ flags already contain `/EHsc` (`/DWIN32 /D_WINDOWS /W3 /GR /EHsc`);
  - `/O2`, CMake's default in Release, already implies `/Ot` and `/Gy`;
  - `/TP` means nothing for C++ files.

  Only `/W4` and `/sdl` are really missing for C++. Moving them may raise new warnings, and `/sdl` turns some of them into errors: test a build before committing.
- [CMakeLists.txt:186](../../CMakeLists.txt#L186): `-std=c++11` contradicts `CMAKE_CXX_STANDARD 20`. In practice CMake adds its own `-std=gnu++20` after `CMAKE_CXX_FLAGS`, which is why `std::variant` compiles. Remove `-std=c++11` for clarity.
- The GCC C++ flags are fine: `-mstackrealign` and `-mfpmath=sse` are set for C++ as well ([CMakeLists.txt:226-229](../../CMakeLists.txt#L226-L229)).
- Related todo item: "Redo the CMake project".

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
