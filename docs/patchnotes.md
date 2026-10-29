# Patch notes

> [!WARNING]
> The proxy is a work in progress.

## Unreleased

- The serverinfo has a new `YbeProxy` key with the proxy version (for example `0.19.0 Beta`), shown in the server info window of the server browser. Added [`proxy_sv_disableServerInfoVersion`](features.md#proxy_sv_disableserverinfoversion) (default `0`, `1` removes the key). Any engine.
- `/netStatus` (original engine): the `ping` column is renamed `gamePing`, next to a new `netPing` column (the ping of the network: the smallest round trip of 4 seconds, measured with real milliseconds, without the wait of the client before it sends its next packet). `timeNudge` is a new estimate that runs the time rules of the client: it works with any `sv_fps` and no longer needs `proxy_sv_pingFix` (the old one used `sv_fps` tables), and shows `-` until it is measured and for a client that gets fewer than about 10 snapshots per second. New column `trueSnaps` (snapshots really sent per second, next to the `snaps` asked for). `fps` and `packets` are counted in real time over the last second: they were off by one, used the client clock, and kept their last value when a client stopped sending.
- IP bans (`addip`, `removeip`, `listip`) no longer treat a `0` as a wildcard. `addip 152.0` bans `152.0.*.*` instead of `152.*.*.*`, and `addip 152.0.24.34` no longer bans `152.99.24.34`. `*` is the wildcard, as in OpenJK. The proxy now keeps the ban list itself (`proxy_sv_banIPs`); the bans of `g_banIPs` are imported when the game loads, with their old meaning, and `g_banIPs` is emptied. Any engine, with `g_filterBan 1` only.
- `forceteam -1 <team>` (server console and `rcon`) now applies the team to every player in the game, for example `rcon forceteam -1 spectator`. The game looked up `-1` as a name and did nothing. Any engine.
- Original engine: `dumpuser` accepts a client number as well as a name, as in ioq3, OpenJK and Quake3e.
- Added the [`mute` and `unmute`](features.md#mute-unmute) commands (server console and `rcon`): `mute <client number>` drops the `say`, `say_team`, `tell` and `voice_cmd` of a player until they leave, the map changes, or they are unmuted. Any engine.
- Original engine: fixed a one-byte buffer overflow in `Info_SetValueForKey` and `Info_SetValueForKey_Big`. When the new key would bring the string to exactly 1024 (or 8192) characters, the engine accepted it and wrote the terminating `\0` past the buffer. A `connect` packet with a 1007-character userinfo reached it through the `ip` key that `SV_DirectConnect` adds (a valid challenge is enough). It is the `>=` of ioq3 and JK2MV. The client is refused with `Userinfo string length exceeded.` as before.
- The proxy cvars are read once per second of server time. It was once every `sv_fps` frames, so a changed `sv_fps` also changed the delay.
- Added [`proxy_sv_disableWelcomeMessage`](features.md#proxy_sv_disablewelcomemessage) (default `0`, `1` disables the message): the welcome message sent to human players when they enter the game can be turned off. Any engine.
- Original engine: the message `Client <name> lost <n> clientCommands` in the server console printed 2 more than the real number of lost commands. Cosmetic.
- Original engine: reading a client message stops at its end, as ioq3 and OpenJK do (CVE-2017-11721). The original `MSG_ReadBits` kept reading the bytes after the end of a short or truncated message, and gave them as values. A read past the end now gives `0`. Hardening: the callers already stopped on an overflow, and the buffer is on the stack, so nothing past it could be read.
- A `forwardmove`, `rightmove` or `upmove` of `-128` in a usercmd is now `-127`, as in ioq3, OpenJK, Quake3e and TaystJK: a client could move about 1% faster backwards than forwards. Any engine.
- The userinfo is now validated before the fixes of the name, the model and `forcepowers` read it, with OpenJK's default rules (size, slashes, control characters, number of each known key). A long userinfo, or one with a key twice or with another case, used to let a bad `forcepowers` or model through, because the game reads a key without case and the proxy replaced it with case. A client that fails is refused on connect, and dropped on a userinfo change, with `Failed userinfo validation: <reason>`. When a cleaned value does not fit in the userinfo, the client is refused too. Any engine. Bots are not validated.
- Original engine: the command tokenizer reads a bounded copy of the text, as Quake3e does, so a text longer than 8191 characters is cut instead of overflowing the token buffer. No caller of the original game is known to send such a text: this is hardening.
- Added [`proxy_sv_maxClientsPerIP`](features.md#proxy_sv_maxclientsperip) (default `3`, `0` disables it): at most that many clients from one IP address. One address can no longer fill every slot of the server. Any engine.
- A player who is already on the server can no longer stay after its address is banned (`addip`) by sending a fake or missing `ip` in its userinfo: the game only checks the ban again at the next map change, with the `ip` of the userinfo. A new connection was never affected, the engine sets `ip` itself. The proxy now sets `ip` again from the address of the connection at every userinfo change (original engine), and refuses on connect a userinfo that had no room for it (any engine).
- Hooks: the instruction decoder is now [Zydis](https://github.com/zyantific/zydis) (git submodule) instead of HDE, and the hook code was rewritten:
  - a `call` or `jmp` with a 32-bit offset in the bytes copied to the trampoline is relocated, so it reaches its target;
  - any other relative instruction in those bytes, a function shorter than 5 bytes, or a decoding error makes the hook fail;
  - if any hook or inline patch cannot be installed, the server stops with the reason in the console (before, it kept running with part of the fixes missing);
  - the original bytes put back by the unhook are the ones saved at the hook, not the ones read from the trampoline.
- Build: CMake rewritten (CMake 3.25 or later) with `CMakePresets.json`. The `CreateVisualStudio*.bat` scripts are removed: use `cmake --preset windows-x86` or `linux-x86-release` (see the README). The module is now in `build/<preset>/bin/`. MinGW and Apple builds are no longer supported.
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
- Fixed `/netStatus` (original engine): for any client with a ping of `0`, it set the engine's snapshot interval to 1 ms, so a human with a very low ping then received more snapshots than their `snaps` setting asked for, until their next userinfo change. `/netStatus` now only reads the engine state. Bots are detected by their address type instead of their ping, and a human with a ping of `0` now gets real `fps` and `packets` values.
- Fixed a possible server crash (division by zero) in the `/netStatus` `timeNudge` estimate when `sv_fps` was set to `0` (original engine, with `proxy_sv_enableNetStatus 1`). Setting `sv_fps` requires rcon or the server console.
- The proxy's per-client data is now reset on every disconnect, also for a client that leaves before entering the game. Before, the next client of the slot could inherit it.
- Client command fixes now compare command names and arguments whole, as the game does, instead of by prefix. `npc spawn` is now only refused for `ragnos` and `saber_droid`, not for every NPC type that starts with `saber_`, and commands such as `teamvote` or `gcX` are no longer caught by the `team` and `gc` checks. The `jkaDST_` anti-cheat check stays a prefix.
- `rcon status` checks that the gametype is in range before printing its name. The game already keeps it in range, so this changes nothing in practice.
- The original game module is now loaded from `fs_basepath` instead of the current working directory. With the default `fs_basepath`, which is the current directory, nothing changes. The path is built with a bounded copy, and the error printed before the server exits now shows the full path that was tried.
- `/myratio`: the number in parentheses is now always Kills minus Deaths, as shown. In red, it added the suicides back, so it could even be positive: Kills 2, Deaths 3, Suicides 2 showed a red `1`, and now shows a red `-1`. A player whose only death is a suicide after one kill now sees a red `-1` instead of `0`.
- Fixed the ping shown too low right after a connect (original engine). For the first 32 snapshots (about 1.6 s at 20 snapshots per second), the snapshot slots not used yet counted as a delay of 0. The engine's own calculation already skipped them, so this came from the proxy, also with `proxy_sv_pingFix 0`.
- Fixed a server crash on `rcon dumpuser` (original engine). A client could put a key or value of 512 characters or more in its userinfo, and `dumpuser` on that client overflowed a buffer of the engine. The engine function that prints these lists (`Info_Print`) is replaced by the ioq3/OpenJK version, which also fixes `serverinfo` and `systeminfo`. A `systeminfo` value can reach that length with `sv_pure 1` and many pk3 files (`sv_paks`, `sv_pakNames`); that case was not reproduced.
- Linux: `libstdc++` and `libgcc` are now linked into `jampgamei386.so`, so the server no longer needs the 32-bit `libstdc++` installed.
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
