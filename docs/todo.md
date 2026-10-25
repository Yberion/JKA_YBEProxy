# Todo

> [!NOTE]
> Bugs and improvements found during the audit are tracked separately in [audit-2026-10-25/](audit-2026-10-25/README.md). Items below that the audit covers link to it.

## Open

### Short term

- Persist bans across server restarts, in a file.
- **Create a plugin system.** See A6 in [audit-2026-10-25/improvements.md](audit-2026-10-25/improvements.md).
- Rate policy.
- Snaps policy.

### Bugs

None known.

### Long term

- Features from JKA_Anti-cheat: https://hackmd.io/CtIPZON0QHKuqjdxXRw8uw

## Maybe

- ffa3 exploit with movement events (a player with a high FPS lags every player in view). EternalJK fixes it in the engine. Postponed: two designs were written then removed, they are kept in O9 in [audit-2026-10-25/other-engines.md](audit-2026-10-25/other-engines.md).

## Done

### Features and fixes

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

- `rcon map mp/ffa3` (any map) and `rcon map_restart 0` sometimes crashed the server since the hooked `SVC_RemoteCommand` was added, more often with Discord running on the server machine. Proxy hook functions were still on the call stack when the engine unloaded the proxy. The packet checks now run in a filter at the entry of `SV_PacketEvent`, which returns before the original code runs, see F1 in [audit-2026-10-25/fixes.md](audit-2026-10-25/fixes.md).

## Won't do

- Uptime in the new `rcon status`: it would need a hook on `SV_Init()`, which is not possible.
- Extend the `jedi_/red` model check to every `jedi_*` model, as EternalJK does (`g_blockUnknownJediModelsInTeamGametypes`): only the six base Jedi models (`jedi_hf`, `jedi_hm`, `jedi_kdm`, `jedi_rm`, `jedi_tf`, `jedi_zf`) would be allowed in team gametypes, anything else becomes `kyle`. The proxy already replaces `jedi_` without a species. The rest is a server policy that would also block valid custom `jedi_*` models, and nobody asked for it.
- Patch `NET_SendPacket` to log packets. Maybe only packets under about 300 bytes, because some packets, such as info responses, are large.
- Only apply hooks on x86 (check the defines): the hooks are only installed when the `version` cvar is the original 32-bit engine (`win-x86` or `linux-i386`), so a 64-bit build never hooks anything. The hook code still compiles in a 64-bit build but never runs.
