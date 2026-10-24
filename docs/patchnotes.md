---
tags: JKA_YBEProxy
---

# JKA_YBEProxy Patchnote

> [!WARNING]
> Work in progress 

## 0.19.0 Beta

- Add [`proxy_sv_disableKillCmd`](https://hackmd.io/kvj--DTaTmOofjxL5bud-Q#proxy_sv_disableKillCmd)
    - Default value is `0`

## 0.18.0 Beta

- Proxy refactor

## 0.17.0 Beta

- Refactored hook/asm code

## 0.16.0 Beta

- Add [`proxy_sv_enableNetStatus`](https://hackmd.io/kvj--DTaTmOofjxL5bud-Q?view#proxy_sv_enableNetStatus)
    - Default value is `0`
- Add [`proxy_sv_modelPathLength`](https://hackmd.io/kvj--DTaTmOofjxL5bud-Q?view#proxy_sv_modelPathLength)
    - Default value is `64`
- Add [`proxy_sv_enableRconCmdCooldown`](https://hackmd.io/kvj--DTaTmOofjxL5bud-Q?view#proxy_sv_enableRconCmdCooldown)
    - Default value is `0`
- Fix a padding issue in `netStatus`
- Fix truncated print remote command (`rcon status` & co)

## 0.15.0 Beta

- The `cl_timenudge` calculation support ~~a range of `sv_fps` from `20` to `80` and `30`~~ `sv_fps` `20` and `30`
    - **edit**: the server limits `snaps` to `30`, if you set higher `sv_fps` then the calculation won't be accurate at all, you better stay with `20` or `30` for now
    - ~~(Windows) **Not** supported `sv_fps`: `60`, `61`, `62`~~
    - ~~(Linux) **Not** supported `sv_fps`: `31`, `32`, `33`, `34`, `41`, `42`, `43`, `44`, `59`, `60`, `61`, `62`~~
- Fix a model crash length by only allowing a certain range of ASCII chars
- Fix callvote string length crash
- Fix callvote issues by doing more checks with:
    - `capturelimit`
    - `fraglimit`
    - `g_doWarmup`
    - `map_restart`
    - `timelimit`

## 0.14.1 Beta

- Enable back the "welcome" message

## 0.14.0 Beta

- Fix q3dirtrav
- Allow usage of `myratio` no matter the engine
- Change `proxy_sv_pingFix` default value to `0`
- Mistake: disabled the "welcome" message

## 0.13.0 Beta and below

- Split cvars:
    - The ones that affect original engine modifications (require original engine)
    - The ones that are just related to the proxy (no matter the engine)
- Fix rcon disabler
- Fix `writeconfig`, it should only work on `.cfg` files
- Fix negative callvote `map_restart` value
- add `proxy_sv_maxCallVoteMapRestartValue`
    - Default value is `60`
- Fix high callvote `map_restart` value
- Better implementation of `SV_ExecuteClientMessage`
    - Fix exploit for new client game state sent cause of wrong `serverId`
    - Fix a bug with `nextdl`
    - (Also fix debug build)
- Fix model length crash on some custom clients
    - You're not allowed to use a model with a length greater than `64`
- Better implementation of `Cmd_TokenizeString` (it uses `Cmd_TokenizeString2`)
- Fix q3infoboom
- Add `proxy_sv_pingFix`
    - Default value is `1`
- Only patch original engine
- Only send a new gamestate if not already connected (fix donedl exploit)
- Better `/rcon status`
- Safe implementation of `Com_Printf`
- **C++ify**
- `/myratio`
- Fix model ghost bug
- `/netstatus`
- get client's packets number (thanks to `jk2mv` and `fau`)
- get client's fps (thanks to `jk2mv` and `fau`)
- calculate approximation of `cl_timenudge`
- pingfix (sort of true ping) & fix ping calculation ([c318f53](https://github.com/mvdevs/jk2mv/commit/c318f53b8b22e64d301fd19f0abf83613b8f6940) & [0301bea](https://github.com/mvdevs/jk2mv/commit/0301bea04182a021e5fb87c33534f0a256a4910f))
- Fix forcepowers crash
- Fix bad models
- Fix bad names
- Fix obscure exploit (angle, forcesel)
- Don't allow `\n\r;` in client commands except `;` for `say`/`say_team`/`tell`
- Remove ability to put negative value on fraglimit and timelimit
- Disable `callteamvote`
- Fix spectator team follow crash
- Fix `npc spawn` crash
- Fix `gc` crash
- Fix `say`/`say_team`/`tell` crash
- Kick players that use `DST`
- Don't allow players to send command if not fully connected
- Ability to block a callback if needed
- Support both Windows and Linux
- Support both 32 and 64 bits
- Support the new API from OpenJK
- Support the default API
- Ability to use CMAKE to compile the project
