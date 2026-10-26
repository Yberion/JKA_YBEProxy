# Fixes

Severity: Critical, High, Medium, Low. Paths are relative to the repository root.

Each item has a verification status from the second review:

- **Confirmed**: checked in the proxy code and in the sources.
- **Source only**: confirmed in the jediacademy source, still to confirm on the retail binaries. That source is close to retail, but not identical.
- **Corrected**: the first version of the claim was wrong or incomplete. The text below is the corrected one.
- **Refuted**: the claim was wrong. It stays here, marked, so that nobody adds it back.

Source references link to GitHub, pinned to the commit that was read, so that the line numbers stay right. A path without a repository root, such as `server/sv_client.cpp:520`, is in [jediacademy](https://github.com/jedis/jediacademy) `codemp`. The other repositories used are Quake 3, OpenJK, ioquake3, Quake3e, JK2MV and TaystJK. See the source list in [README.md](README.md).

## F1. Crash on `rcon map` and `rcon map_restart` (Critical)

Status:

- Cause: **source only**, strongly supported, never reproduced.
  - The engine unloads the game DLL, which is the proxy, on every map change and `map_restart`.
  - Git history: [todo.md](../todo.md) says the crash started "since the hooked `SVC_RemoteCommand` was added". Commit [`84df0e0`](https://github.com/Yberion/JKA_YBEProxy/commit/84df0e0) (2022-10-03) is that commit. It is the first one that put a proxy frame (`Proxy_SVC_RemoteCommand`) below `Cmd_ExecuteString` on the rcon path.
- Link with Discord: speculative.
- First version of the stub: **corrected**. It was unsafe, see [The stub](#the-stub).
- "basejka never uses `EXEC_NOW` to reload the VM": **refuted**, see [Why the original game DLL is not affected](#why-the-original-game-dll-is-not-affected).

### Symptom

Known bug from the README and the todo: intermittent crash on `rcon map mp/ffa3` (any map) or `rcon map_restart 0`, sometimes correlated with Discord running on the server machine. It involves the `SV_PacketEvent`, `SV_ConnectionlessPacket` and `SVC_RemoteCommand` hooks.

### Cause

The engine unloads and reloads the game DLL, which is the proxy, on every map change and every `map_restart`:

- `SV_ShutdownGameProgs` calls `VM_Free`, which calls `Sys_UnloadDll`.
- `SV_RestartGameProgs` calls `VM_Restart`, which calls `VM_Free` then `VM_Create`.

During an `rcon map`, the call stack still contains proxy frames when the unload happens:

```
Com_EventLoop                          engine
 Proxy_SV_PacketEvent                  PROXY   <- frame still active
  Proxy_SV_ConnectionlessPacket        PROXY   <- frame still active
   Proxy_SVC_RemoteCommand             PROXY   <- frame still active
    SVC_RemoteCommand (trampoline)     engine
     Cmd_ExecuteString("map ...")      engine
      SV_SpawnServer
       SV_ShutdownGameProgs
        vmMain(GAME_SHUTDOWN)          PROXY            (call, then return)
         vmMain(GAME_SHUTDOWN)         ORIGINAL DLL     (call, then return)
         FreeLibrary(original DLL)     no frame of the original DLL on the stack: OK
        VM_Free -> FreeLibrary(proxy)  3 proxy frames are still above: they return into unloaded code
```

When the stack unwinds, the CPU returns into the address range of the old proxy instance:

- If the new instance was mapped at the same base address, the code bytes are identical and it works by luck.
- If it was mapped elsewhere, it crashes. Any change to the address space layout can cause that, for example another process injecting a DLL (Discord).

### Why the original game DLL is not affected

The proxy unloads the original DLL in the same way (`FreeLibrary` in `GAME_SHUTDOWN`). The difference is that the original DLL is only ever called as a leaf: the engine or the proxy calls `vmMain`, the DLL does its work, then it returns. When it is freed, no return address points into it. Being reloaded at another address does not matter: its state starts from scratch, and the engine gets fresh pointers through `GAME_INIT` and `LocateGameData`.

Because of its detours, the proxy becomes a *caller* of engine code instead of a leaf. It ends up below the `Cmd_ExecuteString` that triggers the map change. The engine unloads it without knowing that proxy frames are still waiting to return.

The original DLL crashes the same way if it triggers a map change synchronously. Vote map changes use `EXEC_APPEND`: the command goes into the Cbuf and runs later from `Com_Frame`, when no game frame is on the stack.

Correction: basejka does trigger one synchronously. `target_level_change_use` runs `trap_SendConsoleCommand(EXEC_NOW, "map ...")` ([`game/g_target.c:949`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_target.c#L949)). `Cbuf_ExecuteText(EXEC_NOW)` calls `Cmd_ExecuteString` directly ([`qcommon/cmd_common.cpp:125-131`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/cmd_common.cpp#L125-L131)). A map that uses this entity unloads the game DLL while game frames are on the stack, with or without the proxy. This is a vanilla bug, the fix below does not cover it, and MP maps rarely use this entity.

### Rejected option: pinning the DLL

- Windows: `GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&vmMain, &h)`.
- Linux: `dladdr` to find the module path, then `dlopen(path, RTLD_NOW | RTLD_NOLOAD | RTLD_NODELETE)`.

This fixes the crash, but the DLL is then never unloaded. A new proxy build would only load after a full server restart, and every global would survive map changes. Rejected because hot reload through a map change is wanted.

### Recommended fix: filter stub at the entry of `SV_PacketEvent`

Rule: no proxy frame may be on the stack while original engine code runs a command. On the packet path, replace the rewrites and wrappers with one filter that decides, then jumps to the original code.

#### The stub

The detour at the entry of `SV_PacketEvent` jumps to a small generated stub instead of a C++ function. The stub calls the filter. If the filter returns 0, the stub returns to the engine and drops the packet. Otherwise it jumps to the trampoline, and the original `SV_PacketEvent` runs with no proxy frame on the stack.

Correction to the first version: the filter must receive a **copy** of the arguments. Under cdecl, a callee owns its argument slots and the compiler may write to them, for example to spill a register or for a tail call. In the first version, the filter's argument slots were the engine's own slots: the return address of `SV_PacketEvent` and the `from`/`msg` arguments that the trampoline reads next. One write there and the stub returns to garbage, or the original reads a corrupted packet.

```
; on entry: [esp] = engine return address, [esp+4..+23] = from (netadr_t, 20 bytes), [esp+24] = msg
push dword [esp+24]   ; x6: copies msg, then from, last dword first   FF 74 24 18
call filter           ; qboolean filter(netadr_t from, msg_t* msg)     E8 rel32
add  esp, 24          ;                                                83 C4 18
test eax, eax         ;                                                85 C0
jnz  pass             ;                                                75 01
ret                   ; drop the packet                                C3
pass:
jmp  trampoline       ; the original runs with no proxy frame          E9 rel32
```

Each `push dword [esp+24]` moves `esp` down by 4, so the same displacement reaches the next lower dword: the six pushes copy the dwords at entry offsets 24, 20, 16, 12, 8 and 4. `netadr_t` is 20 bytes in the SDK ([sys_public.hpp:51-59](../../src/sdk/sys/sys_public.hpp#L51-L59)) and in the source ([`qcommon/qcommon.h:123-130`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/qcommon.h#L123-L130)).

Sketch (x86 only):

```cpp
// The filter gets its own copy of the arguments: under cdecl a callee may overwrite its argument slots,
// and here those slots are the engine's return address and the arguments the trampoline reads next.
static_assert(sizeof(netadr_t) == 20, "the stub copies 24 bytes of arguments");

static constexpr std::size_t filterStubLength = 42;

unsigned char* MakeFilterStub(void* filter, void* trampoline)
{
	unsigned char* stub = HookUtils::AllocateMemory(filterStubLength - 5); // AllocateMemory adds 5 bytes

	if (!stub)
	{
		return nullptr;
	}

	unsigned char* p = stub;

	for (int i = 0; i < 6; ++i)
	{
		*p++ = 0xFF; *p++ = 0x74; *p++ = 0x24; *p++ = 0x18;                                       // push dword [esp+24]
	}

	*p++ = 0xE8; *(int32_t*)p = (int32_t)((intptr_t)filter - (intptr_t)(p + 4)); p += 4;          // call filter
	*p++ = 0x83; *p++ = 0xC4; *p++ = 0x18;                                                        // add esp, 24
	*p++ = 0x85; *p++ = 0xC0;                                                                     // test eax, eax
	*p++ = 0x75; *p++ = 0x01;                                                                     // jnz +1
	*p++ = 0xC3;                                                                                  // ret
	*p++ = 0xE9; *(int32_t*)p = (int32_t)((intptr_t)trampoline - (intptr_t)(p + 4)); p += 4;      // jmp trampoline

	return stub;
}
```

Size: 24 + 5 + 3 + 2 + 2 + 1 + 5 = 42 bytes. Free it with `ReleaseMemory(stub, filterStubLength - 5)`, which matches the `munmap` length on Linux.

How to install it, with the existing `HookUtils`:

1. Build the stub with a placeholder trampoline.
2. Set it as `proxyFunctionPtr` of the `SV_PacketEvent` hook entry and call `Attach`. `Attach` creates the trampoline and writes `jmp stub` at the entry of `SV_PacketEvent`.
3. Write the real trampoline (`Original_SV_PacketEvent`) into the stub's last `rel32`.
4. In `GAME_SHUTDOWN`, `Detach` as today, then free the stub.

`Detach` during `GAME_SHUTDOWN` is safe: neither the stub nor the trampoline leaves a return address on the stack. `call filter` has already returned, `jmp` pushes nothing, and the trampoline jumps back into the original function.

Stack alignment: the stub moves `esp` by 28 bytes compared with today's direct jump. MSVC on x86 only assumes 4-byte alignment, and the Linux build uses `-mstackrealign` for C++ ([CMakeLists.txt:226-229](../../CMakeLists.txt#L226-L229)). No problem expected.

#### The filter

One filter only, at the entry of `SV_PacketEvent`. It reads out-of-band packets, takes every drop decision the proxy takes today, then lets the original code run.

```cpp
// Called from the stub with a copy of the arguments. Returns 0 to drop the packet.
static qboolean QDECL Proxy_SV_PacketEvent_Filter(netadr_t from, msg_t* msg)
{
	// Sequenced game packet: nothing to filter
	if (msg->cursize < 4 || *(int*)msg->data != -1)
	{
		return qtrue;
	}

	// connect is Huffman-compressed and the original decompresses it in place: leave it alone
	if (!Q_strncmp("connect", (const char*)&msg->data[4], 7))
	{
		return qtrue;
	}

	// Same reading as the original SV_ConnectionlessPacket, which reads and tokenizes again afterwards
	server.common.functions.MSG_BeginReadingOOB(msg);
	server.common.functions.MSG_ReadLong(msg); // skip the -1 marker
	server.common.functions.Cmd_TokenizeString(server.common.functions.MSG_ReadStringLine(msg));

	const char* c = server.common.functions.Cmd_Argv(0);

	if (!Q_stricmp(c, "getstatus") || !Q_stricmp(c, "getinfo"))
	{
		// q3infoboom: Info_SetValueForKey drops the server when the challenge is too long
		if (std::strlen(server.common.functions.Cmd_Argv(1)) > 128)
		{
			return qfalse;
		}

		return Proxy_StatusAllowed(from); // F6
	}

	if (!Q_stricmp(c, "rcon"))
	{
		return Proxy_RconAllowed(); // F6, plus the writeconfig check if F7 is not done with a hook
	}

	// The original answers only the old authorize server, but prints a line for every other sender
	if (!Q_stricmp(c, "ipAuthorize"))
	{
		return qfalse;
	}

	return qtrue;
}
```

What moves where:

| Current hook | Check | After the fix |
| --- | --- | --- |
| `Proxy_SVC_Status` | q3infoboom: drop if `strlen(Cmd_Argv(1)) > 128` | In the filter, `getstatus` |
| `Proxy_SVC_Info` | q3infoboom: same check | In the filter, `getinfo` |
| `Proxy_SVC_RemoteCommand` | 500 ms rcon cooldown (`proxy_sv_enableRconCmdCooldown`) | Replaced by the F6 rate limiting |
| `Proxy_SVC_RemoteCommand` | `writeconfig` only on `.cfg` | Hook on `Com_WriteConfig_f` (F7), or a fixed check in the filter |
| `Proxy_SV_ConnectionlessPacket` | `ipAuthorize` ignored | In the filter |
| `Proxy_SV_PacketEvent` | None: identical to the jediacademy source | Nothing to move |

What it lets you delete:

- The rewrites of `SV_PacketEvent` and `SV_ConnectionlessPacket`, the `SVC_RemoteCommand` wrapper, and the `SVC_Status`/`SVC_Info` hooks: about 180 lines in [Proxy_sv_main.cpp](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_main.cpp#L89-L272), and the matching entries in [Proxy_Engine_Patch.cpp](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Patch.cpp#L14-L44).
- The cvar `proxy_sv_enableRconCmdCooldown`, replaced by F6.

What must stay:

- The inline patches on `SVC_RemoteCommand`: the timer NOP and the `SV_OUTPUTBUF_LENGTH` byte. They patch the original code, which still runs.
- The `Com_EndRedirect` call in `GAME_SHUTDOWN` ([Proxy_Main.cpp:111-115](../../src/JKA_YBEProxy/Proxy_Main.cpp#L111-L115)). Correction: it does not become useless, only its comment is wrong.
  - The original `SVC_RemoteCommand` does call `Com_EndRedirect`, but only once `Cmd_ExecuteString` returns, that is after the whole map load ([`server/sv_main.cpp:490-533`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_main.cpp#L490-L533)).
  - While a redirect is active, the original `Com_Printf` returns early, without the console or the log file ([`qcommon/common.cpp`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/common.cpp), `Com_Printf`). The proxy's `Com_Printf` does the same.
  - Ending the redirect in `GAME_SHUTDOWN` sends the rest of the map load output to the server console and log, instead of the rcon client. The later `Com_EndRedirect` of the original then does nothing, because `rd_flush` is already `NULL`.
  - Fix the comment, not the code.

#### Checks on the retail binaries: done

Both checks were done on the retail servers, read with capstone, nothing executed: `jampDed.exe` (`v1.0.1.0 win-x86 Oct 30 2003`, SHA-1 `fae27a9ddb2595b08c30ee95cb8373348aecc4bd`) and `linuxjampded` (`v1.0.1.1 linux-i386 Nov 10 2003`, SHA-1 `e7d084d5e36c16d7334df93a9e52afec1e094339`). The design works as is.

1. **Calls on the packet path: no inlining**, on both platforms.
   - `SV_PacketEvent` calls `SV_ConnectionlessPacket` and `SV_ExecuteClientMessage` with real `call`s.
   - `SV_ConnectionlessPacket` calls `SVC_Status`, `SVC_Info`, `SVC_RemoteCommand`, `SV_GetChallenge`, `SV_DirectConnect` and `SV_AuthorizeIpPacket` with real `call`s.
   - The two inline patches (timer NOP at `0x443ced` / `0x8056b26`, output buffer length at `0x443de8` / `0x8056c7d`) are inside the standalone `SVC_RemoteCommand`, the one that runs. On Windows, `Com_EndRedirect` is inlined at the end of `SVC_RemoteCommand`, which is why the proxy has its own `Proxy_Windows_Com_EndRedirect`.
2. **First bytes of `SV_PacketEvent`: no relative instruction.**
   - Windows, 8 bytes copied: `push ecx; push ebx; push ebp; push esi; mov esi, [esp+0x28]`. The `[esp+0x28]` is `msg` after four pushes, which confirms the stack layout the stub assumes: return address, then the 20-byte `netadr_t`, then `msg`.
   - Linux, 6 bytes copied: `push ebp; mov ebp, esp; sub esp, 0x18`.
   - Both end with plain `ret`, never `ret N`: cdecl, as the stub assumes.

#### Pitfalls

- The filter must return `qboolean` or `int`, not `bool`. A `bool` only sets AL, while `test eax, eax` reads the whole of EAX.
- The stub only fits cdecl targets. It clobbers EAX, ECX and EDX, which are caller-saved under cdecl. Declare the filter `QDECL` (`__cdecl` on Windows, [q_platform.hpp:37](../../src/sdk/qcommon/q_platform.hpp#L37)).
- Do not call `Huff_Decompress` on a `connect` packet: the original decompresses it again, in place. Work on a copy if you need the userinfo.
- Do not modify `msg->data`. Reading it is safe: the original calls `MSG_BeginReadingOOB` and tokenizes again.

### Replacing the DLL file for a hot reload

Corrected:

- Windows ([`win32/win_main.cpp:762-872`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/win32/win_main.cpp#L762-L872)):
  - The DLL does not have to be in a pk3. `Sys_UnpackDLL` extracts it only when the engine finds it inside a pk3. A loose DLL is loaded as is. On a pure server, the engine only reads files from pk3s, so there it must be in a pk3.
  - When it is in a pk3, the engine rewrites the extracted file on every load (`FS_FOpenFileWrite`, under `fs_homepath`). Replacing the extracted DLL on disk has no effect: replace the pk3. That has not been tested while the server runs.
  - `LoadLibrary` first tries the bare file name, which uses the Windows DLL search order (the executable's directory first), then `fs_basepath/fs_game`.
- Linux ([`unix/unix_main.c`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/unix/unix_main.c), `Sys_LoadDll`): the engine loads the `.so` from `fs_basepath/fs_game`. Replace the file with `mv` (new inode). **Never `cp` over a loaded `.so`: the running server crashes with SIGBUS.**

### Alternative clean design

A pinned loader that holds the hooks and trampolines, plus a reloadable logic DLL. This matches the todo item "Create a plugin system", but it is more work. See A6 in [improvements.md](improvements.md).

### Applied

The recommended fix: the filter stub at the entry of `SV_PacketEvent`, with `Proxy_SV_PacketEvent_Filter`. The crash was never reproduced, so the patch notes call it a likely fix, not confirmed.

## F2. Userinfo not sanitised on connect (High)

Status: **source only**, every point checked in the jediacademy source. The forcepowers crash was never reproduced on a server.

- `SV_DirectConnect` saves the userinfo ([`server/sv_client.cpp:517`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp#L517)), calls `GAME_CLIENT_CONNECT` (line 520), and only then calls `SV_UserinfoChanged` (line 530).
- The game's `ClientConnect` calls `ClientUserinfoChanged` directly, without going through `vmMain` ([`game/g_client.c:2347`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_client.c#L2347)).
- `WP_InitForcePowers` reads the userinfo with `trap_GetUserinfo` ([`game/w_force.c:274`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/w_force.c#L274)). It runs from `ClientBegin` ([`game/g_client.c:2496`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_client.c#L2496)) and `ClientSpawn` (line 3076).
- The forcepowers crash is on the server. `BG_LegalizedForcePowers` ([`game/bg_misc.c:439`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/bg_misc.c#L439)) fills `final_Powers[]` only for the digits present in the string. It then uses every entry, including the uninitialised ones, as an index in `bgForcePowerCost[i][countDown]` (lines 545-563). A short string, such as `7-1-`, reads out of bounds. This is the likely crash the proxy fixes.
- Reconnects on a map change go through `GAME_CLIENT_CONNECT` with `firstTime = qfalse`: `SV_SpawnServer` ([`server/sv_init.cpp:700`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_init.cpp#L700)) and `SV_MapRestart_f` ([`server/sv_ccmds.cpp:326`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_ccmds.cpp#L326)).
- New: `proxy.clientData` is zeroed on every map change, because the proxy DLL is reloaded. `cleanName` is therefore empty for every client after each map change, not only after the first connection.

Two engines fixed the same order:

- JK2MV, which also keeps the retail game module, calls `SV_UserinfoChanged` once before `GAME_CLIENT_CONNECT` and once after (JK2MV [`src/server/sv_client.cpp:300-319`](https://github.com/mvdevs/jk2mv/blob/7d601454c3db68492289d4d4e3dc30bff39e4246/src/server/sv_client.cpp#L300-L319)). Its forcepowers check in `SV_UserinfoChanged` (lines 1088-1136) is the one the proxy uses.
- Quake3e calls `SV_UserinfoChanged` before `GAME_CLIENT_CONNECT` ([`code/server/sv_client.c:785-792`](https://github.com/ec-/Quake3e/blob/3173cef096dd0cb2fea26d91bce71049061f5a13/code/server/sv_client.c#L785-L792)), and skips bots in `SV_UserinfoChanged`.

ioq3 and OpenJK keep the original order.

Consequence: the game module receives the raw userinfo from the connect packet, and a malicious client never has to send a `userinfo` command afterwards. These proxy fixes are bypassed by setting the bad value before connecting:

- the forcepowers crash fix. In the source, a bad `forcepowers` set before connecting reaches `BG_LegalizedForcePowers`. No crash was reproduced: the bug is an out-of-bounds read, which only crashes when it reaches unmapped memory;
- the model length fix. `Proxy_SV_UserinfoChanged` runs after `GAME_CLIENT_CONNECT`, and its comment ([Proxy_sv_client.cpp:464-466](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_client.cpp#L464-L466)) describes the connection order backwards;
- name cleaning. Anti-cheat messages print an empty name.

### Fix

1. Move the body of `Proxy_SharedAPI_ClientUserinfoChanged` ([Proxy_SharedAPI.cpp:232-339](../../src/JKA_YBEProxy/Proxy_SharedAPI.cpp#L232-L339)) into one function:

   ```cpp
   // Cleans the userinfo of a client in place.
   // Returns nullptr, or the reason to reject the client (anti-cheat). Never drops the client itself.
   const char* Proxy_SharedAPI_SanitizeUserinfo(int clientNum);
   ```

   It does `GetUserinfo`, the name, the model (with the F3 fix) and `forcepowers`, then `SetUserinfo`. On a darksidetools model it returns a static string and stops: no `SetUserinfo`.

   It must also close the bypass described in O8 of [other-engines.md](other-engines.md): refuse a userinfo with a key present twice (case-insensitive), and refuse the client when an `Info_SetValueForKey` fails. Otherwise a long userinfo keeps the bad value.

2. Userinfo change path (`GAME_CLIENT_USERINFO_CHANGED` in [Proxy_Main.cpp:203-209](../../src/JKA_YBEProxy/Proxy_Main.cpp#L203-L209), and `Proxy_OpenJKAPI_ClientUserinfoChanged`):

   ```cpp
   const char* reason = Proxy_SharedAPI_SanitizeUserinfo((int)arg0);

   if (reason)
   {
   	proxy.trap->SendServerCommand(-1, va("chat \"^3(Anti-Cheat system) ^7%s^3 got kicked cause of cheating^7\"", proxy.clientData[arg0].cleanName));
   	proxy.trap->DropClient((int)arg0, reason);

   	return 0; // do not forward to the game (F3)
   }
   ```

3. Connect path (`GAME_CLIENT_CONNECT` in [Proxy_Main.cpp:156-162](../../src/JKA_YBEProxy/Proxy_Main.cpp#L156-L162), and `Proxy_OpenJKAPI_ClientConnect`), for both values of `firstTime`, before forwarding:

   ```cpp
   if (!isBot)
   {
   	const char* reason = Proxy_SharedAPI_SanitizeUserinfo(clientNum);

   	if (reason)
   	{
   		return (intptr_t)reason; // the engine refuses the client, as with the game's "Banned."
   	}
   }
   ```

   Then send the welcome message, as today.

4. Fix the comment at [Proxy_sv_client.cpp:464-466](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_client.cpp#L464-L466). On connect, `SV_UserinfoChanged` runs after the game.

Why `DropClient` must not be called during `GAME_CLIENT_CONNECT`:

- During that call, the slot is still `CS_FREE`. The engine sets `CS_CONNECTED` after the call returns ([`server/sv_client.cpp:544`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp#L544)).
- `SV_DropClient` would call `GAME_CLIENT_DISCONNECT` inside `GAME_CLIENT_CONNECT`, then the engine would finish the connection anyway.
- Returning a string is the game's own way to refuse a client (`"Banned."`):
  - first connection: the engine prints the reason to the client and stops ([`server/sv_client.cpp:520-528`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp#L520-L528));
  - map change: the engine drops the client with the reason ([`server/sv_init.cpp:700-703`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_init.cpp#L700-L703)).
- A string literal is fine. For a DLL, `VM_ExplicitArgPtr` returns the pointer unchanged ([`qcommon/vm.cpp:742-758`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/vm.cpp#L742-L758)). `currentVM` is still set after the call, because `VM_Call` only restores a non-null previous value ([`qcommon/vm.cpp:826-827`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/vm.cpp#L826-L827)).

Why bots are skipped:

- Their userinfo comes from the bot files, and the game never puts a `forcepowers` key in it (none in [`game/g_bot.c`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_bot.c)). The sanitiser would add the default string for nothing.
- `WP_InitForcePowers` takes a bot's powers from `botstates[]` anyway ([`game/w_force.c:279-282`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/w_force.c#L279-L282)).
- Skipping bots keeps today's behaviour for them. Quake3e also skips bots.

Side effects:

- Gameplay: none. Same sanitisation, only earlier. A client with a malformed `forcepowers` string gets the default configuration on its first spawn, instead of whatever the game makes of the raw string.
- Reconnect (`firstTime = qfalse`): the userinfo kept by the engine is sanitised again. That is idempotent, and it refills `cleanName` after the map change.
- OpenJK: same change in `Proxy_OpenJKAPI_ClientConnect`, which returns a `char*`. Not needed if F4 option A is chosen.

### Applied

Steps 1 to 4, with one part left out:

- The O8 checks of step 1 (a key present twice, a failed `Info_SetValueForKey`) are not in the sanitiser. They are left to O8, where the author chose to port OpenJK's whole `G_ValidateUserinfo`.
- O1 (`ip` key) is not done either.
- The OpenJK API change was removed with that API path, by F4 option A.

## F3. False positive and bypass in the darksidetools anti-cheat (Critical)

Status: **confirmed** for the comparison bugs (proxy code and [q_string.cpp](../../src/sdk/qcommon/q_string.cpp)). The ghost configstring is **source only**, and was inferred before: now checked.

[Proxy_SharedAPI.cpp:263](../../src/JKA_YBEProxy/Proxy_SharedAPI.cpp#L263) does `Q_stricmpn(val, "darksidetools", len)` with `len = strlen(model)`:

- An empty or missing model gives `len == 0`, and `Q_stricmpn(..., 0)` returns 0. `Info_ValueForKey` returns `""` for a missing key. The player is kicked, with a public "cheating" message.
- Any prefix matches: `d`, `da`, `dark`… are kicked.
- `darksidetools/default` is not detected: 21 characters are compared, and the strings differ at the `/`.
- After `DropClient`, the function keeps going. Checked path:
  1. `SV_DropClient` calls `GAME_CLIENT_DISCONNECT` ([`server/sv_client.cpp:640`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp#L640)). The game's `ClientDisconnect` clears `CS_PLAYERS + clientNum` ([`game/g_client.c:3929`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_client.c#L3929)).
  2. `SV_DropClient` then empties the userinfo ([`server/sv_client.cpp:650`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp#L650)).
  3. The proxy calls `SetUserinfo` and writes the userinfo back.
  4. `vmMain` forwards `GAME_CLIENT_USERINFO_CHANGED`. The game's `ClientUserinfoChanged` does not check that the client is connected, and calls `trap_SetConfigstring(CS_PLAYERS + clientNum, ...)` again ([`game/g_client.c:2215`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_client.c#L2215)).
  5. A ghost player configstring stays for a dropped client, until someone else takes the slot.

### Fix

What the checks match today, because they compare `strlen(model)` characters, that is the model as a prefix of the name:

| Check | Matched today | Missed today |
| --- | --- | --- |
| `darksidetools` (kick) | `darksidetools`, and any start of it: `d`, `dark`, the empty model | `darksidetools/default`, `darksidetoolsXYZ` |
| `rancor`, `wampa` (kyle) | `rancor`, any start of it (`r`, `ra`…), the empty model | `rancor/default`, `wampa/default`, any skin |
| `jedi_/red`, `jedi_/blue` (kyle) | `jedi_`, `jedi_/`, `jedi_/red`, `jedi_/blue`, and the starts in between (`jedi_/r`…) | `jedi_/green`, any other skin |

Fix:

- `darksidetools`: kick every model that starts with `darksidetools` (decision of the author): `!Q_stricmpn(val, "darksidetools", 13)`.
- For the others, accepted by the author although it is wider than today:
  - `rancor` and `wampa`: every model that starts with the name, skins included, as TaystJK does (`!Q_stricmpn(val, "rancor", 6)`, [`codemp/server/sv_client.cpp:1290-1295`](https://github.com/taysta/TaystJK/blob/7d99efb4a77a174d1a7853f2a8024daf78fa47b7/codemp/server/sv_client.cpp#L1290-L1295)). Today `rancor/default` gets through.
  - `jedi_` without a species: `jedi_` alone or with any skin, `!Q_stricmp(val, "jedi_") || !Q_stricmpn(val, "jedi_/", 6)`. Today only the `red` and `blue` skins are caught. A real customisable model always has a species (`jedi_hm`, `jedi_tf`…), so this does not touch valid models.
  - An empty model: kyle, explicitly. Today it already ends up as kyle, through the prefix bug of the `rancor` check, right after the darksidetools kick.
- After the drop, return the reason. Do not call `SetUserinfo`, and do not forward to the game (see F2, step 2).

Side effects: a player with an empty model, or a model that is only the start of one of these names, is no longer kicked.

### Applied

As written above, with the decisions of the author: every model that starts with `darksidetools` is kicked, and the wider `rancor`, `wampa` and `jedi_` rule is used.

## F4. OpenJK API path is broken (High)

Decision: OpenJK must stay supported, including an OpenJK `jampgame` on an OpenJK server (`linuxjampded`, OpenJK build).

Status: **confirmed** in the proxy code. On the OpenJK side, checked in the OpenJK repository:

- `GetModuleAPI` returning `NULL` gives `Com_Error(ERR_FATAL, "GetGameAPI failed on %s")` ([`codemp/server/sv_gameapi.cpp:3122-3128`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/server/sv_gameapi.cpp#L3122-L3128)).
- The OpenJK game keeps the `import` pointer (`trap = import`, [`codemp/game/g_main.c:3574`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_main.c#L3574)), so changing the copy after `GetModuleAPI` works. That holds for OpenJK's own game only. Another mod could copy the table.

The problems:

- `InitGame` is not wrapped, so the `GAME_INIT` logic never runs and no proxy cvar is registered. They all stay at 0:
  - `proxy_sv_modelPathLength = 0`, so `len >= 0` is always true: **every model is forced to kyle** on each userinfo change.
  - `proxy_sv_maxCallVoteMapRestartValue = 0`: every `callvote map_restart N` with N > 0 is refused.
  - No startup banner or log (todo item "Add log in server console when using OpenJK API").
- The `RunFrame` wrapper ([Proxy_OpenJKAPI_Wrappers.cpp:74-77](../../src/JKA_YBEProxy/Wrappers/Proxy_OpenJKAPI_Wrappers.cpp#L74-L77)) updates neither `svsTime` nor the cvars:
  - `/myratio` never gets past its cooldown, because `svsTime` stays 0 ([Proxy_ClientCommand.cpp:47](../../src/JKA_YBEProxy/Proxy_ClientCommand.cpp#L47)). This is the cause of the todo item "Fix `/myratio` on OpenJK API".
  - Cvar changes are ignored.
- `GetUsercmd` ([Proxy_OpenJKAPI_Wrappers.cpp:9-14](../../src/JKA_YBEProxy/Wrappers/Proxy_OpenJKAPI_Wrappers.cpp#L9-L14)) sanitises before the engine fills `cmd`, so it does nothing. The original API path does it in the right order ([Proxy_OriginalAPI_Wrappers.cpp:35-45](../../src/JKA_YBEProxy/Wrappers/Proxy_OriginalAPI_Wrappers.cpp#L35-L45)).
- Fallback when the retail jampgame has no `GetModuleAPI` ([Proxy_Main.cpp:248-257](../../src/JKA_YBEProxy/Proxy_Main.cpp#L248-L257)): it calls `Proxy_GetOriginalGameAPI`, then returns `nullptr`, so OpenJK stops with `ERR_FATAL`. The message the proxy prints just before is misleading.
- The userinfo is not sanitised in `ClientConnect` (see F2).
- The welcome message never reaches the client. The proxy sends it during `GAME_CLIENT_CONNECT`, while the slot is still `CS_FREE`, and OpenJK's `SV_AddServerCommand` drops every command to a client below `CS_PRIMED` ([`codemp/server/sv_main.cpp:124-126`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/server/sv_main.cpp#L124-L126)). The retail engine has no such check. This is an engine difference, not an API one: both options below leave it as is.

### Fix

Two options.

**Option A (recommended): stop exporting `GetModuleAPI`.** Checked in OpenJK:

- `Sys_LoadGameDll` unloads a DLL that has no `GetModuleAPI` and returns `NULL` ([`shared/sys/sys_main.cpp`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/shared/sys/sys_main.cpp), `Sys_LoadGameDll`).
- `SV_BindGame` then falls back to the legacy API, `VM_CreateLegacy` ([`codemp/server/sv_gameapi.cpp:3134-3139`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/server/sv_gameapi.cpp#L3134-L3139)). It loads the proxy through `dllEntry`/`vmMain`, the same path as the original engine.
- From there, the proxy loads the retail jampgame through `dllEntry`/`vmMain`, as it does today on the original engine. An OpenJK jampgame also exports `vmMain` and `dllEntry`, and its legacy path is complete ([`codemp/game/g_main.c:3637`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_main.c#L3637), [`codemp/game/g_syscalls.c:31-35`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_syscalls.c#L31-L35), `TranslateSyscalls`).

Result:

- Every F4 bug disappears. The cvars, `GAME_RUN_FRAME`, `/myratio` and the usercmd fix run on OpenJK through code that already works on the original engine.
- Both OpenJK cases work: OpenJK engine with the retail jampgame, and OpenJK engine with an OpenJK jampgame. Checked: the legacy path of the OpenJK game (`TranslateSyscalls` in `codemp/game/g_syscalls.c`) only uses syscalls of the retail `G_*` enum (171 of its 189), and the proxy forwards every syscall it does not handle. On a 64-bit OpenJK server, the proxy and the game must be 64-bit builds; the retail jampgame only exists in 32-bit.
- A2 (bridge) is no longer needed: OpenJK with the retail jampgame works.
- Deleted: `GetModuleAPI` in [Proxy_Main.cpp](../../src/JKA_YBEProxy/Proxy_Main.cpp#L225-L274), [Proxy_OpenJKAPI_Wrappers.cpp](../../src/JKA_YBEProxy/Wrappers/Proxy_OpenJKAPI_Wrappers.cpp), and the `*OpenJKAPI*` fields of `proxy`.
- Cost: each call goes through OpenJK's legacy `GVM_*` wrappers, one more `switch`. Negligible.
- To test on a real OpenJK server: loading, cvars, `/myratio`, a map change. `/netStatus` stays disabled on OpenJK (`isOriginalEngine` is false).

Welcome message, with either option: set a flag in `GAME_CLIENT_CONNECT` when `firstTime && !isBot`, and send the message in `GAME_CLIENT_BEGIN`, where the client is `CS_ACTIVE` on both engines. `proxy.clientData` is zeroed on every map change, and the reconnect has `firstTime = qfalse`, so the message is sent once per connection, as today.

**Option B: keep the API path and fix it.**

1. Wrap `InitGame`: banner, `Proxy_CVars_Registration()`, then call the original.
2. Wrap `RunFrame`: `Proxy_UpdateAllCvars()`, `proxy.proxyData.svsTime = levelTime`, then call the original. OpenJK passes `svs.time`, as `GAME_RUN_FRAME` does.
3. `GetUsercmd`: call the engine first, then `Proxy_SharedAPI_GetUsercmd`.
4. `ClientConnect`: F2.
5. Fill the import copy before calling the original `GetModuleAPI` (move [Proxy_Main.cpp:270-271](../../src/JKA_YBEProxy/Proxy_Main.cpp#L270-L271) before line 266). Then it no longer depends on how the game stores the pointer.
6. Retail jampgame on OpenJK: A2 bridge, or at least a clear `trap->Error` message instead of `Proxy_GetOriginalGameAPI` plus `nullptr`.

### Applied

Option A: the proxy no longer exports `GetModuleAPI`, and the OpenJK API code is deleted. The welcome message is now sent in `GAME_CLIENT_BEGIN`, as described above.

## F5. Wrong cvars read (High)

Status: **source only**, checked.

- The engine's C variable `sv_gametype` points to the cvar `"g_gametype"` ([`server/sv_init.cpp:821`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_init.cpp#L821)).
- `Cvar_Get` ORs the flags into an existing cvar: `var->flags |= flags` ([`qcommon/cvar.cpp`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/cvar.cpp), `Cvar_Get`).
- The engine registers `sv_fps` with `CVAR_TEMP`.

[Proxy_CVars.cpp:20-21](../../src/JKA_YBEProxy/Proxy_CVars.cpp#L20-L21):

- `sv_gametype` does not exist as a cvar. The proxy creates a bogus cvar `sv_gametype`, set to 0, with `CVAR_SERVERINFO`:
  - it shows up in the serverinfo;
  - the gametype check in `/myratio` always sees `GT_FFA`, so it always passes.
- `sv_fps` is registered with `CVAR_SERVERINFO`, which `Cvar_Get` ORs into the engine's cvar: `sv_fps` shows up in the serverinfo.

### Fix

```cpp
{ &proxy.cvars.sv_fps, "sv_fps", "20", 0, 0, qfalse },
{ &proxy.cvars.sv_gametype, "g_gametype", "0", 0, 0, qfalse }
```

Side effects of registering `g_gametype` from the proxy, checked:

- Flags 0: `|=` adds nothing.
- Default value `"0"`: same as the engine and the game ([`game/g_main.c:246`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_main.c#L246)), so no warning.
- Latch: `Cvar_Get` applies a pending latched value. The proxy registers its cvars in `GAME_INIT` before forwarding to the game, and the game registers `g_gametype` in the same call, then clamps it ([`game/g_main.c:824-841`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_main.c#L824-L841)). The latched value is applied a few instructions earlier, in the same call. No visible change.
- `/myratio` reads the right gametype, updated about once per second like the other proxy cvars.

Things to know:

- Flags already ORed stay until the server restarts: `sv_fps` and the bogus `sv_gametype` stay in the serverinfo of a running server.
- Decision of the author: the proxy only needs to read these two values, so flags 0 for both. `sv_fps` leaves the serverinfo (after a server restart).

### Applied

As written above: `g_gametype` and `sv_fps`, both with flags 0.

## F6. Rcon without rate limiting, getstatus/getinfo amplification (High)

Status: **source only** for the original engine. **Confirmed** for ioq3, Quake3e and OpenJK (local sources).

- Original `SVC_RemoteCommand` ([`server/sv_main.cpp:490-533`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_main.cpp#L490-L533)):
  - a global 500 ms timer (`Com_Milliseconds`), before the password check;
  - `Bad rcon` only with `developer 1` (`Com_DPrintf`);
  - it always replies `Bad rconpassword.` to the sender.
- No limit at all on `getstatus`/`getinfo`.

Today:

- By default the engine timer is NOPed ([Proxy_Engine_Patch.cpp:97-98](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Patch.cpp#L97-L98)) and `proxy_sv_enableRconCmdCooldown` is 0: the rcon password can be brute-forced with no limit.
- With the cooldown on, it is global and updated **before** the password check ([Proxy_sv_main.cpp:128-134](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_main.cpp#L128-L134)). Anyone can lock the admins out: the "rcon disabler" again.
- `getstatus`/`getinfo`: the server can be used as a DRDoS amplifier.

Constraint: attackers can spoof source IPs, so every packet can come from a different address. Admins must keep a working rcon during such a flood. The one thing a spoofing attacker cannot fake is the password.

### What the other engines do

| Engine | rcon | getstatus / getinfo | Bucket table |
| --- | --- | --- | --- |
| ioq3 ([`code/server/sv_main.c`](https://github.com/ioquake/ioq3/blob/87a99d18206e878a20289f05ac3c16005dcae5c3/code/server/sv_main.c)) | Per address `(10, 1000)` for **every** rcon packet, then one global bucket `(10, 1000)` for bad passwords | Per address `(10, 1000)`, then global outbound `(10, 100)` | Fixed: 16384 buckets, 1024 hashes. Linear scan of the table for each new address. Full table: limited. |
| Quake3e ([`code/server/sv_main.c:357-640`](https://github.com/ec-/Quake3e/blob/3173cef096dd0cb2fea26d91bce71049061f5a13/code/server/sv_main.c#L357-L640)) | Same as ioq3 | Same as ioq3, also on `getchallenge` and `connect` ([`sv_client.c:130, 502`](https://github.com/ec-/Quake3e/blob/3173cef096dd0cb2fea26d91bce71049061f5a13/code/server/sv_client.c#L130)) | Same table, but the scan starts where the last one stopped. Non-IP addresses are never limited. |
| OpenJK ([`codemp/server/sv_main.cpp:326-408, 698-731`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/server/sv_main.cpp#L326-L408)) | No limit of its own | One per-address bucket and one global bucket on **every** out-of-band packet (`sv_maxOOBRateIP 1`, `sv_maxOOBRate 1000`) | `std::map` keyed by the IPv4 address, garbage-collected once per second |
| JK2MV ([`src/server/sv_main.cpp:320-390, 727-800`](https://github.com/mvdevs/jk2mv/blob/7d601454c3db68492289d4d4e3dc30bff39e4246/src/server/sv_main.cpp#L320-L390)) | No limit of its own | Per address `(10, 1000)` on **every** out-of-band packet, then one global bucket per command type (`sv_maxOOBRate`), doubled for whitelisted addresses | `std::map`, as OpenJK |

All of them use `Sys_Milliseconds`, not `svs.time`.

ioq3 and Quake3e check the per-address bucket before the password, and OpenJK and JK2MV limit every packet. A flood spoofing the admin's own address therefore blocks the admin too. The design below avoids that.

### Fix

- **Valid password: never limited.** Admins keep rcon during any flood, even when the attacker spoofs the admin's own address.
- **Bad password: one global bucket `(10, 1000)`**, bursts of 10, then one per second. That is the ioq3 value.
  - A per-address bucket adds nothing here: under spoofing, every guess comes from a new address. The global bucket caps guesses at about one per second, whatever the addresses.
  - When the bucket is empty, drop the packet silently. That also caps the `Bad rconpassword.` replies sent to spoofed addresses.
  - An attacker who sees no reply can still detect a hit through a visible side effect, for example `rcon <guess> sv_hostname X`, then `getstatus` from a real address. The global bucket is what limits that.
- **getstatus / getinfo: per address `(10, 1000)`, then a global outbound bucket `(10, 100)`**, the ioq3 values.
  - In a reflection attack the spoofed address is the victim's, always the same one: the per-address bucket protects the victim.
  - The global bucket caps the total. During a flood the server may briefly disappear from server browsers. Players in game are not affected.
- Time source: `proxy.trap->Milliseconds()` (`G_MILLISECONDS`, which is `Sys_Milliseconds`). Not `svs.time`: it only moves once per server frame, so every packet of one frame sees the same time.
- Bucket table, only needed for getstatus/getinfo: port OpenJK's `std::map` version. It has no fixed size and no linear scan. Add a size cap, for example 65536 entries, and limit new addresses beyond it. Entries live for `burst * period` = 10 s, so a spoofed flood of N packets per second keeps about 10 × N entries.
- The buckets live in the proxy DLL, so they reset on every map change. Harmless.
- Keep the engine's 500 ms timer removed (inline NOP patch). Delete `proxy_sv_enableRconCmdCooldown`.
- Optional, as Quake3e does: a per-address limit on `getchallenge`. A `getchallenge` flood can push legitimate challenges out of `svs.challenges`.

Sketch:

```cpp
// From OpenJK codemp/server/sv_main.cpp (SVC_RateLimit), GPL
struct leakyBucket_t
{
	int lastTime;
	int burst;
};

// Returns true when the request must be dropped
static bool Proxy_RateLimit(leakyBucket_t* bucket, int burst, int period, int now)
{
	const int interval = now - bucket->lastTime;
	const int expired = interval / period;
	const int expiredRemainder = interval % period;

	if (expired > bucket->burst || interval < 0)
	{
		bucket->burst = 0;
		bucket->lastTime = now;
	}
	else
	{
		bucket->burst -= expired;
		bucket->lastTime = now - expiredRemainder;
	}

	if (bucket->burst < burst)
	{
		bucket->burst++;

		return false;
	}

	return true;
}

// Valid password: never limited, so admins keep rcon during a spoofed flood.
// Bad password: one global bucket, about one guess per second whatever the source addresses.
static qboolean Proxy_RconAllowed(void)
{
	const char* password = server.cvars.sv_rconPassword->string;

	if (password[0] && !std::strcmp(server.common.functions.Cmd_Argv(1), password))
	{
		return qtrue;
	}

	static leakyBucket_t badPasswordBucket;

	if (Proxy_RateLimit(&badPasswordBucket, 10, 1000, proxy.trap->Milliseconds()))
	{
		return qfalse; // drop silently
	}

	return qtrue; // the original replies "Bad rconpassword."
}
```

`Proxy_StatusAllowed(from)` is the same with `Proxy_RateLimit` on the address's bucket `(10, 1000)`, then on a static outbound bucket `(10, 100)`.

Implement all of it in the F1 filter. This covers the todo item "rate limiter (anti ddos)".

Side effects:

- Admins with the right password: no change.
- An admin who mistypes during a flood may get no reply.
- Players in game: no change. Only out-of-band packets are limited.

### Applied

The fix above, in the F1 filter, with these choices:

- Bucket table: a modified version of OpenJK's `std::map`, in [Proxy_Engine_RateLimit.cpp](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_RateLimit.cpp). It is capped at 65536 addresses, and a new address beyond the cap is dropped. Times are unsigned, so the buckets stay right when `Sys_Milliseconds` wraps after about 24.8 days. Only `NA_IP` addresses are limited, as Quake3e.
- The optional `getchallenge` limit is done, and `connect` gets it too, as Quake3e. It is per address only, and shares the `getstatus`/`getinfo` buckets.
- Each dropped request is printed only with `developer 1`, as ioq3 and Quake3e.
- `proxy_sv_enableRconCmdCooldown` is deleted.

Rejected:

- A per-address limit on every out-of-band packet, as OpenJK and JK2MV do: a flood that spoofs the admin's address would block the admin's rcon.
- The global outbound bucket on `getchallenge`: a `getstatus` flood would empty it and block every connection.
- JK2MV's periodic summary of the drops.

The stateless challenges seen while comparing the engines are written up as A9 in [improvements.md](improvements.md). They are optional.

## F7. `writeconfig` filter can be bypassed (Medium)

Status: **source only**, checked.

- `Com_WriteConfig_f` calls `COM_DefaultExtension(filename, ..., ".cfg")` ([`qcommon/common.cpp:1515-1527`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/common.cpp#L1515-L1527)). It adds `.cfg` only when the name has no `.` after its last `/` ([`game/q_shared.c:112-131`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/q_shared.c#L112-L131)).
- `FS_FOpenFileWrite` writes to `fs_homepath/fs_game/<name>` with no check on the extension or on `..` ([`qcommon/files.cpp:864-897`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/files.cpp#L864-L897)).

[Proxy_sv_main.cpp:137-144](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_main.cpp#L137-L144) only blocks a name that is at least 5 characters long and has a 3-letter extension other than `.cfg`:

- `x.so`, `x.js` or `jampgamei386.so` get through.
- It can also be bypassed with `vstr` or `exec`: the command then runs later from the Cbuf.
- It requires rcon, but filtering rcon arguments is the wrong layer.

Corrected impact:

- Linux: the engine loads the game `.so` from `fs_basepath` ([`unix/unix_main.c`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/unix/unix_main.c), `Sys_LoadDll`), and `writeconfig` writes to `fs_homepath`, which is `~/.ja` by default. Only when both point to the same directory does `rcon writeconfig jampgamei386.so` hit the loaded proxy. Then `fopen("wb")` truncates a mapped file, and the server can crash right away (SIGBUS, as with `cp` over a loaded `.so`), not only at the next map.
- Any platform: any file type can be created or overwritten under `fs_homepath/fs_game`, for example a `.pk3`, and `..` reaches other directories.

### What the other engines do

- ioq3 ([`code/qcommon/common.c`](https://github.com/ioquake/ioq3/blob/87a99d18206e878a20289f05ac3c16005dcae5c3/code/qcommon/common.c), `Com_WriteConfig_f`): `.cfg` only, with `COM_CompareExtension`. Its file system also refuses to write `.dll`/`.so`/`.qvm`/`.pk3` (`FS_CheckFilenameIsMutable`, `ERR_FATAL`).
- OpenJK ([`codemp/qcommon/common.cpp:1383-1408`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/qcommon/common.cpp#L1383-L1408)): `.cfg` only, plus the reserved names `mpdefault.cfg` and `default.cfg`.
- Quake3e ([`code/qcommon/common.c:4303-4322`](https://github.com/ec-/Quake3e/blob/3173cef096dd0cb2fea26d91bce71049061f5a13/code/qcommon/common.c#L4303-L4322)): a denylist of extensions (`dll`, `exe`, `so`, `dylib`, `qvm`, `pk3`, `.so.N`), so it still allows `.txt`.
- JK2MV ([`src/qcommon/common.cpp:2750-2762`](https://github.com/mvdevs/jk2mv/blob/7d601454c3db68492289d4d4e3dc30bff39e4246/src/qcommon/common.cpp#L2750-L2762)): never refuses, forces the extension instead. `COM_SanitizeExtension` appends `.cfg` to any name that does not already end with it, so `x.so` becomes `x.so.cfg` ([`src/qcommon/q_shared.cpp:121-140`](https://github.com/mvdevs/jk2mv/blob/7d601454c3db68492289d4d4e3dc30bff39e4246/src/qcommon/q_shared.cpp#L121-L140)).

### Fix

Hook `Com_WriteConfig_f` and copy the OpenJK version (refuse), or the JK2MV one (force `.cfg`). Refusing is clearer for the admin. Add a `..` check with the proxy's existing `FS_CheckDirTraversal` (already used by `Proxy_SV_BeginDownload_f`):

```cpp
void (*Original_Com_WriteConfig_f)(void);
void Proxy_Com_WriteConfig_f(void)
{
	if (*server.common.vars.cmd_argc == 2)
	{
		char filename[MAX_QPATH];

		Q_strncpyz(filename, server.common.functions.Cmd_Argv(1), sizeof(filename));
		COM_DefaultExtension(filename, sizeof(filename), ".cfg");

		if (!COM_CompareExtension(filename, ".cfg") || FS_CheckDirTraversal(filename)
			|| !Q_stricmp(filename, "mpdefault.cfg") || !Q_stricmp(filename, "default.cfg"))
		{
			server.common.functions.Com_Printf("writeconfig: only a .cfg file name, without '..', is allowed\n");

			return;
		}
	}

	Original_Com_WriteConfig_f(); // also prints the usage when the argument count is wrong
}
```

- `COM_DefaultExtension` and `COM_CompareExtension` are declared in [q_shared.hpp:606-607](../../src/sdk/qcommon/q_shared.hpp#L606-L607) but not defined in the repository. Port them from OpenJK [`codemp/qcommon/q_shared.c:130-170`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/qcommon/q_shared.c#L130-L170).
- It also covers `vstr`, `exec` and the server console, because the check sits in the command itself.
- It is safe for F1: `writeconfig` never reloads the VM, so this hook is a leaf.
- Address of `Com_WriteConfig_f`, found in the retail binaries (see F1 for the files): `0x410680` on Windows, `0x8074d64` on Linux. Both are registered by `Cmd_AddCommand("writeconfig", ...)` (at `0x411507` and `0x8074b55`), and both print `Usage: writeconfig <filename>`. Add them to [Proxy_Engine_Wrappers.hpp](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Wrappers.hpp) as `{ 0x410680, 0x8074d64 }`.
- **Windows needed A1 first** (done, `Attach` now relocates a copied `e8`/`e9 rel32`). The Windows function starts with `sub esp, 0x40` (3 bytes) then `call Cmd_Argc` (`e8 rel32`). The trampoline copies these 8 bytes as they are, so the copied `call` would jump to a wrong address. `GetTramp` must relocate a copied `e8`/`e9 rel32` before this hook can be added (see A1). Linux starts with `push ebp; mov ebp, esp; sub esp, 0x50` (6 bytes): no problem.
- Then delete the check in `Proxy_SVC_RemoteCommand`.

Fallback without a new address: put the same test in the F1 filter, on `Cmd_Argv(3)` when `Cmd_Argv(2)` is `writeconfig`, and accept that `vstr`/`exec` bypass it.

### Applied

The OpenJK behaviour (refuse), with the `..` check and the reserved names, done without A1: `Proxy_Com_WriteConfig_f` rewrites the whole function and calls `Com_WriteConfigToFile` itself (`0x410630` / `0x8074cb4`, read on both binaries: a cdecl function with one argument, which starts with `FS_FOpenFileWrite`). The trampoline, whose Windows copy holds the unrelocated `call Cmd_Argc`, is never run. `Detach` copies the original bytes back from it unchanged, so unpatching stays correct.

## F8. Unbounded index in `SV_SvEntityForGentity` (Low, was Medium)

Status: **confirmed** in the proxy code. The risk is corrected: theoretical with the retail game.

- [Proxy_sv_game.cpp:16-26](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_game.cpp#L16-L26) validates `s.number`, but indexes with `num`, computed by pointer arithmetic and never bounds-checked.
- The jediacademy, OpenJK and ioq3 originals all index with `s.number` ([`server/sv_game.cpp:70-75`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_game.cpp#L70-L75)).
- Origin, confirmed by the author: JK2MV's `mv_fixplayerghosting` fix, which uses the same pointer arithmetic against the "player ghosting" bug where players can walk through an affected player (JK2MV [`src/server/sv_game.cpp:34-41, 71-90`](https://github.com/mvdevs/jk2mv/blob/7d601454c3db68492289d4d4e3dc30bff39e4246/src/server/sv_game.cpp#L34-L41), [`CVARS.rst`](https://github.com/mvdevs/jk2mv/blob/7d601454c3db68492289d4d4e3dc30bff39e4246/CVARS.rst)). The fix keeps it and only adds the bounds checks that JK2MV has.

Corrected risk:

- Out of bounds only if the game passes an entity outside `g_entities`. The game passes `&g_entities[n]` everywhere in the source.
- Division by zero only if `SV_SvEntityForGentity` runs before `LocateGameData`. The game calls `trap_LocateGameData` early in `G_InitGame`, before it spawns entities.

### Fix

Take JK2MV's version: the index from the address, a fallback to `s.number` when the address is not inside `g_entities`, then the bounds check.

```cpp
static int Proxy_SV_NumForGentity(sharedEntity_t* ent)
{
	const std::ptrdiff_t offset = (byte*)ent - (byte*)server.sv->gentities;

	if (server.sv->gentitySize <= 0 || offset < 0 || offset % server.sv->gentitySize)
	{
		return -1;
	}

	return (int)(offset / server.sv->gentitySize);
}

svEntity_t* Proxy_SV_SvEntityForGentity(sharedEntity_t* gEnt)
{
	if (!gEnt)
	{
		Com_Error(ERR_DROP, "SV_SvEntityForGentity: null gEnt\n");
	}

	int num = Proxy_SV_NumForGentity(gEnt);

	// Not an element of g_entities: fall back to s.number, as JK2MV does
	if ((unsigned)num >= (unsigned)MAX_GENTITIES)
	{
		num = gEnt->s.number;
	}

	if ((unsigned)num >= (unsigned)MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "SV_SvEntityForGentity: bad gEnt\n");
	}

	return &server.sv->svEntities[num];
}
```

The alignment test (`offset % gentitySize`) is an addition: JK2MV does not have it.

### Applied

As written above, alignment test included.

## F9. `netStatus` modifies engine state (Medium)

Status: **source only**, checked. The impact is **corrected**: it is a netcode change for a real player. The fix is **corrected** too: the first version left `Proxy_SV_UserMove` unchanged, and a human with a ping of 0 would then show wrong `fps` and `packets` values.

- Without `proxy_sv_pingFix`, `messageSent` and `messageAcked` both use `svs.time` ([`server/sv_snapshot.cpp:667`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_snapshot.cpp#L667), [`server/sv_client.cpp:1715`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp#L1715)), which only changes once per server frame. A client whose round trip is shorter than one frame (50 ms at `sv_fps 20`) shows a ping of 0.
- [Proxy_Engine_ClientCommand.cpp:57-64](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_ClientCommand.cpp#L57-L64) treats any client with a ping below 1 as a bot, and sets `cl->snapshotMsec = 1`.
- `snapshotMsec` caps the snapshot rate ([`server/sv_snapshot.cpp:681-692`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_snapshot.cpp#L681-L692)). It is normally at least 33 ms, because `snaps` is clamped to 1-30 ([`server/sv_client.cpp:1488-1497`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp#L1488-L1497)).
- With `snapshotMsec = 1`, that player gets a snapshot every server frame, limited only by `rate`, instead of their `snaps` value. That is a gameplay "feeling" change, triggered by a display command, and it lasts until the player's next userinfo change.

### Fix

Two changes.

1. Do not write to `cl`. In [Proxy_Engine_ClientCommand.cpp:57-73](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_ClientCommand.cpp#L57-L73):

   ```cpp
   // Bots have no snapshot rate: avoid the division by zero without touching the engine state
   const int snapshotMsec = cl->snapshotMsec > 0 ? cl->snapshotMsec : 1;

   if (cl->netchan.remoteAddress.type != NA_BOT)
   {
   	Proxy_Engine_Client_CalcPacketsAndFPS(i, &packets, &fps);
   }

   snaps = std::min(1000 / snapshotMsec, server.cvars.sv_fps->integer);
   ```

   `NA_BOT` is the test `Proxy_SV_Status_f` already uses.

2. Record the usercmd statistics of a human with a ping of 0. `Proxy_SV_UserMove` skips every client with a ping below 1 ([Proxy_sv_client.cpp:183-187](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_client.cpp#L183-L187)), so it never fills `cmdStats` for such a human. Delete that test:

   ```cpp
   // Proxy -------------->
   if (client->ping < 1)
   {
   	continue;
   }
   ```

   - The test only ever skips humans. Bots never reach `SV_UserMove`: their usercmds go through `BOTLIB_USER_COMMAND` straight to `SV_ClientThink` ([`server/sv_game.cpp:977-979`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_game.cpp#L977-L979)). `SV_UserMove` is only called for network packets ([`server/sv_client.cpp:1845-1847`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp#L1845-L1847)).
   - Without this change, step 1 is wrong for a human whose ping was always 0: `Proxy_Engine_Client_CalcPacketsAndFPS` counts every empty entry of `cmdStats`, and shows `fps 1024` (`CMD_MASK`) and `packets 0`.

With both changes, a human with a ping of 0 gets real `fps` and `packets` values.

### Applied

Both changes, as written above.

## F10. Low-priority bugs

Status per point:

- **F10.1. Refuted.** [Proxy_CVars.cpp:148](../../src/JKA_YBEProxy/Proxy_CVars.cpp#L148), `frame == sv_fps`: "if `sv_fps` is lowered below the frame counter, the cvars are never updated again".
  - `proxy.cvars.sv_fps` is a vmCvar. It only changes inside `Proxy_UpdateCvars`, which runs when `frame == sv_fps`, and then `frame` restarts from 1.
  - The new value is at least 1: the engine resets an `sv_fps` below 1 to 10 at the start of `SV_Frame` ([`server/sv_main.cpp:847-848`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_main.cpp#L847-L848)), before `GAME_RUN_FRAME`.
  - The counter always reaches the new value. No fix needed.
  - Applied: nothing.
- **F10.2. Source only.** [Proxy_Engine_Client.cpp:111](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Client.cpp#L111): `1000 / svFps` divides by zero if `sv_fps` is set to 0.
  - The engine resets it only at the start of the next `SV_Frame`, and the packets of the same event loop are processed before that.
  - Only an rcon user can set `sv_fps 0`.
  - Fix: `+ (svFps > 0 ? 1000 / svFps : 0)`, or return early when `svFps <= 0`.
  - Applied: the early return. The usercmd data is kept and used at the next call.
- **F10.3. Corrected.** [Proxy_SharedAPI.cpp:46-52](../../src/JKA_YBEProxy/Proxy_SharedAPI.cpp#L46-L52): `ClientDisconnect` only resets the client data if `isConnected` is set, and that flag is only set in `ClientBegin`.
  - Only `cleanName` can be stale. The usercmd statistics and the timers are only written after `ClientBegin`: `SV_UserMove` enters the world (`GAME_CLIENT_BEGIN`) before it processes the first usercmd.
  - The next client in the slot gets a new `cleanName` on its first userinfo change, or on connect once F2 is done.
  - Fix: drop the `isConnected` test and always reset.
  - Applied: as written.
- **F10.4. Confirmed, harmless today.** [Proxy_SharedAPI.cpp:87-133](../../src/JKA_YBEProxy/Proxy_SharedAPI.cpp#L87-L133): command names are compared by prefix.
  - `"team",4` also matches `teamvote`, `"say",3` already covers `say_team`, and `"gc",2` matches anything that starts with `gc`.
  - Each over-matched command also needs the argument condition (`follow1`, a client number…). `teamvote follow1` is dropped, but it means nothing to the game anyway.
  - The arguments are compared by prefix too. `!Q_stricmpn(cmd_arg2, "saber_droid", 6)` ([Proxy_SharedAPI.cpp:113](../../src/JKA_YBEProxy/Proxy_SharedAPI.cpp#L113)) only compares `saber_`, so `npc spawn` is refused for every NPC type that starts with `saber_`, not only `saber_droid`. Whether base has other `saber_*` NPC types was not checked.
  - The game compares all these names whole, with `Q_stricmp`: the commands (`game/g_cmds.c`, `ClientCommand`), `spawn` ([`game/NPC_spawn.c:4197`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/NPC_spawn.c#L4197)), the NPC type ([`game/NPC_stats.c:1123`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/NPC_stats.c#L1123)), `follow1`/`follow2` ([`game/g_cmds.c:771-775`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_cmds.c#L771-L775)) and the vote names ([`game/g_cmds.c:2015-2023`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_cmds.c#L2015-L2023)). An exact comparison in the proxy therefore blocks everything the game would act on, and nothing more.
  - Fix: `Q_stricmp` on the command names and on these arguments, except `jkaDST_`, which must stay a prefix.
  - Applied: as written.
- **F10.5. Refuted.** [Proxy_sv_ccmds.cpp:71](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_ccmds.cpp#L71), `gametypeNames[sv_gametype->integer]`: "the clamp only sets the latched value".
  - `trap_Cvar_Set` goes through `Cvar_Set`, which is `Cvar_Set2(..., force = qtrue)` ([`server/sv_game.cpp:535-537`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_game.cpp#L535-L537), [`qcommon/cvar.cpp:402-404`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/cvar.cpp#L402-L404)). That skips the latch and sets the value at once.
  - `G_InitGame` clamps `g_gametype` to `[0, GT_MAX_GAME_TYPE)` right after registering it ([`game/g_main.c:824-841`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_main.c#L824-L841)), and `gametypeNames` has `GT_MAX_GAME_TYPE` (10) entries.
  - `rcon g_gametype 99` is latched, then clamped at the next `GAME_INIT`, before `status` can run.
  - A bound check costs one line and the code comment already asks for it. Optional.
  - Applied: the optional bound check.
- **F10.6. Corrected.** [Proxy_Files.cpp:24-38](../../src/JKA_YBEProxy/Proxy_Files.cpp#L24-L38):
  - The `memcpy` is not bounded. Windows only: `MAX_OSPATH` is 256 there, and `PATH_MAX` (4096) on Linux. `fs_game` has `CVAR_INIT`, so only the command line can set it.
  - The path is relative to the current directory instead of `fs_basepath`/`fs_homepath`.
  - A failure calls `exit()`.
  - Fix: build the path with `Com_sprintf` into a `MAX_OSPATH` buffer, from `fs_basepath`, and fail with `proxy.trap->Error(...)` (`G_ERROR`, an `ERR_DROP`) instead of `exit()`.
  - Applied: the bounded path from `fs_basepath`, and the error now prints the full path. `exit()` is kept, by decision of the author:
    - the engine itself stops (`ERR_FATAL`) when it cannot load a game module;
    - an `ERR_DROP` version was tried and reverted: the server stayed up with no map, and rcon did not work, because `Com_EventLoop` only passes packets to the server while `com_sv_running` is set.
- **F10.7. Corrected: intended, one edge case.** [Proxy_ClientCommand.cpp:13-20](../../src/JKA_YBEProxy/Proxy_ClientCommand.cpp#L13-L20): `ratioString` uses `kill - death` in the green branch and `kill - (death - suicides)` in the red one.
  - Intent, per the author (with screenshots): "Kills" is `PERS_SCORE`, which loses 1 point per suicide, and "Deaths" (`PERS_KILLED`) counts the suicide too ([`game/g_combat.c:2571-2630`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_combat.c#L2571-L2630)). The color comes from `kill - death`. A player who is behind only because of a suicide shows `0` in red, not `-1`.
    - 1 kill: Kills 1, Deaths 0 gives `+1` in green.
    - 1 kill, then 1 suicide: Kills 0, Deaths 1, Suicides 1 gives `0` in red.
    - 2 kills, then 1 suicide: Kills 1, Deaths 1, Suicides 1 gives `+0` in green.
  - Edge case: the red branch can show a positive number. 4 kills, 2 suicides, then 1 death by another player gives Kills 2, Deaths 3, Suicides 2: `kill - death` is -1, so red, but the number is `2 - (3 - 2)` = `1`.
  - Fix: keep both formulas, and never show more than 0 in red.

    ```cpp
    // Red branch: a suicide already removed a kill, so it is not counted again as a death,
    // but a player who is behind never shows a positive number
    Com_sprintf(ratioString, sizeRatioString, "(" S_COLOR_RED "%i" S_COLOR_WHITE ")", std::min(0, kill - (death - suicides)));
    ```

  - Applied differently, by decision of the author: `kill - death` in both colors, and no clamp. The number then matches the Kills and Deaths shown and the ratio, which uses the same two values: Kills 2, Deaths 3, Suicides 2 shows `-1` in red. One kill then one suicide now shows `-1` in red instead of `0`.

- **F10.8. Corrected: `== -1` is intended.** [Proxy_sv_main.cpp:50](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_main.cpp#L50): `SV_CalcPings` skips `messageAcked == -1` instead of `<= 0` ([`server/sv_main.cpp:684`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_main.cpp#L684)).
  - Origin: `proxy_sv_pingFix` is a port of JK2MV's `sv_pingFix`. JK2MV changed `<= 0` to `== -1` on purpose, in commit [`0301bea`](https://github.com/mvdevs/jk2mv/commit/0301bea) "Fix sv_pingFix on servers running for more than 25 days" (JK2MV [`src/server/sv_main.cpp:946`](https://github.com/mvdevs/jk2mv/blob/7d601454c3db68492289d4d4e3dc30bff39e4246/src/server/sv_main.cpp#L946)).
  - With `proxy_sv_pingFix 1`, the timestamps come from `Sys_Milliseconds`, a signed `int` that turns negative after about 24.8 days of uptime. `<= 0` would then skip every frame, and every ping would show 999. Do not revert it.
  - What remains: a frame not acknowledged yet holds -1 ([`server/sv_snapshot.cpp:668`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_snapshot.cpp#L668)), but a frame never used since the client was cleared on connect holds 0, and counts as a delay of 0. That only lasts for the first 32 snapshots (`PACKET_BACKUP`), about 1.6 s at 20 snapshots per second, just after a connect. JK2MV has the same transient. Quake3e avoids it by using 0 as the "not acknowledged" marker ([`code/server/sv_snapshot.c:694`](https://github.com/ec-/Quake3e/blob/3173cef096dd0cb2fea26d91bce71049061f5a13/code/server/sv_snapshot.c#L694), [`sv_main.c:1064`](https://github.com/ec-/Quake3e/blob/3173cef096dd0cb2fea26d91bce71049061f5a13/code/server/sv_main.c#L1064)).
  - Optional fix: also skip frames that were never sent, `messageSent == 0 && messageAcked == 0`. Not worth much.
  - Applied: the optional fix, `== -1` kept. The transient also happened with `proxy_sv_pingFix 0`: the proxy's ping calculation always replaces the engine's, whose `<= 0` skipped these frames.

## F11. `dumpuser` crashes on a long userinfo value (Medium)

Status: **confirmed**, observed on the retail server on 2026-10-24 while testing O8.

- `Info_Print` copies each userinfo value into `char value[512]` with no bound, and each key into `char key[512]` ([`qcommon/common.cpp:509-543`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/common.cpp#L509-L543)). A value of 512 characters or more overflows the stack buffer.
- `SV_DumpUser_f` is its only server-side caller ([`server/sv_ccmds.cpp:938`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_ccmds.cpp#L938)). On the client side it reads `Cvar_InfoString`, bounded by the cvar system, so only the server path is exposed.
- A client sets such a value with the `userinfo` command. `Proxy_SV_UpdateUserinfo_f` stores the raw string into `cl->userinfo` up to `MAX_INFO_STRING` (1024), with no per-value limit ([Proxy_sv_client.cpp:652](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_client.cpp#L652)). The connect packet does the same in `SV_DirectConnect`.
- Trigger: an admin `rcon dumpuser <name>`. The value comes from an unprivileged client, so a client plants it and the server dies on the next routine `dumpuser`.
- O8's `G_ValidateUserinfo` does not close it: a 790-character value in an otherwise valid userinfo under 1024 characters passes every O8 check (size, slash count, key counts). A legitimate userinfo never has a value that long: `name` is 32, `model` about 64, the saber names and colours all well under 512.
- OpenJK and ioq3 size these buffers `BIG_INFO_VALUE` (8192), which holds any value from a `MAX_INFO_STRING` userinfo (OpenJK [`codemp/qcommon/common.cpp:490-492`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/qcommon/common.cpp#L490)).

### Fix

Two options, not exclusive.

1. In the F2/O8 sanitiser, refuse a userinfo that has any value of `MAX_INFO_VALUE` or more, or simply of 512 or more. It reuses the drop path of F2 and O8, needs no new hook, and closes both the connect and `userinfo` command paths before the bad value is stored long enough to matter. It does not fix `Info_Print` itself.
2. Hook `Info_Print` and copy the OpenJK version with `BIG_INFO_VALUE` buffers. This fixes the root cause, so `dumpuser` is safe whatever the userinfo. It needs the retail address of `Info_Print`, to find in the binaries as in F7, and A1's relocation if its first bytes hold a relative instruction.

Recommended: option 1 with the F2/O8 work, and option 2 if a hook on `Info_Print` is wanted for defence in depth.

### Applied

Option 2 only, by decision of the author. Option 1 was written, then removed.

- Every engine that changed `Info_Print` only enlarged its buffers: ioq3 [`21668c06`](https://github.com/ioquake/ioq3/commit/21668c06) "Fix systeminfo command" (2011), ported to OpenJK in [`8733bddb`](https://github.com/JACoders/OpenJK/commit/8733bddb), and Quake3e rewrote it with `Info_NextPair`. JK2MV still has 512.
- So the ioq3 fix was made for `systeminfo`, not `dumpuser`. In the retail engine, `serverinfo`, `systeminfo` and `dumpuser` all print through `Info_Print` ([`server/sv_ccmds.cpp:890, 906, 938`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_ccmds.cpp#L890)). With `sv_pure 1` and many pk3 files, `sv_paks` or `sv_pakNames` can reach 512 characters. That case was not reproduced.
- Option 2 fixes all three commands, and drops no client. Option 1 only covered `dumpuser`, and on OpenJK it would drop clients for a crash that does not exist there.

What was done:

- `Proxy_Info_Print` rewrites the whole function from the retail source. Only the buffer sizes change, to `BIG_INFO_KEY` and `BIG_INFO_VALUE`. The output formats are the retail ones (`"%s"`, not OpenJK's `"%s "`). The original is never called.
- Addresses: `0x410ce0` on Windows, `0x8073504` on Linux, found from the `MISSING VALUE` string.
- Checked on both binaries:
  - exactly three callers, `serverinfo`, `systeminfo` and `dumpuser`;
  - cdecl, one argument on the stack;
  - no jump or pointer to the function;
  - no relative instruction in the first bytes (Windows `sub esp, 0x400`, Linux `push ebp; mov ebp, esp; sub esp, 0x420`).
- The strings printed are at most `MAX_INFO_STRING`: `cl->userinfo`, and `Cvar_InfoString` for the two others (not `Cvar_InfoString_Big`, also checked on the binaries).
