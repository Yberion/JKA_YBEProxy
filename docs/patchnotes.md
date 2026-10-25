# Patch notes

> [!WARNING]
> The proxy is a work in progress.

## Unreleased

- The userinfo sent on connect is now checked before the game reads it. The name, model and `forcepowers` fixes were skipped until the client's first userinfo change, and now also apply on connect and on the reconnect after each map change.
- The anti-cheat now refuses a client on connect, with its reason, instead of kicking it later.
- Fixed a ghost player entry left after an anti-cheat kick.
- Fixed the darksidetools anti-cheat: an empty model, or any start of `darksidetools` (`d`, `dark`…), kicked the player, and `darksidetools/default` was not detected. Every model that starts with `darksidetools` is now kicked.
- Every `rancor` and `wampa` model, skins included, and `jedi_` without a species, with any skin, are now replaced with `kyle`. Before, `rancor/default` or `jedi_/green` got through. An empty model also becomes `kyle`.
- OpenJK: the proxy no longer exports `GetModuleAPI`, so OpenJK loads it through `dllEntry` and `vmMain`, like the original engine. This fixes, on OpenJK:
  - proxy cvars that were never registered (every model was replaced with `kyle`, every `callvote map_restart` above `0` was refused);
  - `/myratio`, which never answered;
  - the levitation and roll angle fixes, which had no effect;
  - the startup log;
  - the welcome message, now sent when the player enters the game (OpenJK dropped it on connect);
  - a retail game module, which failed to start.
- Fixed the gametype check of `/myratio`: it read a `sv_gametype` cvar that does not exist, so it always passed. It now reads `g_gametype`.
- The proxy no longer adds `sv_fps` and a bogus `sv_gametype` to the serverinfo. On a server that ran an older proxy, they stay in it until the next server restart.
- Likely fix for the intermittent crash on `rcon map` and `rcon map_restart` (original engine). The out-of-band packet checks (`getstatus`/`getinfo` length, rcon cooldown and `writeconfig`, `ipAuthorize`) now run in one filter at the entry of `SV_PacketEvent`, which returns before the original packet code runs. No proxy code is left on the call stack when the engine unloads the proxy during the map change. The crash was never reproduced, so the fix is not confirmed.
- Added rate limiting on out-of-band packets (original engine):
  - rcon: a valid password is never limited. Bad passwords are limited for the whole server to a burst of 10, then 1 per second, so the password can no longer be brute-forced quickly;
  - `getstatus` / `getinfo`: a burst of 10, then 1 per second per address, and 10 per second in total, so the server can no longer be used to amplify a DDoS;
  - `getchallenge` / `connect`: the same per-address limit, shared with `getstatus` / `getinfo`, as Quake3e;
  - with `developer 1`, each dropped request is printed in the server console, as in ioq3 and Quake3e.
- Fixed the `writeconfig` filter (original engine). It only blocked a 3-letter extension other than `.cfg` in rcon, so `x.so`, a `.pk3`, `../` paths, or the command run through `vstr`/`exec` got through. `writeconfig` itself now refuses anything but a `.cfg` file name, a name with `..`, and `mpdefault.cfg`/`default.cfg`, as in OpenJK.
- Added the missing bounds checks to the player ghosting fix (`SV_SvEntityForGentity`, original engine), as in JK2MV. The index computed from the entity's address was never checked, and it divided by zero before the game located its entities. With the retail game this was not reachable, since it always passes entities of its own array.
- Removed `proxy_sv_enableRconCmdCooldown`, replaced by the rcon rate limiting. When set to `1`, its 500 ms cooldown applied to every rcon packet, even with a wrong password, so a flood of rcon packets also blocked the admins' commands. With the default `0`, rcon had no limit at all.

## 0.19.0 Beta

