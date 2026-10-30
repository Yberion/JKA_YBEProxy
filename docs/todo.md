# Todo

> [!NOTE]
> Bugs and improvements found during the audit are tracked separately in [audit-2026-10-25/](audit-2026-10-25/README.md). Items below that the audit covers link to it.

## Open

### Short term

- **Create a plugin system.** See A6 in [audit-2026-10-25/improvements.md](audit-2026-10-25/improvements.md).

### Bugs

Found by reading the code of the other engines (2026-10-31), not reproduced. Most severe first.

- Original engine: memory leak of the sub-BSPs. For a sub-BSP (a `misc_bsp` entity, model `#name`), `CM_LoadMap_Actual` reads the whole file into a buffer that it never frees: only the main map goes through `gpvCachedMapDiskImage` ([`qcommon/cm_load.cpp:672-681`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/cm_load.cpp#L672-L681)). It leaks at each load of a map that has some. Which MP maps use `misc_bsp` was not checked. OpenJK [`e76263d8`](https://github.com/JACoders/OpenJK/commit/e76263d8). Possible fix: on a dedicated server the buffer of the main map is freed at the end of the load, so after `GAME_INIT` only the lost buffers are left in `TAG_BSP_DISKIMAGE`, and `Z_TagFree(TAG_BSP_DISKIMAGE)` would free them (needs the address of `Z_TagFree`).
- Ghoul2 system calls with a null pointer crash the server: the engine uses the `ghoul2` argument without a test ([`server/sv_game.cpp:1380`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_game.cpp#L1380)). Seen with an NPC without its model or animations, and with FFA changed to Siege then `map_restart`. `npc spawn` needs cheats, so otherwise it takes broken content or an admin action. OpenJK [`b244d4e3`](https://github.com/JACoders/OpenJK/commit/b244d4e3), [`41bd4d8f`](https://github.com/JACoders/OpenJK/commit/41bd4d8f), [`083ae51e`](https://github.com/JACoders/OpenJK/commit/083ae51e). Possible fix: null tests in `Proxy_OriginalAPI_VM_DllSyscall`, which every system call of the game goes through. Any engine, no engine address.
- Original engine: `sv_maxPing` can be bypassed. `SV_GetChallenge` reuses the existing challenge of an address instead of making a new one ([`server/sv_client.cpp:67-76`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp#L67-L76)). Only matters with `sv_maxPing` set (default `0`). ioq3, OpenJK [`5ee248fd`](https://github.com/JACoders/OpenJK/commit/5ee248fd).
- Original engine: a `map_restart` while a client is still loading makes it active with the last usercmd of the previous map, and the client hangs. ioq3, OpenJK [`36bc2c55`](https://github.com/JACoders/OpenJK/commit/36bc2c55).
- Original engine: a dropped bot goes to `CS_ZOMBIE` for a few seconds instead of `CS_FREE`, so its slot stays taken meanwhile. OpenJK [`93daa30d`](https://github.com/JACoders/OpenJK/commit/93daa30d).
- Original engine: `SV_SendServerCommand` to one client does not check that it is at least `CS_PRIMED` (only the broadcast does), so commands can pile up before the gamestate is sent. No realistic path to a "Server command overflow" was found. OpenJK [`c0e9b765`](https://github.com/JACoders/OpenJK/commit/c0e9b765).

### Long term

- Features from JKA_Anti-cheat: https://hackmd.io/CtIPZON0QHKuqjdxXRw8uw

## Maybe

- ffa3 exploit with movement events (a player with a high FPS lags every player in view). EternalJK fixes it in the engine. Postponed: two designs were written then removed, they are kept in O9 in [audit-2026-10-25/other-engines.md](audit-2026-10-25/other-engines.md).

## Done

### Features and fixes

- `proxyconfig` command: the version, the engine it runs on, and every proxy cvar with its value and its default, see [features.md](features.md#proxyconfig).
- Snaps policy: `sv_minSnaps` and `sv_maxSnaps` (the engine kept `snaps` between 1 and 30 whatever `sv_fps` was), and a `userSnaps` column in `/netStatus`, see [features.md](features.md#sv_minsnaps-sv_maxsnaps).
- Rate policy: `sv_minRate`, and the `userRate` and `trueRate` columns in `/netStatus`, see [features.md](features.md#sv_minrate).
- Persist bans across server restarts: the IP bans are stored in `ybeproxy_bans.txt`, up to 1024, and `ban` and `unban` take a client number, see [features.md](features.md#addip-removeip-listip).
- `BOT` instead of a ping of `0` for bots in `rcon status` and `/netStatus`.
- `YbeProxy` key with the proxy version in the serverinfo, and `proxy_sv_disableServerInfoVersion` to remove it.
- `/netStatus`: `netPing`, `gamePing`, and a `timeNudge` from a simulation of the client, see [features.md](features.md#netstatus-shownet).
- Ban system: a `0` in an IP ban is no longer a wildcard (`addip 152.0` bans `152.0.*.*` only), see `addip` in [features.md](features.md#addip-removeip-listip).
- `/rcon forceteam -1 <team>` moves everyone, caught with `GAME_CONSOLE_COMMAND`.
- `dumpuser` accepts a client number, as in ioq3, OpenJK and Quake3e.
- Mute system: `mute` and `unmute` commands, until the player leaves or the map changes. It blocks the chat commands in `Proxy_SharedAPI_ClientCommand`, not `SV_SendServerCommand`.
- Store player IPs, as OpenJK does: the proxy keeps the address of each client (without the port) from the `ip` key of its userinfo, set by the engine and kept up to date by the proxy. It is used by `proxy_sv_maxClientsPerIP`. OpenJK also uses its stored IP in the game logs, which the proxy cannot add to.
- `donedl` exploit.
- q3infoboom.
- rcon disabler, with `proxy_sv_enableRconCmdCooldown` to bring the timer back.
- Model length crash:
  - check in `ClientUserinfoChanged()`;
  - check in `SV_UserinfoChanged()`, so that connecting players cannot affect other players;
  - `proxy_sv_modelPathLength` (default `64`) to change the limit.
- High `callvote map_restart` value, with `proxy_sv_maxCallVoteMapRestartValue`.
- Negative `callvote map_restart` value.
- `writeconfig` on files other than `.cfg`.
- q3dirtrav.
- Linux startup crash.
- `cl_timenudge` estimate for "any" `sv_fps`. It was later restricted to `sv_fps` `20` and `30`, see [features.md](features.md#netstatus-shownet).
- `proxy_sv_enableNetStatus` to enable or disable `netStatus`.
- `rcon status` truncated with many players.
- **Refactor of the hook/asm utilities.**
- **Refactor of the proxy.**
- `/netstatus`.
- `/myratio`.
- Model ghost bug.
- Better `rcon status`.
- Hooks only on the original `jampDed.exe` and `linuxjampded` (`version` cvar check).
- Narrower scope for the force exploit fix.
- Load order:
  - on `GAME_INIT`: load the original game module, then patch the engine if the version matches;
  - on `GAME_SHUTDOWN`: unpatch the engine if the version matches, then unload the original game module.
- `netstatus` not working when a bot is on the server.
- Debug build not working: the `msg` parameter in `Proxy_SV_UserMove()` seemed invalid.
- Better README.
- OpenJK: the proxy no longer exports `GetModuleAPI`, so OpenJK loads it through `dllEntry` and `vmMain`. Fixes the console log, the cvars and `/myratio` on OpenJK, see F4 in [audit-2026-10-25/fixes.md](audit-2026-10-25/fixes.md).
- Rate limiter (anti-DDoS): rcon bad passwords, `getstatus` and `getinfo`, see F6 in [audit-2026-10-25/fixes.md](audit-2026-10-25/fixes.md).
- Replace the HDE length disassembler with [Zydis](https://github.com/zyantific/zydis) (git submodule) and rewrite `HookUtils`: relative `call`/`jmp` relocated, other relative instructions refused, any hook failure stops the server, see A1 in [audit-2026-10-25/improvements.md](audit-2026-10-25/improvements.md).
- Redo the CMake project, based on [cpp-best-practices/cmake_template](https://github.com/cpp-best-practices/cmake_template), with `CMakePresets.json`. The includes of the proxy code are uniform and the SDK was compared with OpenJK, see A5 in [audit-2026-10-25/improvements.md](audit-2026-10-25/improvements.md).

### Bugs

- NPC navigation data never freed: memory leaked at each map change and `map_restart`, and the nodes of the previous map stayed for the NPCs. The proxy frees it before the game loads, as OpenJK does in `CNavigator::Load`.
- Original engine: the server stayed down when a counter was about to wrap. `SV_Frame` calls `SV_Shutdown` then `map_restart 0`, but `map_restart` refuses to run once the server is down ("Server is not running."), so every player was dropped and the server had no map until someone restarted it ([`server/sv_main.cpp:866-878`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_main.cpp#L866-L878), [`server/sv_ccmds.cpp:246`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_ccmds.cpp#L246)). Found by reading the code, not reproduced. The proxy now loads the map again a little before the limit:
  - `svs.nextSnapshotEntities` (it grows by the entities of every snapshot built, and only a map change resets it, so a server that stays on one map with many players reaches it in hours): `map <current map>`, a normal map change, the players stay connected;
  - `svs.time` (after about 21.7 days, only `SV_Shutdown` resets it): `killserver` then `map <current map>`, as ioq3 and OpenJK. The players are dropped.
- `rcon map mp/ffa3` (any map) and `rcon map_restart 0` sometimes crashed the server since the hooked `SVC_RemoteCommand` was added, more often with Discord running on the server machine. Proxy hook functions were still on the call stack when the engine unloaded the proxy. The packet checks now run in a filter at the entry of `SV_PacketEvent`, which returns before the original code runs, see F1 in [audit-2026-10-25/fixes.md](audit-2026-10-25/fixes.md).

## Won't do

- Uptime in the new `rcon status`: it would need a hook on `SV_Init()`, which is not possible.
- Extend the `jedi_/red` model check to every `jedi_*` model, as EternalJK does (`g_blockUnknownJediModelsInTeamGametypes`): only the six base Jedi models (`jedi_hf`, `jedi_hm`, `jedi_kdm`, `jedi_rm`, `jedi_tf`, `jedi_zf`) would be allowed in team gametypes, anything else becomes `kyle`. The proxy already replaces `jedi_` without a species. The rest is a server policy that would also block valid custom `jedi_*` models, and nobody asked for it.
- Patch `NET_SendPacket` to log packets. Maybe only packets under about 300 bytes, because some packets, such as info responses, are large.
- Only apply hooks on x86 (check the defines): the hooks are only installed when the `version` cvar is the original 32-bit engine (`win-x86` or `linux-i386`), so a 64-bit build never hooks anything. The hook code still compiles in a 64-bit build but never runs.
