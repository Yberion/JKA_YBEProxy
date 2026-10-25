# Comparison with other engines

The proxy took several fixes from OpenJK, which itself took some from ioquake3. This file compares the proxy with the other engines that fix the same code base, multiplayer code only (`codemp`). It lists fixes that are missing, done better elsewhere, or that do not apply to JKA.

Sources: the repositories below, with their git history. Links to files are pinned to the commit that was read (see [README.md](README.md#sources)). Links to commits point to the commit that made the fix.

| Repository | Used for |
| --- | --- |
| [Quake-III-Arena](https://github.com/id-Software/Quake-III-Arena) | id's Quake 3 1.32 source. It tells whether a fix is from id (1.32 point release) or came later. |
| [jediacademy](https://github.com/jedis/jediacademy) | Raven's JKA source (`codemp`), close to the retail binaries but not identical. |
| [ioq3](https://github.com/ioquake/ioq3) | ioquake3 |
| [Quake3e](https://github.com/ec-/Quake3e) | Quake3e |
| [OpenJK](https://github.com/JACoders/OpenJK) | OpenJK, JKA engine and game |
| [jk2mv](https://github.com/mvdevs/jk2mv) | JK2MV. Like the proxy, it keeps the retail game module and fixes its exploits from the engine side. |
| [TaystJK](https://github.com/taysta/TaystJK) | Fork of EternalJK. Its engine has `sv_legacyFixes`, a set of engine-side fixes for retail game modules. |

Classes used below:

- **Missing**: the bug exists in the JKA code, another engine fixes it, the proxy does not.
- **Q3/JKA difference**: the other engine's fix targets code that JKA does not have, or that JKA or the proxy already fixes. Nothing to do.
- **To check**: the other engine fixes a game-module bug of JK2 or Quake 3. Whether JKA has the same bug was not checked.

## Already in the proxy

These proxy fixes match another engine.

| Proxy fix | Same fix in |
| --- | --- |
| `forcepowers` check | JK2MV `mv_fixforcecrash` ([`src/server/sv_client.cpp:1088-1136`](https://github.com/mvdevs/jk2mv/blob/7d601454c3db68492289d4d4e3dc30bff39e4246/src/server/sv_client.cpp#L1088-L1136)), same rules and same default string. Also in TaystJK `SV_UserinfoChanged`. |
| `proxy_sv_pingFix` | JK2MV `sv_pingFix` ([`src/server/sv_client.cpp:1481-1482`](https://github.com/mvdevs/jk2mv/blob/7d601454c3db68492289d4d4e3dc30bff39e4246/src/server/sv_client.cpp#L1481-L1482), [`sv_main.cpp:946`](https://github.com/mvdevs/jk2mv/blob/7d601454c3db68492289d4d4e3dc30bff39e4246/src/server/sv_main.cpp#L946)) |
| `/netStatus` packets and FPS columns | JK2MV, per the comment in [Proxy_Engine_Client.cpp:16](../../src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Client.cpp#L16) |
| `SV_SvEntityForGentity` from the address | JK2MV `mv_fixplayerghosting` ([`src/server/sv_game.cpp:71-90`](https://github.com/mvdevs/jk2mv/blob/7d601454c3db68492289d4d4e3dc30bff39e4246/src/server/sv_game.cpp#L71-L90), commit [`88c4603`](https://github.com/mvdevs/jk2mv/commit/88c4603)), without its bounds checks (F8). TaystJK calls it "skinglitch" (`SVFIXES_ALLOW_GHOSTED_PLAYERS`). |
| Usercmd: levitation and invalid `forcesel` reset, roll set to 0 | OpenJK `sv_legacyFixes` ([`codemp/server/sv_client.cpp:1375-1383`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/server/sv_client.cpp#L1375-L1383), commits [`d455743c`](https://github.com/JACoders/OpenJK/commit/d455743c), [`8b07e58c`](https://github.com/JACoders/OpenJK/commit/8b07e58c)) |
| `gc`, `npc spawn ragnos`, `team follow1/2`, `callteamvote`, long `say`, model checks | TaystJK `sv_legacyFixes` ([`codemp/server/sv_client.cpp:1470-1505`](https://github.com/taysta/TaystJK/blob/7d99efb4a77a174d1a7853f2a8024daf78fa47b7/codemp/server/sv_client.cpp#L1470-L1505), [`server.h:349-362`](https://github.com/taysta/TaystJK/blob/7d99efb4a77a174d1a7853f2a8024daf78fa47b7/codemp/server/server.h#L349-L362)), commits of 2019-05-03 and 2019-05-04. Same code as the proxy, including the same prefix comparisons (F3, F10.4). |
| Negative `fraglimit`/`timelimit` votes | TaystJK `SVFIXES_ALLOW_NEGATIVE_CALLVOTES`. The proxy covers it with its 0 minimum. |
| Name cleaning | OpenJK `ClientCleanName`. TaystJK runs the same function in the engine's `SV_UserinfoChanged`. |
| `\r`, `\n` and `;` in client commands | OpenJK `sv_filterCommands` (`SV_ExecuteClientCommand`), which replaces the characters, where the proxy drops the command |
| `userinfo`: empty argument ignored, flood check before the copy | OpenJK `SV_UpdateUserinfo_f` |
| q3infoboom (`getstatus`/`getinfo` argument longer than 128) | ioq3, Quake3e, OpenJK (commit [`64e20c5b`](https://github.com/JACoders/OpenJK/commit/64e20c5b)) |
| `Cmd_TokenizeString` | Quake 3 1.32 `Cmd_TokenizeString2` |
| `Com_Printf` into a bounded buffer | ioq3 `Q_vsnprintf` |
| `Q_IsColorStringExt` without `isdigit` | OpenJK [`cb9be4a5`](https://github.com/JACoders/OpenJK/commit/cb9be4a5) "Fixed crash when player uses non-ASCII character in name". The proxy's SDK already uses a range test ([q_color.hpp:17](../../src/sdk/qcommon/q_color.hpp#L17)). |

## Missing

| Item | Severity | Subject |
| --- | --- | --- |
| [O1](#o1-the-ip-userinfo-key-is-not-maintained-high) | High | `ip` userinfo key not maintained: IP bans can be avoided |
| [O2](#o2-rate-limiting-of-out-of-band-packets-high) | High | Rate limiting, see F6 |
| [O3](#o3-userinfo-sanitised-before-the-game-sees-it-on-connect-high) | High | Userinfo on connect, see F2 |
| [O4](#o4-writeconfig-medium) | Medium | `writeconfig`, see F7 |
| [O5](#o5-connections-per-ip-address-medium-optional) | Medium, optional | Connections per IP address |
| [O6](#o6-server-commands-longer-than-1022-characters-do-not-port-corrected) | Do not port | Server commands longer than 1022 characters |
| [O7](#o7-tokenizer-reads-its-bounded-copy-low) | Low | Tokenizer, see A4 |
| [O8](#o8-userinfo-sanitisation-bypassed-by-a-long-userinfo-high) | High | Userinfo sanitisation bypassed by a long userinfo |
| [O9](#o9-movement-events-that-lag-every-player-around-medium) | Medium | Movement events that lag every player around (ffa3) |
| [O10](#o10-forwardmove-of--128-low) | Low | `forwardmove` of -128 |
| [O11](#o11-msg_readbits-reads-past-the-message-low) | Low | `MSG_ReadBits` reads past the message |

### O1. The `ip` userinfo key is not maintained (High)

Status: **source only**.

- JKA sets `ip` once, in `SV_DirectConnect` ([`server/sv_client.cpp:292`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp#L292)), and never again. `SV_UserinfoChanged` does not touch it ([`server/sv_client.cpp`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp), `SV_UserinfoChanged`), and the `userinfo` command replaces the whole userinfo with the client's string (`SV_UpdateUserinfo_f`, line 1511). The proxy's rewrite does the same ([Proxy_sv_client.cpp:626-656](../../src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_client.cpp#L626-L656)).
- Quake 3 1.32 restores `ip` in `SV_UserinfoChanged` when it is missing (Quake 3 [`code/server/sv_client.c:1164-1177`](https://github.com/id-Software/Quake-III-Arena/blob/dbe4ddb10315479fc00086f08e25d968b4b43c49/code/server/sv_client.c#L1164-L1177)). JKA does not have this point-release fix.
- ioq3 and OpenJK always overwrite it, so a client cannot fake it (OpenJK [`codemp/server/sv_client.cpp:1125-1143`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/server/sv_client.cpp#L1125-L1143)). They also refuse a connect whose userinfo leaves no room for `ip` (OpenJK lines 194-205).
- The game reads `ip` in every `ClientConnect`, including the reconnect after each map change, to check bans and to store the address ([`game/g_client.c:2269-2278`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_client.c#L2269-L2278)).

Consequence: a client sends a `userinfo` with a fake `ip`, or without one. From the next map change on:

- `G_FilterPacket` checks the fake address, so a ban added with `addip` does not apply to them;
- the game logs and stores the fake address.

A connect userinfo too long to receive `ip` has the same effect from the start: `Info_SetValueForKey` prints "Info string length exceeded" and adds nothing.

Fix, in `Proxy_SV_UserinfoChanged` before calling the original, as OpenJK does:

```cpp
// The game's ban check reads "ip" from the userinfo: never trust the client's value
const char* ip = cl->netchan.remoteAddress.type == NA_LOOPBACK // what NET_IsLocalAddress tests
	? "localhost"
	: server.common.functions.NET_AdrToString(cl->netchan.remoteAddress);
const char* val = Info_ValueForKey(cl->userinfo, "ip");
const std::size_t len = val[0]
	? std::strlen(ip) - std::strlen(val) + std::strlen(cl->userinfo)
	: std::strlen(ip) + 4 + std::strlen(cl->userinfo);

if (len >= MAX_INFO_STRING)
{
	server.functions.SV_DropClient(cl, "userinfo string length exceeded");

	return;
}

Info_SetValueForKey(cl->userinfo, "ip", ip);
```

- `NET_AdrToString` is already in the proxy's engine table. `NET_IsLocalAddress` is not, hence the `NA_LOOPBACK` test.
- On connect, this hook runs after `GAME_CLIENT_CONNECT`. The F2 sanitiser, which runs in `GAME_CLIENT_CONNECT`, must refuse the client when `ip` is missing: return `"Userinfo string length exceeded."`.
- On connect, this hook runs while the slot is still `CS_FREE`, and `SV_DropClient` there has the same problem as in F2. It cannot happen: when `SV_DirectConnect` managed to add `ip`, the length check passes, and when it did not, the F2 check has already refused the client. So the drop only happens on a `userinfo` command, where it is safe.
- OpenJK path: OpenJK's engine already does it.

### O2. Rate limiting of out-of-band packets (High)

Same as F6 in [fixes.md](fixes.md), which compares the engines. Quake3e also limits `getchallenge` and `connect` per address ([`code/server/sv_client.c:130, 502`](https://github.com/ec-/Quake3e/blob/3173cef096dd0cb2fea26d91bce71049061f5a13/code/server/sv_client.c#L130)). JK2MV [`ee07c4b`](https://github.com/mvdevs/jk2mv/commit/ee07c4b) and OpenJK also expire buckets when the time wraps (`interval < 0`): keep that test when porting.

### O3. Userinfo sanitised before the game sees it on connect (High)

Same as F2 in [fixes.md](fixes.md). JK2MV and Quake3e call `SV_UserinfoChanged` before `GAME_CLIENT_CONNECT`.

### O4. `writeconfig` (Medium)

Same as F7 in [fixes.md](fixes.md). ioq3 and OpenJK ([`05928a57`](https://github.com/JACoders/OpenJK/commit/05928a57)) only accept `.cfg`, Quake3e has a denylist, JK2MV ([`fed4849`](https://github.com/mvdevs/jk2mv/commit/fed4849)) forces `.cfg`.

### O5. Connections per IP address (Medium, optional)

Status: **source only**. Only Quake3e has it: `sv_maxclientsPerIP`, default 3 ([`code/server/sv_client.c:502-520`](https://github.com/ec-/Quake3e/blob/3173cef096dd0cb2fea26d91bce71049061f5a13/code/server/sv_client.c#L502-L520), [`sv_init.c:755-757`](https://github.com/ec-/Quake3e/blob/3173cef096dd0cb2fea26d91bce71049061f5a13/code/server/sv_init.c#L755-L757)). JKA, ioq3, OpenJK and JK2MV have no limit, so one address can fill every slot with clients that complete the challenge (tools like q3fill).

Fix, in the F2 connect path, for a first connection of a non-bot:

1. Count the slots in state `CS_CONNECTED` or above whose `netchan.remoteAddress` has the same base address (`NET_CompareBaseAdr`), not counting the slot being connected.
2. When the count reaches the limit, return `"Too many connections."`.

Decision of the author: add it, as a new cvar `proxy_sv_maxClientsPerIP` (archived, any engine). Several players can share one address behind NAT, so it needs a setting: default `3` like Quake3e, `0` disables it.

### O6. Server commands longer than 1022 characters: do not port (corrected)

Status: **source only**.

- ioq3 and OpenJK drop a server command longer than 1022 characters in `SV_SendServerCommand` (fix for aluigi's q3msgboom). Quake 3 1.32 and JKA do not.
- Correction: do not port it. The retail game builds the `scores` command up to 1022 characters, then adds the `scores %i %i %i` prefix ([`game/g_cmds.c`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_cmds.c), `DeathmatchScoreboardMessage`). With many players the command goes past 1022 characters, and dropping it would remove the scoreboard. Today the engine truncates it to 1023 characters, which retail clients handle. OpenJK could add the drop because it also fixed its own `scores` builder ([`9d4cac81`](https://github.com/JACoders/OpenJK/commit/9d4cac81)).
- With the retail game, free text from players is already short: chat is cut to 150 characters by the game, names to 36, vote strings by the proxy.

### O7. Tokenizer reads its bounded copy (Low)

Same as A4 in [improvements.md](improvements.md). Quake3e does it.

### O8. Userinfo sanitisation bypassed by a long userinfo (High)

Status: **source only**. Found while comparing with OpenJK's `G_ValidateUserinfo`. The bug is in the proxy's SDK, which copies the original code.

How the info functions behave, in the SDK ([q_shared.cpp:104-205](../../src/sdk/qcommon/q_shared.cpp#L104-L205)) as in the retail game ([`game/q_shared.c`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/q_shared.c)):

- `Info_ValueForKey` finds the first key that matches **without case** (`Q_stricmp`).
- `Info_RemoveKey` removes only the first key that matches **with case** (`strcmp`).
- `Info_SetValueForKey` removes the key, then puts the new pair in front. When the result would reach `MAX_INFO_STRING`, it prints "Info string length exceeded" and **adds nothing, without telling the caller**.

The attack: a userinfo close to 1024 characters with a bad `Forcepowers` (capital F).

1. The proxy reads it: `Info_ValueForKey` ignores case. The value is bad.
2. `Info_SetValueForKey(userinfo, "forcepowers", default)` removes nothing, because `strcmp` does not match `Forcepowers`. Adding the new pair would overflow, so it adds nothing.
3. The proxy writes the userinfo back unchanged. The game reads `Forcepowers` without case, and parses the bad value.

The same works with two `forcepowers` keys: the first one is removed, the default does not fit, and the game then reads the second one. The model checks have the same hole.

What the other engines do:

- OpenJK's game validates the userinfo before using it ([`codemp/game/g_client.c:1958-2106`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_client.c#L1958-L2106), commits [`36fda213`](https://github.com/JACoders/OpenJK/commit/36fda213), [`ae2b3777`](https://github.com/JACoders/OpenJK/commit/ae2b3777), [`d63be8a7`](https://github.com/JACoders/OpenJK/commit/d63be8a7)): length, slashes, control characters, and the number of each known key, counted without case. A key present twice is refused. `cl_guid` and `cl_punkbuster` are refused too, against q3fill. It drops the client on failure.
- JK2MV makes `Info_SetValueForKey` return a result, and drops the client with "userinfo string length exceeded" when it fails ([`src/server/sv_client.cpp:1130-1135`](https://github.com/mvdevs/jk2mv/blob/7d601454c3db68492289d4d4e3dc30bff39e4246/src/server/sv_client.cpp#L1130-L1135)).

Fix, in the F2 sanitiser:

1. Before reading anything, count the keys that the proxy checks (`name`, `model`, `forcepowers`, `ip`) without case. Refuse the client when one of them appears more than once.
2. Make `Info_SetValueForKey` in the SDK return `bool`, and refuse the client when it fails.
3. Decision of the author: port OpenJK's whole `G_ValidateUserinfo` ([`codemp/game/g_client.c:1958-2106`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_client.c#L1958-L2106)), with OpenJK's default checks (`g_userinfoValidate` `25165823`: every key count, size, slashes and control characters; extended ASCII not checked). It must stay compatible with the retail client. Checked in the jediacademy source:
   - The retail client sends every key that OpenJK requires (`minCount` 1) with its default settings: `name`, `rate`, `snaps`, `model`, `forcepowers`, `color1`, `color2`, `handicap`, `cg_predictItems`, `saber1`, `saber2`, `char_color_red`, `char_color_green`, `char_color_blue` are all `CVAR_USERINFO` cvars with a non-empty default.
   - Its other userinfo keys (`sex`, `teamtask`, `teamoverlay`, `password`, `cl_anonymous`, `cg_pmove_fixed`…) are either optional in OpenJK's table or not in it, and keys outside the table are not checked.
   - It never sends `cl_guid` or `cl_punkbuster`, which the table forbids.
   - Limit: a player who empties one of the required cvars, for example `/color1 ""`, loses that key, because an empty value is not stored in the userinfo. That player is then refused. OpenJK servers have run with the same rule since 2014.
   - Skip bots, as in F2: their userinfo comes from the bot files, and has no `forcepowers` key.

"Refuse" means returning the reason from `GAME_CLIENT_CONNECT`, and `DropClient` on a userinfo change (F2).

### O9. Movement events that lag every player around (Medium)

Status: **source only**. This is the todo item "ffa3 crash exploit with events (look at `SV_AddEntitiesVisibleFromPoint`)". The author knows the bug and has reproduced it. Decision: fix it.

Only TaystJK (EternalJK) fixes it. OpenJK and JK2MV do not: their servers have no filter on event entities, and OpenJK's game sends the pending events exactly as the retail game does (`SendPendingPredictableEvents` is unchanged).

TaystJK commit [`e0d767299`](https://github.com/taysta/TaystJK/commit/e0d767299) (2019-05-03), "Fix obscure staff backwards DFA exploit":

> Player with high fps could lag every player in their PVS by doing this move in a crawlspace-like area. Was caused by pmove code redundantly adding many movement-generated events causing the snapshots to come out large enough that they were rate limited.

Its fix, in `SV_AddEntitiesVisibleFromPoint` (TaystJK [`codemp/server/sv_snapshot.cpp:410, 488-505`](https://github.com/taysta/TaystJK/blob/7d99efb4a77a174d1a7853f2a8024daf78fa47b7/codemp/server/sv_snapshot.cpp#L410)):

- skip the event entities `EV_JUMP`, `EV_FALL` and `EV_FOOTSTEP`;
- send at most 16 landing effects (`EV_PLAY_EFFECT` with `EFFECT_WATER_SPLASH` to `EFFECT_LANDING_GRAVEL`) per snapshot.

Why these entities exist: after each usercmd (`ClientThink_real`), the retail game puts the player's next pending predictable event inside the player's own entity, and sends the one after it as a temporary event entity for the other players (`SendPendingPredictableEvents`, [`game/g_active.c:1059`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_active.c#L1059), called at lines 3434 and 3869). So such an entity only exists when one usercmd produced two events. The exploit makes pmove produce many per usercmd, and a high-FPS client sends many usercmds.

Things to know before porting it:

- No gameplay change: the game applies the events itself (fall damage, for example) from the player state, before anything is sent. Movement, hits and damage do not go through these entities. The filter only changes which sound and effect events the other clients receive.
- Skipping `EV_JUMP`/`EV_FALL`/`EV_FOOTSTEP` entities also drops the legitimate ones: the second movement event of a single usercmd, for example a landing and a footstep in the same 8 ms usercmd. Other players then miss that one sound. Almost never in normal play, systematic with the exploit.
- The landing-effect cap only triggers beyond 16 per snapshot.
- In the proxy it needs a hook on `SV_AddEntitiesVisibleFromPoint` (a rewrite of that function), or on the function that adds one entity to the snapshot.

### O10. `forwardmove` of -128 (Low)

Status: **source only**. ioq3 [`89f7863`](https://github.com/ioquake/ioq3/commit/89f7863) (bug 5273, "exploit resolved, we're now disallowing forwardmove of -128 and vice versa"), merged in OpenJK [`cc718e6e`](https://github.com/JACoders/OpenJK/commit/cc718e6e). JKA's `MSG_ReadDeltaUsercmdKey` does not have it.

The fix turns `forwardmove`, `rightmove` and `upmove` of -128 into -127, so that negative moves are never longer than positive ones. In the proxy it is three lines in `Proxy_SharedAPI_GetUsercmd`, next to the roll fix.

"Feeling": a normal key press gives ±127, so only an unusual input changes. Mouse movement with `+strafe` can reach -128, and would then move 1/128 slower.

### O11. `MSG_ReadBits` reads past the message (Low)

Status: **source only**. CVE-2017-11721, fixed in ioq3 and OpenJK ([`bf403f7e`](https://github.com/JACoders/OpenJK/commit/bf403f7e)). JKA's `MSG_ReadBits` has no bound at all ([`qcommon/msg.cpp`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/msg.cpp)).

In JKA the receive buffer is a local `bufData[MAX_MSGLEN]` of `Com_EventLoop` ([`qcommon/common.cpp:924`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/common.cpp#L924)), so a read past the end reads the stack, not unmapped memory. The fix needs a hook on `MSG_ReadBits` and on the Huffman read, two hot functions. Low priority.

## Q3/JKA differences: nothing to do

| Fix in another engine | Why nothing to do |
| --- | --- |
| Fragment length checks in `Netchan_Process` | Already in JKA ([`qcommon/net_chan.cpp:318-319`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/net_chan.cpp#L318-L319)) |
| `Huff_Decompress` bounds | Already in JKA ([`qcommon/huffman.cpp:325-339`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/huffman.cpp#L325-L339)), same as ioq3 |
| `MSG_ReadString` turns `%` into `.` | Already in JKA ([`qcommon/msg.cpp`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/msg.cpp)) |
| `cmdCount` checks in `SV_UserMove` | Already in JKA ([`server/sv_client.cpp:1689-1697`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/server/sv_client.cpp#L1689-L1697)) and in the proxy |
| Quake3e: `connect` packets longer than `MAX_INFO_STRING*2` dropped before `Huff_Decompress` | Optional: JKA's `Huff_Decompress` is already bounded |
| Quake3e, OpenJK ([`0755fcdd`](https://github.com/JACoders/OpenJK/commit/0755fcdd)): `ipAuthorize` removed | JKA still has the authorize code. The proxy ignores the packet, and the F1 filter drops it. |
| OpenJK: `isJediMaster` reset when a spectator stops following ([`codemp/game/g_cmds.c:958`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_cmds.c#L958)) | The retail game turns the Jedi Master gametype into FFA ([`game/g_main.c:833-836`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_main.c#L833-L836)) |
| OpenJK: loop guard in `Cmd_FollowCycle_f` ("team follow1 crash", [`codemp/game/g_cmds.c:1460`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_cmds.c#L1460)) | The infinite loop needs a negative `spectatorClient`, which only `team follow1`/`team follow2` set, and the proxy blocks them |
| OpenJK [`cb9be4a5`](https://github.com/JACoders/OpenJK/commit/cb9be4a5): `isdigit` in `Q_IsColorStringExt` | Already a range test in the proxy's SDK. The retail game's `Q_IsColorString` has no `isdigit` either ([`game/q_shared.h:1147`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/q_shared.h#L1147)). |
| JK2MV [`fd892ce`](https://github.com/mvdevs/jk2mv/commit/fd892ce): `client > sv_maxclients` instead of `>=` in bot syscalls | Indexes come from the game module, not from players |
| JK2MV [`f07f5a8`](https://github.com/mvdevs/jk2mv/commit/f07f5a8), [`8a91d58`](https://github.com/mvdevs/jk2mv/commit/8a91d58), [`e2ebea4`](https://github.com/mvdevs/jk2mv/commit/e2ebea4): syscall arguments checked against VM memory | QVM sandboxing. The retail jampgame is a native DLL. |
| JK2MV `mv_fixgalaking`, `mv_fixbrokenmodels` | JK2 models (`galak_mech`, `kyle/fpls`, `morgan`) |

## Game-module bugs out of the proxy's reach

OpenJK fixed these in its own game code. The proxy keeps the retail game DLL untouched, so it can only filter commands and userinfo, or read the shared structures (`sharedEntity_t`, `playerState_t`).

| OpenJK fix | Why the proxy cannot simply do it |
| --- | --- |
| [`962b3db4`](https://github.com/JACoders/OpenJK/commit/962b3db4) "Fixed the infamous saber lock crash" ([`codemp/game/w_saber.c`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/w_saber.c): `saberNum` used instead of `rSaberNum`) | A bug inside game code. Only a memory patch of the retail game DLL would fix it. |
| [`3c1257cf`](https://github.com/JACoders/OpenJK/commit/3c1257cf) "Fixed fast teamswitch bypass exploit with follow commands" | Needs `switchTeamTime` and the spectator state, which live in `gclient_t` after `playerState_t`. Their offsets in the retail DLL are unknown. |
| [`b200ec70`](https://github.com/JACoders/OpenJK/commit/b200ec70) (from ioq3 [`82f79897`](https://github.com/ioquake/ioq3/commit/82f79897)): duel reconnect exploit | Needs the game's `level` state (intermission) |

## To check on JKA

JK2MV fixes JK2 game bugs from the engine. JKA's game code is related, so some of these bugs may exist in JKA too. None of them was checked.

| JK2MV fix | What it does | Where |
| --- | --- | --- |
| `mv_fixturretcrash` | Discards `EV_SABER_BLOCK` temporary entities beyond 100 per second, so that a turret or sentry cannot exhaust the entities and crash the server | [`src/server/sv_world.cpp:205-217`](https://github.com/mvdevs/jk2mv/blob/7d601454c3db68492289d4d4e3dc30bff39e4246/src/server/sv_world.cpp#L205-L217) |
| `mv_fixsaberstealing` | Sets `saberEntityNum` to `ENTITYNUM_NONE` for spectators every frame | [`src/server/sv_main.cpp:1124-1137`](https://github.com/mvdevs/jk2mv/blob/7d601454c3db68492289d4d4e3dc30bff39e4246/src/server/sv_main.cpp#L1124-L1137) |
| `mv_fixnamecrash` | Keeps only ASCII and a few accented characters in names. The proxy's name cleaning removes control characters and some Windows-1252 codes, but keeps the rest of the extended range. | [`src/server/sv_client.cpp:1058-1084`](https://github.com/mvdevs/jk2mv/blob/7d601454c3db68492289d4d4e3dc30bff39e4246/src/server/sv_client.cpp#L1058-L1084) |

Whether a JKA version is needed depends on each bug existing in the retail JKA game. That has to be tested on a server, or read in `jediacademy/codemp/game` first.