- Added [`proxy_sv_disableKillCmd`](features.md#proxy_sv_disablekillcmd) (default `0`).

## 0.18.0 Beta

- Refactored the whole proxy.

## 0.17.0 Beta

- Refactored the hook and assembly code:
  - moved to C++20;
  - moved every hook utility into the `HookUtils` namespace and added checks;
  - switched the length disassembler to the Hacker Disassembler Engine (HDE, x86 and x64).

## 0.16.0 Beta

- Added [`proxy_sv_enableNetStatus`](features.md#proxy_sv_enablenetstatus) (default `0`).
- Added [`proxy_sv_modelPathLength`](features.md#proxy_sv_modelpathlength) (default `64`).
- Added `proxy_sv_enableRconCmdCooldown` (default `0`).
- Fixed a padding issue in the `netStatus` output.
- Fixed truncated output of rcon commands (`rcon status` and others).

## 0.15.0 Beta

- The `cl_timenudge` estimate now only supports `sv_fps` `20` and `30`.
  - The engine caps snapshots at 30 per second, so with a higher `sv_fps` the estimate is not accurate at all.
  - Earlier versions claimed support for `sv_fps` `20` to `80`, except `60` to `62` on Windows and `31` to `34`, `41` to `44` and `59` to `62` on Linux.
- Fixed a model length crash: only printable ASCII characters are allowed in model names.
- Fixed a crash caused by a long `callvote` string.
- Added range checks on `callvote` for `capturelimit`, `fraglimit`, `g_doWarmup`, `map_restart` and `timelimit`.

## 0.14.1 Beta

- Re-enabled the welcome message.

## 0.14.0 Beta

- Fixed q3dirtrav.
- `/myratio` now works on any engine.
- Changed the default value of `proxy_sv_pingFix` to `0`.
- The welcome message was disabled by mistake (fixed in 0.14.1).

## 0.13.0 Beta and earlier

- Split the cvars into two groups:
  - cvars that control engine patches (original engine required);
  - cvars that only concern the proxy (any engine).
- Fixed the rcon disabler.
- `writeconfig` only works on `.cfg` files.
- Fixed negative `callvote map_restart` values.
- Added `proxy_sv_maxCallVoteMapRestartValue` (default `60`).
- Fixed high `callvote map_restart` values.
- Better implementation of `SV_ExecuteClientMessage`:
  - fixed an exploit that made the server send a new gamestate because of a wrong `serverId`;
  - fixed a bug with `nextdl`;
  - fixed the debug build.
- Fixed a model length crash on some custom clients: model names longer than `64` characters are not allowed.
- Better implementation of `Cmd_TokenizeString` (uses `Cmd_TokenizeString2`).
- Fixed q3infoboom.
- Added `proxy_sv_pingFix` (default `1`, changed to `0` in 0.14.0).
- Only the original engine is patched.
- A new gamestate is only sent to a client that is not already in the game (fixes the `donedl` exploit).
- Better `/rcon status`.
- Safe implementation of `Com_Printf`.
- Moved the code base to C++.
- Added `/myratio`.
- Fixed the model ghost bug.
- Added `/netstatus`:
  - client packets per second (thanks to `jk2mv` and `fau`);
  - client FPS (thanks to `jk2mv` and `fau`);
  - estimate of `cl_timenudge`.
- Ping fix, closer to the true ping, and fixed ping calculation (jk2mv commits [c318f53](https://github.com/mvdevs/jk2mv/commit/c318f53b8b22e64d301fd19f0abf83613b8f6940) and [0301bea](https://github.com/mvdevs/jk2mv/commit/0301bea04182a021e5fb87c33534f0a256a4910f)).
- Fixed the `forcepowers` crash.
- Fixed bad models.
- Fixed bad names.
- Fixed an obscure exploit (angles, `forcesel`).
- Client commands may not contain `\n`, `\r` or `;`. `;` is still allowed in `say`, `say_team` and `tell`.
- Negative values are refused for `fraglimit` and `timelimit`.
- Disabled `callteamvote`.
- Fixed the spectator team follow crash.
- Fixed the `npc spawn` crash.
- Fixed the `gc` crash.
- Fixed the `say` / `say_team` / `tell` crash.
- Players using `DST` are kicked.
- Players cannot send commands before they have fully entered the game.
- A call can be blocked by the proxy when needed.
- Windows and Linux support.
- 32-bit and 64-bit support.
- OpenJK API support.
- Original API support.
- CMake build.
