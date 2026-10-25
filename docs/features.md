# Features

The "Engine" information below means:

- **Original**: only with the original 1.01 engine (`jampDed.exe` / `linuxjampded`), because the feature needs an engine patch.
- **Any**: with the original engine and with OpenJK.

## Cvars

All proxy cvars are archived (`CVAR_ARCHIVE`). The proxy reads them again about once per second, so a change applies without a restart.

| Cvar | Default | Engine | Summary |
| --- | --- | --- | --- |
| [`proxy_sv_pingFix`](#proxy_sv_pingfix) | `0` | Original | More accurate ping |
| [`proxy_sv_enableNetStatus`](#proxy_sv_enablenetstatus) | `0` | Original | Enables `/netStatus` and `/showNet` |
| [`proxy_sv_maxCallVoteMapRestartValue`](#proxy_sv_maxcallvotemaprestartvalue) | `60` | Any | Highest delay allowed for `callvote map_restart` |
| [`proxy_sv_maxClientsPerIP`](#proxy_sv_maxclientsperip) | `3` | Any | Maximum clients from one IP address |
| [`proxy_sv_disableKillCmd`](#proxy_sv_disablekillcmd) | `0` | Any | Disables `/kill` |
| [`proxy_sv_disableWelcomeMessage`](#proxy_sv_disablewelcomemessage) | `0` | Any | Disables the welcome message |
| [`proxy_sv_modelPathLength`](#proxy_sv_modelpathlength) | `64` | Any (partial) | Maximum model name length |

### `proxy_sv_pingFix`

Engine: Original.

The engine measures ping in server frames (`svs.time`), so the value is rounded to the frame duration (50 ms at `sv_fps 20`), and low pings often show as `0`. With the fix, the proxy:

- timestamps sent and acknowledged snapshots with real milliseconds;
- keeps only the first acknowledgement of each snapshot;
- never reports a ping below 1 ms.

Values:

- `0` (default): engine ping calculation.
- `1`: accurate ping calculation.

> [!WARNING]
> The `timeNudge` column of [`/netStatus`](#netstatus-shownet) needs this fix. With `0`, that column is wrong.

### `proxy_sv_enableNetStatus`

Engine: Original.

Values:

- `0` (default): `/netStatus` and `/showNet` are disabled.
- `1`: the commands are enabled. See [`/netStatus`](#netstatus-shownet).

### `proxy_sv_maxCallVoteMapRestartValue`

Engine: Any.

Highest delay, in seconds, accepted for `callvote map_restart <delay>`. The proxy drops a vote outside `0` to this value without any message.

Values: a number. Default `60`.

### `proxy_sv_maxClientsPerIP`

Engine: Any.

Maximum number of clients that can be connected at the same time from one IP address (the port is ignored). A new connection beyond that is refused with `Too many connections from your IP address.`. Without a limit, one address can fill every slot with clients that complete the connection (tools like q3fill).

- Only new connections are counted and refused. The clients that are already in are never removed, and the reconnect of every client at each map change is not affected.
- Bots are not counted.
- Several players can share one address (LAN party, carrier NAT): raise the value for them.

Credits: Quake3e's `sv_maxclientsPerIP`.

Values:

- `0`: no limit.
- a number: the limit. Default `3`.

### `proxy_sv_disableKillCmd`

Engine: Any.

Values:

- `0` (default): `/kill` works.
- `1`: the proxy ignores `/kill`.

### `proxy_sv_disableWelcomeMessage`

Engine: Any.

Values:

- `0` (default): human players receive the [welcome message](#welcome-message).
- `1`: the message is not sent.

### `proxy_sv_modelPathLength`

Engine: Any, with a partial effect outside the original engine.

When a model name is this length or longer, the proxy replaces it with `kyle`. Long model names crash some custom clients.

The proxy checks the model:

- on connect, and when the game processes a userinfo change (any engine);
- in the engine's `SV_UserinfoChanged` as well (original engine only).

Values: a number. Default `64`.

## Commands

### `/netStatus` `/showNet`

Engine: Original. Needs `proxy_sv_enableNetStatus 1`.

Prints the network settings of every client:

| Column | Meaning |
| --- | --- |
| `score` | Score |
| `ping` | Ping (`CON` while connecting, `ZMB` for a zombie client) |
| `rate` | Client `rate` |
| `fps` | User commands received during the last second (client FPS, from jk2mv) |
| `packets` | Packets received during the last second (client `cl_maxpackets`, from jk2mv) |
| `timeNudge` | Estimate of the client's `cl_timenudge`, about ±7 with a stable connection |
| `snaps` | Snapshots per second the client receives (capped by `sv_fps`) |
| `id` | Client number |
| `name` | Name |

A player can run the command at most once every 500 ms. Bots show `0` for `fps` and `packets`. A human whose ping shows `0`, which can happen without [`proxy_sv_pingFix`](#proxy_sv_pingfix), gets real values.

> [!WARNING]
> The `timeNudge` estimate needs:
>
> - `proxy_sv_pingFix 1`;
> - `sv_fps` set to `20` or `30`. The engine caps snapshots at 30 per second, so with a higher `sv_fps` the estimate is not accurate.

### `/myratio`

Engine: Any.

Prints your score (used as kills), deaths, suicides, kill/death ratio, and kills minus deaths. Only in Free For All, Duel and Team FFA. A player can run it at most once per second.

### `mute` `unmute`

Engine: Any. Server console or `rcon` only.

`mute <client number>` stops the chat of a player in the game: their `say`, `say_team`, `tell` and `voice_cmd` are dropped, and they get `You are muted.` instead. `unmute <client number>` lifts it. The client number is the one shown by `status`. Both commands print the player name, and tell the player.

The mute is kept in the proxy's data of the client slot. It ends when the player leaves, and also at a map change, because the game module (and the proxy with it) is loaded again.

### `addip` `removeip` `listip`

Engine: Any. Server console or `rcon`. Only with `g_filterBan 1` (the default).

IP bans, with the rules of OpenJK instead of the retail game's. In the retail game a `0` in a ban is a wildcard: `addip 152.0` banned `152.*.*.*`, and `addip 152.0.24.34` also banned `152.99.24.34`. Now every octet that is written is compared, `0` included, and `*` is the wildcard:

| Command | Bans |
| --- | --- |
| `addip 152.0` | `152.0.*.*` |
| `addip 152.0.24.34` | that address only |
| `addip 152.*.24.34` | any second octet |
| `addip 152` | `152.*.*.*` |

- `removeip` takes the same spelling. The old spelling of a ban made before the proxy (`removeip 152.0.0.0` for `152.*.*.*`) still finds it.
- `listip` prints the bans, with `*` for a wildcard.
- The proxy keeps the list in `proxy_sv_banIPs` (archived, like `g_banIPs` was), and the game's own list stays empty. The bans of `g_banIPs` are imported when the game loads, with their old meaning (a `0` was a wildcard), then `g_banIPs` is emptied. The cvar holds `MAX_CVAR_VALUE_STRING` characters, about 20 bans, as `g_banIPs` did. The bans after that are kept until the map changes only.
- A banned address is refused on connect with `Banned.`, as before. Bots and `localhost` are never banned.
- With `g_filterBan 0` (only the listed addresses may connect) the proxy does nothing: the game keeps its own list and rules, and a `0` stays a wildcard. The mode is read when the game loads: after a change of `g_filterBan`, restart the map.

Credits: OpenJK's `StringToFilter`.

### `forceteam -1 <team>`

Engine: Any. Server console or `rcon` only.

The game's `forceteam <player> <team>` takes a client number or a name. `-1` now means every player in the game: `rcon forceteam -1 spectator` moves everyone to spectators. The team is anything the game accepts (`spectator`, `red`, `blue`, `free`…). The proxy runs the game's own `forceteam <number> <team>` for each player, so the usual rules apply (for example, the team is still refused where the game refuses it). A player who is still connecting is skipped.

## Automatic fixes

These are always on and need no configuration.

### Client commands

Engine: Any.

- The proxy ignores commands from a client that has not fully entered the game yet.
- Command names and arguments are compared whole and without case, as the game does.
- It drops commands that contain `\r` or `\n`.
- It drops commands that contain `;`, except `say`, `say_team` and `tell`. This prevents command injection.
- It drops `say`, `say_team` and `tell` with more than 256 characters (crash fix).
- It drops `gc` with a client number equal to or above `sv_maxclients` (crash fix).
- It drops `npc spawn ragnos` and `npc spawn saber_droid` (crash fix).
- It drops `team follow1` and `team follow2` (spectator follow crash fix).
- `callteamvote` is disabled: it is useless in basejka and breaks the UI on some custom clients.
- `callvote`:
  - It drops a vote whose value is 256 characters or longer (crash fix).
  - It accepts a vote only when the value is in range:

    | Vote | Allowed values |
    | --- | --- |
    | `capturelimit` | `0` to `2147483647` |
    | `fraglimit` | `0` to `2147483647` |
    | `g_doWarmup` | `0` or `1` |
    | `map_restart` | `0` to `proxy_sv_maxCallVoteMapRestartValue` |
    | `timelimit` | `0` to `35790` |

### Userinfo

Engine: Any.

The proxy checks the userinfo on connect, on the reconnect after each map change, and on every userinfo change. Bots are not checked on connect.

- The userinfo is validated first, with the default rules of OpenJK's `G_ValidateUserinfo`:
  - length: not empty, below 1024 characters;
  - slashes: a leading slash, no trailing slash, an even number of slashes;
  - no `\n`, `\r`, `;` or `"`;
  - the number of each known key, counted without case: `name`, `rate`, `snaps`, `model`, `forcepowers`, `color1`, `color2`, `handicap`, `cg_predictItems`, `saber1`, `saber2`, `char_color_red`, `char_color_green` and `char_color_blue` exactly once; `ip`, `sex`, `teamtask`, `password` and `teamoverlay` at most once; `cl_guid` and `cl_punkbuster` never (q3fill protection). Other keys are not checked.

  A client that fails is refused on connect with `Failed userinfo validation: <reason>`, and dropped with the same message on a userinfo change. The bots are not validated. A player who empties one of the required cvars (for example `/color1 ""`) loses that key, because an empty value is not kept in the userinfo, and is refused.
- When the proxy cannot put a cleaned value in the userinfo because it has no room left (the name, the model or `forcepowers`), the client is refused with `Userinfo string length exceeded.`, instead of keeping the value it sent.

- Names are cleaned, using a port of OpenJK's `ClientCleanName`:
  - leading spaces and `*` are removed;
  - control characters are removed;
  - runs of more than 3 spaces or `@` are shortened;
  - an empty name becomes `Padawan`.
- The model is replaced with `kyle` when it is empty, when it starts with `rancor` or `wampa` (any skin), when it is `jedi_` without a species (`jedi_` alone or with any skin), when it contains a character outside printable ASCII, or when it is too long (see [`proxy_sv_modelPathLength`](#proxy_sv_modelpathlength)).
- `forcepowers` must look like `rank-side-powers` (22 to 24 characters). Otherwise the proxy replaces it with `7-1-030000000000003332` (crash fix).
- The `ip` key, which the game's ban check (`addip`) reads, is set again from the address of the connection at every userinfo change (original engine). The engine only sets it when the client connects, and the `userinfo` command replaces the whole userinfo with the client's string: a client already on the server could send a fake `ip`, or none, and stay after its address was banned with `addip`: the game checks the ban again at the next map change (the reconnect of every client), with the `ip` of the userinfo. A new connection was never affected, the engine sets `ip` itself. A client whose userinfo has no room for `ip` is dropped (`userinfo string length exceeded`). On connect (any engine), a userinfo that was too long to receive `ip` is refused with `Userinfo string length exceeded.  Try removing setu cvars from your config.`, as on ioq3, JK2MV and OpenJK.

### User commands

Engine: Any.

- The proxy resets a force power selection of levitation, or out of range, to none (force exploit fix).
- It forces the roll angle to `0` (angle exploit fix).
- It turns a `forwardmove`, `rightmove` or `upmove` of `-128` into `-127`. A signed byte reaches `-128` but only `+127`, so a client could move one unit faster backwards, left and down than forwards, right and up (ioq3 bug 5273, also fixed by OpenJK, Quake3e and TaystJK).

### Anti-cheat

Engine: Any.

- A client that sends a command starting with `jkaDST_` is kicked, with a public message.
- A client using a model that starts with `darksidetools` is kicked, with a public message. On connect, it is refused with the same reason, without the public message.

### Welcome message

Engine: Any. When they enter the game after their first connection, human players receive a message with the proxy name and version. Turn it off with [`proxy_sv_disableWelcomeMessage`](#proxy_sv_disablewelcomemessage).

### Engine fixes

Engine: Original.

- `Com_Printf` formats into a bounded buffer. The original `vsprintf` could overflow.
- `Cmd_TokenizeString` is replaced by the Quake 3 1.32 version (`Cmd_TokenizeString2`), which reads a bounded copy of the text, as Quake3e does: a text longer than 8191 characters is cut instead of overflowing the engine's token buffer.
- `MSG_ReadBits`, which every `MSG_Read*` call goes through, stops at the end of the message, as ioq3 and OpenJK do (CVE-2017-11721). The original reads the bytes after the end of the received message: once it is past the end, a read now gives `0` and the callers see the overflow. One read can still go a few bytes past the end before the last test (a Huffman symbol has no fixed length), inside the receive buffer, and its value is dropped.
- `Info_SetValueForKey` and `Info_SetValueForKey_Big` refuse a result that reaches exactly 1024 (or 8192) characters, as ioq3 and JK2MV do. The original accepted it and wrote the terminating `\0` one byte past the buffer. A `connect` with a 1007-character userinfo could do it through the `ip` key added by `SV_DirectConnect`.
- `dumpuser` accepts a client number (the one shown by `status`) as well as a name, as in ioq3, OpenJK and Quake3e. The original only found a player by name. A name, or a number with no player, goes to the original command.
- `Info_Print`, used by `serverinfo`, `systeminfo` and `dumpuser`, gets the ioq3/OpenJK buffer sizes. A key or value of 512 characters or more, such as one a client puts in its userinfo, no longer crashes the server.
- Client packet processing (`SV_ExecuteClientMessage`, `SV_UserMove`) is rewritten with ioquake3 fixes:
  - the gamestate is only resent when the client really dropped it;
  - a client that floods text commands has those commands ignored, but its movement is still processed;
  - a `nextdl` bug is fixed.
- The server never sends a new gamestate to a client that is already active (`donedl` exploit fix).
- Downloads:
  - only `.pk3` files can be downloaded;
  - paths with directory traversal are refused (q3dirtrav fix);
  - only pk3 files referenced by the server can be downloaded;
  - download commands are refused once the client is in the game.
- `getstatus` and `getinfo`:
  - requests with an argument longer than 128 characters are ignored (q3infoboom fix);
  - rate limiting, so the server cannot be used to amplify a DDoS: each address gets a burst of 10, then 1 per second, and the server answers a burst of 10, then 10 per second in total. Requests beyond that are ignored. During a flood the server may briefly disappear from server browsers.
- `getchallenge` and `connect`: the same per-address limit, shared with `getstatus` and `getinfo` (a burst of 10, then 1 per second), as Quake3e. Requests beyond that are ignored. Players behind one public address (LAN party, carrier NAT) who connect at the same time may wait a few seconds: their client retries every 3 seconds.
- With `developer 1`, every request dropped by these limits, rcon included, is printed in the server console, as in ioq3 and Quake3e.
- rcon:
  - the engine's 500 ms timer is removed. It was global and checked before the password, so anyone could lock the admins out (rcon disabler fix);
  - a valid password is never limited, even during a flood with spoofed addresses;
  - bad passwords are limited for the whole server, whatever the addresses: a burst of 10, then 1 per second. Packets beyond that are ignored without a reply;
  - rcon output is sent in chunks of 1008 bytes, so long output such as `rcon status` is no longer truncated.
- `writeconfig` only writes `.cfg` files. A name without an extension still gets `.cfg`. It refuses any other extension, a name with `..`, and the reserved names `mpdefault.cfg` and `default.cfg`. The check is in the command itself, so it applies to rcon, the server console, `vstr` and `exec`.
- `status` is rewritten:
  - it shows the hostname, address, OS, version, gamedir, map, gametype, and the number of humans and bots;
  - the name is printed last, so it is never truncated.
- `SV_SvEntityForGentity` computes the entity index from the entity's address instead of trusting `s.number`, like JK2MV's fix for the "player ghosting" bug (players walking through another player). When the address is not an entity of the game's array, it falls back to `s.number`, and an index out of range stops with an error instead of reading outside the array.
- Ping calculation: bots show `0`, and the accurate mode is available (see [`proxy_sv_pingFix`](#proxy_sv_pingfix)).
