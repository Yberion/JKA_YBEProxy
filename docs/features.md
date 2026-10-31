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
| [`proxy_sv_disableServerInfoVersion`](#proxy_sv_disableserverinfoversion) | `0` | Any | Removes the proxy version from the serverinfo |
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

### `proxy_sv_enableNetStatus`

Engine: Original.

Values:

- `0` (default): `/netStatus` and `/showNet` are disabled.
- `1`: the commands are enabled. See [`/netStatus`](#netstatus-shownet).

### `sv_minRate`

Engine: Original. Same name and meaning as in ioq3, Quake3e, OpenJK and JK2MV.

The `rate` of a client is the most bytes per second it wants from the server. After each message, the server waits for it to clear at that rate, so a client short of rate gets fewer snapshots than its `snaps` (yellow in `cg_lagometer`), and its game state and downloads come slower. The retail client sends `rate 4000` by default: at `snaps 20`, it is short as soon as a snapshot is bigger than about 150 bytes.

`sv_minRate` is a floor to the `rate` of the clients. The engine's `sv_maxRate` (default `0`, no cap) stays the cap and wins over a higher floor: the same value in both gives every client that rate, like `sv_ratePolicy 1` and `sv_clientRate` in OpenJK. A LAN client is never limited. A change applies within a second to the clients already in. Like `sv_maxRate`, it is in the serverinfo.

The rate a client needs for all its snapshots is about `(snapshot size + 48) × snaps`: `50000` covers 30 snapshots per second of 1400 bytes. `/netStatus` shows the rate of each client and what it really gets.

Values:

- `0` (default): no floor, the `rate` of the client (1000 to 90000).
- A number: the lowest rate given to a client. Below 1000 it changes nothing.

### `sv_minSnaps` `sv_maxSnaps`

Engine: Original. Same names and meaning as in JK2MV.

`snaps` is the number of snapshots per second a client wants. The client sees the other players about one snapshot interval in the past (50 ms at 20 snapshots per second, 25 ms at 40), on top of its ping: the base game has no lag compensation, so it adds to the lead a player needs to aim. The engine keeps `snaps` between 1 and 30 whatever `sv_fps` is, and the retail client sends `snaps 20` by default.

The server sends snapshots on its frames only, so a client gets `sv_fps / n` snapshots per second: the highest one that is not above its `snaps`.

| `sv_fps` | `snaps 20` | `snaps 30` | `snaps 40` |
| --- | --- | --- | --- |
| 20 | 20 | 20 | 20 |
| 30 | 15 | 30 | 30 |
| 40 | 20 | 20 | 40 (20 without the proxy) |

`sv_maxSnaps` is a cap to the `snaps` of the clients, never above `sv_fps`. `sv_minSnaps` is a floor, and the cap wins over a higher floor: `sv_minSnaps` at `sv_fps` or above gives every client one snapshot per frame, like `sv_snapsPolicy 1` in OpenJK. Take the values from the table: a floor of 30 at `sv_fps 40` gives 20. A change of either, or of `sv_fps`, applies within a second to the clients already in. A LAN client gets a snapshot at every frame whatever its `snaps`, as before.

More snapshots need more [`rate`](#sv_minrate): about `(snapshot size + 48) × snaps`. `/netStatus` shows the `snaps` each client asked for, the one it gets, and what the server really sent.

Values of `sv_minSnaps`:

- `0` (default): no floor.
- A number: the lowest `snaps` given to a client.

Values of `sv_maxSnaps`:

- `0` (default): `sv_fps`.
- A number: the highest `snaps` given to a client, at most `sv_fps`.

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

### `proxy_sv_disableServerInfoVersion`

Engine: Any.

The proxy adds a `YbeProxy` key with its version (for example `1.0.0 Beta`) to the serverinfo. Clients see it in the server info window of the server browser (`getstatus`) and with `rcon serverinfo`. The key is read-only.

Values:

- `0` (default): the `YbeProxy` key is in the serverinfo.
- `1`: the key is removed from the serverinfo.

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
| `gamePing` | Ping of the engine, the one of the scoreboard and `status` (`CON` while connecting, `ZMB` for a zombie client, `BOT` for a bot): the mean time between a message and its acknowledgement, over the last 32 messages. It holds the wait of the client before it sends its next packet (half the time between two packets on average, that its commands wait too) and the jitter. Rounded to the server frame without [`proxy_sv_pingFix 1`](#proxy_sv_pingfix) |
| `netPing` | Ping of the network: the smallest round trip of the last 4 seconds, measured with real milliseconds, close to the ping of the OS. Without the wait of the client and the jitter. `BOT` for a bot |
| `userRate` | `rate` of the client's userinfo, as it sent it |
| `rate` | Rate the server applies: the `rate` of the client kept between 1000 and 90000 (99999 for a LAN client, which is never limited), raised to [`sv_minRate`](#sv_minrate), capped by `sv_maxRate` |
| `trueRate` | Bytes per second the server sent in the last second, with 48 bytes of header per message, as the engine counts |
| `fps` | User commands received during the last second (client FPS) |
| `packets` | Packets received during the last second |
| `timeNudge` | Estimate of the client's `cl_timenudge` (see below). `-` until it is measured, and for a client that gets fewer than about 10 snapshots per second |
| `userSnaps` | `snaps` of the client's userinfo, as it sent it |
| `snaps` | Snapshots per second the server applies: the `snaps` of the client between [`sv_minSnaps` and `sv_maxSnaps`](#sv_minsnaps-sv_maxsnaps), at most `sv_fps` |
| `trueSnaps` | Snapshots per second the server sent in the last second |
| `id` | Client number |
| `name` | Name |

`/explainNetStatus` prints the meaning of each column for the player, in short, and how to read `packets`, `trueSnaps` and `trueRate`. The output of `/netStatus` ends with a line that points to it.

A player can run the command at most once every 500 ms, `/explainNetStatus` included. Bots show `BOT` for `gamePing` and `netPing`, `0` for everything else but `userRate`, `rate`, `userSnaps` and `snaps`, and `-` for `timeNudge`. All the values of the last second are measured when the command runs, so a client that stopped sending shows `0`, not its last value.

- `packets` is what the server received. It is not `cl_maxpackets`: a client sends a packet at the first frame where `1000 / cl_maxpackets` milliseconds went by since the last one, so at 125 fps (8 ms frames) `cl_maxpackets 100` (10 ms) gives one packet every 2 frames, about 62 per second, and a client in the same LAN sends one at every frame.
- `trueSnaps` is lower than `snaps` when the server sends on its frames: a request between two frame rates gets the lower one (`sv_fps 40` and `snaps 30` give 20), and a client that is short of `rate` gets less.
- `rate` is a limit, not a target: `trueRate` is the size of the snapshots times their number, and stays below `rate` as long as they fit. A `trueRate` close to `rate` with `trueSnaps` below `snaps` is a client short of rate: raise [`sv_minRate`](#sv_minrate).
- `timeNudge` compares the time of the newest usercmd of each packet with the server time (the mean over one second), and subtracts the mean round trip (the mean of the round trips of the last 4 seconds minus the average wait of the client before it sends an acknowledgement, never below `netPing`). Both are means, so the jitter of the path to the server cancels out. It also adds back the time each snapshot waited on the server before it left (the game frame, the snapshots of the other clients), which the client sees as delay: without it, 26 bots raised the estimate by about 3 ms. Where the client settles its time depends on how the snapshots fall on its frames, so the proxy runs the time rules of the client (`CL_SendCmd`, `CL_AdjustTimeDelta`, `CL_SetCGameTime`) with the measured snapshot interval, frame length and frames per packet, and no network delay, and compares. It is the median of three windows of one second, limited to ±100 (the retail client limits `cl_timeNudge` to ±30, other clients allow more). It does not need `proxy_sv_pingFix`, and works with any `sv_fps`. It shows `-` when the snapshots of the client are 95 ms apart or more (for example `snaps` 10 to 19 with `sv_fps 20`, 10 to 14 with `sv_fps 30`): the client is then about 100 ms behind its newest snapshot, moves its time by halves (`CL_AdjustTimeDelta`), and its time wanders by tens of milliseconds. A fixed half frame instead of the simulation was off by -0.3 to -6 ms depending on `sv_fps` and the fps (TaystJK client, Linux server); the simulation gives those errors within 0.7 ms. The jitter of the path from the server adds up to about 2.5 ms (the client settles behind its late snapshots), in a simulation. Measured on a Linux server with a TaystJK client and the retail client: a mean error of +0.2 to +1.0 ms without added delay (+0.6 to +0.8 ms with 26 bots), and up to +3.6 ms with 31 ms of delay from the server (clumsy).

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
- `ban <client number>` and `unban <client number>` are `addip` and `removeip` of the address of a player in the game (the client number shown by `status`). `ban` prints the player name and the address. Like `addip`, it does not drop the player: they are refused at the next map change, or when they connect again. `unban` removes the ban of that exact address, not a wider ban that covers it (`152.0.*.*`): use `removeip` for that. Bots have no address.
- The proxy keeps the list in the file `ybeproxy_bans.txt`, next to `jampserver.cfg` (`fs_homepath/<fs_game>/`, `base` without `fs_game`), and the game's own list stays empty. The file has one ban per line (`152.0.*.*`). It is read when the game loads (at every map change) and rewritten after every `addip` and `removeip`. It can be edited by hand while the server runs: the change applies at the next map change, and the next `addip` or `removeip` before it overwrites the file. Up to 1024 bans, as in the game.
- The bans of `g_banIPs` are imported into the file when the game loads, with their old meaning (a `0` was a wildcard), then `g_banIPs` is emptied.
- A banned address is refused on connect with `Banned.`, as before. Bots and `localhost` are never banned.
- With `g_filterBan 0` (only the listed addresses may connect) the proxy does nothing: the game keeps its own list and rules, and a `0` stays a wildcard. The mode is read when the game loads: after a change of `g_filterBan`, restart the map.

Credits: OpenJK's `StringToFilter`.

### `forceteam -1 <team>`

Engine: Any. Server console or `rcon` only.

The game's `forceteam <player> <team>` takes a client number or a name. `-1` now means every player in the game: `rcon forceteam -1 spectator` moves everyone to spectators. The team is anything the game accepts (`spectator`, `red`, `blue`, `free`…). The proxy runs the game's own `forceteam <number> <team>` for each player, so the usual rules apply (for example, the team is still refused where the game refuses it). A player who is still connecting is skipped.

### `proxyconfig`

Engine: Any. Server console or `rcon` only.

Prints the proxy version, the engine (its `version` string, and whether it is the original engine, the only one the engine fixes apply to), then every proxy cvar with its value and its default. The cvars of the original engine (`proxy_sv_pingFix`, `proxy_sv_enableNetStatus`, `sv_minRate`, `sv_minSnaps`, `sv_maxSnaps`) are listed on the original engine only, with the engine's `sv_maxRate` next to `sv_minRate`.

Then the main server settings, with their value only (their default depends on the engine and the mod): `sv_hostname`, `sv_maxclients`, `sv_privateClients`, `sv_fps`, `sv_maxRate` (on the other engines), `sv_pure`, `fraglimit` and `timelimit`.

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

### Spectator chat below the team gametypes

Engine: Any.

In FFA, duel and the other gametypes without teams, the game turns `say_team` into a public `say`, so the spectators could not talk among themselves without the players seeing it. The `say_team` of a spectator now only reaches the spectators, shown as a team chat. The `say_team` of a player stays public, as before. The game still logs it as `say:`.

### Welcome message

Engine: Any. When they enter the game after their first connection, human players receive a message with the proxy name and version. Turn it off with [`proxy_sv_disableWelcomeMessage`](#proxy_sv_disablewelcomemessage).

### NPC navigation

Engine: Any.

The game never frees the navigation data of the NPCs (the nodes of the `.nav` file of the map, or one per `waypoint` entity), and the retail engine adds the nodes of the new map to those of the previous one: memory leaked at each map change and `map_restart`, and NPCs could pick a node of the previous map. The proxy frees it before the game loads, as OpenJK does. Bots use another system and are not affected.

### Ghoul2 calls

Engine: Any.

The engine uses the Ghoul2 model instance that the game passes without a test, so a null one crashed the server. The game passes one when a model failed to load: OpenJK saw it with an NPC without its model or animations, and with FFA changed to Siege then `map_restart`. The proxy answers these calls as the engine does for an empty instance, as OpenJK does.

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
- A `map_restart` only puts back in the game the clients that were in it, as Quake3e does. The original put every connected client in the game, even one that did not have the map yet (one that downloads a pk3, for example): with the fix above, that client never got the map and stayed stuck. The others now finish their connection.
- Before a counter of the engine wraps, the original server stops (`SV_Shutdown`) and then fails to start again (`map_restart` refuses to run once the server is down), so it stays without a map until someone restarts it. The proxy loads the current map again a little before:
  - the snapshot entities counter grows with every snapshot sent, and only a map change resets it: a server that stays on one map with many players reaches it in hours. The proxy runs `map <current map>`, a normal map change: the players stay connected;
  - the server time, after about 21.7 days: only a shutdown resets it. The proxy runs `killserver` then `map <current map>`, as ioq3 and OpenJK do. The players are dropped.
- Maps that contain other BSPs (`misc_bsp` entities) no longer leak memory. The engine read each of these BSP files into a buffer that it never freed, at each map load. The proxy frees these buffers once the game has loaded the map; OpenJK frees them at the end of their load. No map of the base game has a `misc_bsp`.
- Downloads:
  - only `.pk3` files can be downloaded;
  - paths with directory traversal are refused (q3dirtrav fix);
  - only pk3 files referenced by the server can be downloaded;
  - download commands are refused once the client is in the game.
- `getstatus` and `getinfo`:
  - requests with an argument longer than 128 characters are ignored (q3infoboom fix);
  - rate limiting, so the server cannot be used to amplify a DDoS: each address gets a burst of 10, then 1 per second, and the server answers a burst of 10, then 10 per second in total. Requests beyond that are ignored. During a flood the server may briefly disappear from server browsers.
- `getchallenge` and `connect`: the same per-address limit, shared with `getstatus` and `getinfo` (a burst of 10, then 1 per second), as Quake3e. Requests beyond that are ignored. Players behind one public address (LAN party, carrier NAT) who connect at the same time may wait a few seconds: their client retries every 3 seconds.
- `sv_maxPing` can no longer be bypassed. The ping checked on connect is the time between the last challenge answer and the `connect`, and the original kept the same challenge for an address: a client could ask for a challenge again right before it connected with the one it already had, and get in with a ping close to 0. Each `getchallenge` now gets a new challenge, as in ioq3 and OpenJK.
- With `developer 1`, every request dropped by these limits, rcon included, is printed in the server console, as in ioq3 and Quake3e.
- rcon:
  - the engine's 500 ms timer is removed. It was global and checked before the password, so anyone could lock the admins out (rcon disabler fix);
  - a valid password is never limited, even during a flood with spoofed addresses;
  - bad passwords are limited for the whole server, whatever the addresses: a burst of 10, then 1 per second. Packets beyond that are ignored without a reply;
  - rcon output is sent in chunks of 1008 bytes, so long output such as `rcon status` is no longer truncated.
- `writeconfig` only writes `.cfg` files. A name without an extension still gets `.cfg`. It refuses any other extension, a name with `..`, and the reserved names `mpdefault.cfg` and `default.cfg`. The check is in the command itself, so it applies to rcon, the server console, `vstr` and `exec`.
- `status` is rewritten:
  - it shows the hostname, address, OS, version, gamedir, map, gametype, and the number of humans and bots;
  - the name is printed last, so it is never truncated;
  - a bot shows `BOT` instead of its ping.
- `SV_SvEntityForGentity` computes the entity index from the entity's address instead of trusting `s.number`, like JK2MV's fix for the "player ghosting" bug (players walking through another player). When the address is not an entity of the game's array, it falls back to `s.number`, and an index out of range stops with an error instead of reading outside the array.
- Ping calculation: bots show `0`, and the accurate mode is available (see [`proxy_sv_pingFix`](#proxy_sv_pingfix)).
