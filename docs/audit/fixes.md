# Fixes

Severity: Critical, High, Medium, Low. Paths are relative to the repository root.

## F1. Crash on `rcon map` and `rcon map_restart` (Critical)

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

The original DLL would crash the same way, even on a vanilla engine, if it triggered a map change synchronously, for example with `trap_SendConsoleCommand(EXEC_NOW, "map ...")` from `ClientCommand`. Vote map changes use `EXEC_APPEND`: the command goes into the Cbuf and runs later from `Com_Frame`, when no game frame is on the stack.

Not verified: that basejka never uses `EXEC_NOW` for a command that reloads the VM.

### Rejected option: pinning the DLL

- Windows: `GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&vmMain, &h)`.
- Linux: `dladdr` to find the module path, then `dlopen(path, RTLD_NOW | RTLD_NOLOAD | RTLD_NODELETE)`.

This fixes the crash, but the DLL is then never unloaded. A new proxy build would only load after a full server restart, and every global would survive map changes. Rejected because hot reload through a map change is wanted.

### Recommended fix: filter hook with a tail jump

Rule: no proxy frame may be on the stack while original engine code runs. In the packet path, replace the "wrapper that calls the original" hooks with a "filter, then `jmp`" stub:

```
stub (15 bytes, allocated with the trampoline):
  call filter        ; filter(engineRet, from, msg) -> qboolean
  test eax, eax
  jnz  pass
  ret                ; drop the packet
pass:
  jmp  trampoline    ; the original runs with no proxy frame on the stack
```

```cpp
// engineRet is ignored: it lines the arguments up with the stack as seen on entry to SV_PacketEvent
qboolean Proxy_SV_PacketEvent_Filter(void* engineRet, netadr_t from, msg_t* msg);
```

Sketch of the stub generator (x86 only, not compiled):

```cpp
unsigned char* MakeFilterStub(void* filter, void* trampoline)
{
	unsigned char* stub = AllocateMemory(10); // AllocateMemory adds 5 bytes, so 15 bytes in total

	stub[0] = 0xE8; *(int32_t*)(stub + 1) = (int32_t)((intptr_t)filter - (intptr_t)(stub + 5));          // call filter
	stub[5] = 0x85; stub[6] = 0xC0;                                                                      // test eax, eax
	stub[7] = 0x75; stub[8] = 0x01;                                                                      // jnz +1
	stub[9] = 0xC3;                                                                                      // ret
	stub[10] = 0xE9; *(int32_t*)(stub + 11) = (int32_t)((intptr_t)trampoline - (intptr_t)(stub + 15));   // jmp trampoline

	return stub;
}
```

The detour at the `SV_PacketEvent` entry then jumps to the stub instead of a C++ function.

What the filter does:

- One filter only, at the entry of `SV_PacketEvent` (a global cdecl function).
- It reads out-of-band packets and takes every drop decision the proxy takes today:
  - rcon rate limiting per IP (see F6);
  - `writeconfig` filtering (see F7, though hooking `Com_WriteConfig_f` is better);
  - the q3infoboom check on `getstatus`/`getinfo` (`strlen(Cmd_Argv(1)) > 128`);
  - dropping `ipAuthorize`.
- Otherwise it lets the original code run.

What it lets you delete:

- The rewrites of `SV_PacketEvent` and `SV_ConnectionlessPacket`, the `SVC_RemoteCommand` wrapper, and the `SVC_Status`/`SVC_Info` hooks: about 200 lines in [Proxy_sv_main.cpp](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_main.cpp#L89-L272) and the matching entries in [Proxy_Engine_Patch.cpp](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Patch.cpp#L14-L44).
- This also resolves the README note about `static` functions: no static function is hooked in this path anymore.
- The `Com_EndRedirect` hack in `GAME_SHUTDOWN` ([Proxy_Main.cpp:111-115](../../src/JKA_YBEProxy/Proxy_Main.cpp#L111-L115)) should become useless, because the original `SVC_RemoteCommand` calls `Com_EndRedirect` once `Cmd_ExecuteString` returns. Retest before removing it.
- Keep the inline patches (timer NOP and `SV_OUTPUTBUF_LENGTH` byte): they patch the original code, which still runs.

Pitfalls:

- The filter must return `qboolean`/`int`, not `bool`. A `bool` only sets AL, while `test eax, eax` reads the whole of EAX.
- The stub only fits cdecl or stdcall targets. It clobbers EAX/ECX/EDX, which are caller-saved under cdecl. Do not use it on functions that pass arguments in registers. A stdcall target needs `ret N` instead of `ret`.
- Do not call `Huff_Decompress` in place on a `connect` packet: the original decompresses it again. Work on a copy if you need the userinfo (which could also serve F2).
- Reading the packet in the filter is safe: the original calls `MSG_BeginReadingOOB` and tokenizes again. Do not modify `msg->data`.
- Check in the disassembly that the original `SV_PacketEvent` calls `SV_ExecuteClientMessage` with a real `call` and does not inline it. Otherwise the existing `SV_ExecuteClientMessage` hook stops firing. That may be why `SV_PacketEvent` was rewritten in the first place.
- `Detach` frees the stub and the trampoline during `GAME_SHUTDOWN`. That is safe: neither leaves a return address on the stack (`call filter` has already returned, `jmp` pushes nothing, and the trampoline jumps back into the original function).

### Replacing the DLL file for a hot reload

- Windows: a loaded DLL cannot be overwritten, but it can be renamed. Rename the old file, copy the new one, then change map.
- Linux: replace the file with `mv` (new inode). **Never `cp` over a loaded `.so`: the running server crashes with SIGBUS.**

### Alternative clean design

A pinned loader that holds the hooks and trampolines, plus a reloadable logic DLL. This matches the todo item "Create a plugin system", but it is more work. See A6 in [improvements.md](improvements.md).

## F2. Userinfo not sanitised on connect (Critical)

Checked in jediacademy `SV_DirectConnect`: the engine saves the userinfo, then calls `VM_Call(gvm, GAME_CLIENT_CONNECT, ...)`, and only after that calls `SV_UserinfoChanged(newcl)`. The game's `ClientConnect` also calls `ClientUserinfoChanged` internally, without going through `vmMain`.

Consequence: the game module receives the raw userinfo from the connect packet. These proxy fixes can therefore be bypassed by setting the bad value before connecting:

- the forcepowers crash fix;
- the model length fix. `Proxy_SV_UserinfoChanged` runs after `GAME_CLIENT_CONNECT`, and its comment ([Proxy_sv_client.cpp:464-466](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_client.cpp#L464-L466)) describes the order backwards;
- name cleaning. `cleanName` stays empty until the first userinfo change, so anti-cheat messages print an empty name.

The same probably happens when clients reconnect on a map change (`SV_SpawnServer` calls `GAME_CLIENT_CONNECT` with `firstTime = qfalse`). Not checked in the jediacademy source.

Fix:

1. Move the sanitisation out of `Proxy_SharedAPI_ClientUserinfoChanged` ([Proxy_SharedAPI.cpp:232-339](../../src/JKA_YBEProxy/Proxy_SharedAPI.cpp#L232-L339)) into its own function (GetUserinfo, sanitise, SetUserinfo).
2. Call it in `GAME_CLIENT_CONNECT` ([Proxy_Main.cpp:156-162](../../src/JKA_YBEProxy/Proxy_Main.cpp#L156-L162)) and in `Proxy_OpenJKAPI_ClientConnect`, before forwarding.
3. Keep calling it in `ClientUserinfoChanged`.

## F3. False positive and bypass in the darksidetools anti-cheat (Critical)

[Proxy_SharedAPI.cpp:263](../../src/JKA_YBEProxy/Proxy_SharedAPI.cpp#L263) does `Q_stricmpn(val, "darksidetools", len)` with `len = strlen(model)`:

- An empty or missing model gives `len == 0`, and `Q_stricmpn(..., 0)` returns 0. The player is kicked, with a public "cheating" message. A plain `/model ""` triggers it.
- Any prefix matches: `d`, `da`, `dark`… are kicked.
- `darksidetools/default` is not detected.
- After `DropClient`, the function keeps going: it calls `SetUserinfo` on a dropped client, and `vmMain` still forwards `GAME_CLIENT_USERINFO_CHANGED` to the game. That risks a ghost configstring.

Fix:

- Use `!Q_stricmpn(val, "darksidetools", 13)` (model starts with).
- Return a flag after the drop and do not forward to the game.
- The `rancor`, `wampa` and `jedi_` checks use the same prefix logic. Harmless (the model is reset to kyle), but use a proper comparison.

## F4. OpenJK API path is broken (High)

Decision: OpenJK must stay supported.

- `InitGame` is not wrapped, so the `GAME_INIT` logic never runs and no proxy cvar is registered. They all stay at 0:
  - `proxy_sv_modelPathLength = 0`, so `len >= 0` is always true: **every model is forced to kyle** on each userinfo change.
  - `proxy_sv_maxCallVoteMapRestartValue = 0`: every `callvote map_restart N` with N > 0 is refused.
  - No startup banner or log (todo item "Add log in server console when using OpenJK API").
- The `RunFrame` wrapper ([Proxy_OpenJKAPI_Wrappers.cpp:74-77](../../src/JKA_YBEProxy/Wrappers/Proxy_OpenJKAPI_Wrappers.cpp#L74-L77)) updates neither `svsTime` nor the cvars:
  - `/myratio` never gets past its cooldown. This is the cause behind the todo item "Fix `/myratio` on OpenJK API".
  - Cvar changes are ignored.
- `GetUsercmd` ([Proxy_OpenJKAPI_Wrappers.cpp:9-14](../../src/JKA_YBEProxy/Wrappers/Proxy_OpenJKAPI_Wrappers.cpp#L9-L14)) sanitises before the engine fills `cmd`, so it does nothing. Swap the order, as the original API path does.
- Fallback when the retail jampgame has no `GetModuleAPI` ([Proxy_Main.cpp:248-257](../../src/JKA_YBEProxy/Proxy_Main.cpp#L248-L257)): it calls `Proxy_GetOriginalGameAPI` and returns `nullptr`. OpenJK then does `Com_Error(ERR_FATAL, "GetGameAPI failed on %s")` (checked), and the message the proxy prints before that is misleading. At minimum, fail with a clear message. The real solution is A2 in [improvements.md](improvements.md).
- Also sanitise the userinfo in `ClientConnect` (see F2).

## F5. Wrong cvars read (High)

[Proxy_CVars.cpp:20-21](../../src/JKA_YBEProxy/Proxy_CVars.cpp#L20-L21):

- `sv_gametype` is the name of the engine's C variable. The cvar it points to is `"g_gametype"` (checked in `sv_init.cpp`). The proxy therefore creates a bogus cvar `sv_gametype` set to 0 with `CVAR_SERVERINFO`:
  - it shows up in the serverinfo;
  - the gametype check in `/myratio` always sees `GT_FFA`.
- `sv_fps` is registered with `CVAR_SERVERINFO`. `Cvar_Get` ORs the flags into the existing cvar, which the engine registers as `CVAR_TEMP`, so `sv_fps` shows up in the serverinfo.

Fix: register `"g_gametype"`, and use flags 0 for cvars the proxy only reads.

## F6. Rcon without rate limiting, getstatus/getinfo amplification (High)

- By default the engine timer is NOPed ([Proxy_Engine_Patch.cpp:97-98](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Patch.cpp#L97-L98)) and `proxy_sv_enableRconCmdCooldown` is 0. Result: brute force of the rcon password with no limit at all.
- With the cooldown on, it is global and updated **before** the password check ([Proxy_sv_main.cpp:128-134](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_main.cpp#L128-L134)). Anyone can lock the admins out. That is exactly the "rcon disabler" the option is meant to fix; the original engine timer has the same flaw.
- `getstatus`/`getinfo`: the original JKA has no throttle at all (checked), so the server can be used as a DRDoS amplifier.

Fix: port ioq3's leaky bucket.

- Per-address limit before any rcon processing: `SVC_RateLimitAddress(from, 10, 1000)`.
- A separate global bucket for bad passwords: `SVC_RateLimit(&bucket, 10, 1000)`.
- The same per-address limit on `getstatus`/`getinfo`, plus a global outbound bucket.
- Implement it in the F1 filter. This covers the todo item "rate limiter (anti ddos)".

## F7. `writeconfig` filter can be bypassed (Medium)

[Proxy_sv_main.cpp:137-144](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_main.cpp#L137-L144) only blocks when the argument has a 4-character extension other than `.cfg`.

- `x.so`, `x.js` or `jampgamei386.so` get through. `COM_DefaultExtension` does not add `.cfg` when an extension is already present. On Linux, an rcon user can therefore overwrite the game `.so` with config text, and the next map fails to load.
- It can also be bypassed with `vstr`/`exec`: the command then runs later from the Cbuf.
- It requires rcon, but filtering rcon arguments is the wrong layer.

Fix: hook `Com_WriteConfig_f` and require the `.cfg` extension there (OpenJK does `COM_CompareExtension(filename, ".cfg")`).

## F8. Unbounded index in `SV_SvEntityForGentity` (Medium)

[Proxy_sv_game.cpp:16-26](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_game.cpp#L16-L26) validates `s.number`, but indexes with `num`, computed by pointer arithmetic and never bounds-checked.

- If the game passes an entity that is not inside `g_entities`, the code writes out of bounds in `sv.svEntities`.
- If `gentitySize` is 0 (before `LocateGameData`), it divides by zero.

Fix: check `0 <= num < MAX_GENTITIES`, and check `gentitySize`.

## F9. `netStatus` modifies engine state (Medium)

[Proxy_Engine_ClientCommand.cpp:61-64](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_ClientCommand.cpp#L61-L64) sets `cl->snapshotMsec = 1` for any client with a ping below 1, not only bots. Without `proxy_sv_pingFix`, a human with a ping under 50 ms (at `sv_fps 20`) can show a ping of 0. A display command then changes that client's snapshot rate.

Fix: use a local variable.

## F10. Low-priority bugs

- [Proxy_CVars.cpp:148](../../src/JKA_YBEProxy/Proxy_CVars.cpp#L148): `frame == sv_fps`. If `sv_fps` is lowered below the current frame counter, the cvars are never updated again. Use `>=`, or a time-based check.
- [Proxy_Engine_Client.cpp:111](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Client.cpp#L111): `1000 / svFps` divides by zero if `sv_fps` is set to 0. The engine resets it in the next `SV_Frame`, but packets are processed before that.
- [Proxy_SharedAPI.cpp:46-52](../../src/JKA_YBEProxy/Proxy_SharedAPI.cpp#L46-L52): `ClientDisconnect` only resets the client data if `isConnected` is set, and that flag is only set in `ClientBegin`. A client dropped before `ClientBegin` leaves stale data (`cleanName`, `cmdStats`, timers) for the next client in the slot. Reset in `ClientConnect` when `firstTime` is true.
- [Proxy_SharedAPI.cpp:87-133](../../src/JKA_YBEProxy/Proxy_SharedAPI.cpp#L87-L133): command names are compared by prefix. `"team",4` also matches `teamtask`, `"say",3` already covers `say_team`, and `"gc",2` matches anything starting with `gc`. This over-blocks. Use `Q_stricmp` for command names.
- [Proxy_sv_ccmds.cpp:71](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_ccmds.cpp#L71): `gametypeNames[sv_gametype->integer]` is not bounded. `g_gametype` is latched, and the game's clamp through `trap_Cvar_Set` only sets the latched value, so an out-of-range value leads to an out-of-bounds read on `status`.
- [Proxy_Files.cpp:24-38](../../src/JKA_YBEProxy/Proxy_Files.cpp#L24-L38):
  - the `memcpy` is not bounded: an `fs_game` of about 240 characters or more overflows a stack buffer;
  - the path is relative to the current directory instead of `fs_basepath`/`fs_homepath`;
  - a failure calls `exit()` instead of `trap->Error`.
- [Proxy_ClientCommand.cpp:13-20](../../src/JKA_YBEProxy/Proxy_ClientCommand.cpp#L13-L20): `ratioString` uses `kill - (death - suicides)` in the negative branch but `kill - death` in the positive one.
- [Proxy_sv_main.cpp:50](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_main.cpp#L50): `SV_CalcPings` skips `messageAcked == -1` instead of `<= 0`. Frames never used yet (`messageAcked` is 0 after the memset) count as 0 ms right after a connect. Open question: is that intended?
