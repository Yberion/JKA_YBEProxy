# Fixes

Severity: Critical, High, Medium, Low. Paths are relative to the repository root.

Each item has a verification status:

- **Confirmed (binary)**: checked in the jediacademy source and in the retail game modules, Linux and Windows (see [README.md](README.md#retail-binaries)).
- **Confirmed (source)**: checked in the jediacademy source only. That source is close to retail, but not identical.
- **Corrected**: the first pass was wrong or incomplete. The text below is the corrected one.
- **Refuted**: the claim was wrong. It stays here, marked, so that nobody adds it back.

Source references link to GitHub, pinned to the commit that was read, so that the line numbers stay right. A path without a repository root, such as `game/g_cmds.c:1835`, is in [jediacademy](https://github.com/jedis/jediacademy) `codemp`. See the source list in [README.md](README.md#sources).

Every fix below is in the proxy: the retail game module is not rebuilt. Nothing below is compiled or tested.

## F1. Stack overflow in `G_LogPrintf` with a long command at intermission (Critical)

Status:

- Overflow: **confirmed (binary)**. Not reproduced on a server.
- Severity: **corrected**. The first pass said High. The write reaches the return address with text chosen by the player, and any player can cause it at every map end.
- Other caller `ClientUserinfoChanged: %i %s`: **corrected**, not a reliable trigger with the proxy (see below).

### Cause

[`G_LogPrintf`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_main.c#L2213-L2240) writes the time in a `char string[1024]`, then `vsprintf( string + 7, fmt, argptr )` with no size. It does so even when there is no log file: the test of `level.logFile` comes after.

At intermission, [`ClientCommand`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_cmds.c#L3254-L3362) turns any command outside a fixed list (`give`, `kill`, `team`, `callvote`, `gc`…) into public chat: `Cmd_Say_f( ent, qfalse, qtrue )`. With `arg0` set, [`Cmd_Say_f`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_cmds.c#L1695-L1711) takes [`ConcatArgs( 0 )`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_cmds.c#L127-L152), the whole command line, and passes it to [`G_Say`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_cmds.c#L1618-L1688). `G_Say` logs it before it cuts it to 150 characters: `G_LogPrintf( "say: %s: %s\n", netname, chatText )` (line 1635).

`say`, `say_team` and `tell` are handled before the intermission test, and the proxy already drops them above 256 characters. Any other command name is not limited.

Size: the engine reads a client command into a 1024-byte buffer ([`MSG_ReadString`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/msg.cpp#L459-L460)), and `ConcatArgs` stops at 1022 characters. From the start of `string`: 7 (time) + 5 (`say: `) + name (35 at most) + 2 (`: `) + 1022 + 1 (`\n`) + 1 (terminator) = 1073 bytes, 49 past the end. With a 1-character name it is still 15 bytes past the end.

`G_LogPrintf` then calls `G_Printf( "%s", string + 7 )` when `g_dedicated` is set. [`G_Printf`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_main.c#L703-L712) does another `vsprintf` in 1024 bytes: a second overflow. `G_Error`, `Com_Error` and `Com_Printf` of the game have the same pattern, but no player-controlled text of that length reaches them.

### Retail binary

`G_LogPrintf__FPCce` at `0x880c4`:

- `sub esp, 0x420`. `string` is at `ebp-0x408`, 0x400 bytes, so it ends at `ebp-0x8`. The saved `edi` is at `ebp-0x8`, the saved `ebp` at `ebp`, the return address at `ebp+4`.
- `0x88140`: `Com_sprintf( string, 0x400, ... )`.
- `0x8815c`: `vsprintf( ebp-0x401, fmt, args )`, three arguments, no size.

A write of 1073 bytes from `ebp-0x408` ends at `ebp+0x29`: it overwrites the saved registers and the return address with the player's text. A crash is certain when the function returns. Control of the execution is plausible, not checked.

`ClientCommand__Fi` at `0x130024`, intermission branch: `0x1304c1` to `0x1304d4` calls `Cmd_Say_f` (`0x12c8f4`, static, no symbol) with `(ent, 0, 1)`. `Cmd_Say_f` calls `ConcatArgs(0)` at `0x12c92b`.

`G_Printf__FPCce` at `0x85ce4`: `text` at `ebp-0x400`, `vsprintf` at `0x85d04`, no size.

Windows, `G_LogPrintf` at `0x2008fd10` (found from its `%3i:%i%i ` string):

- `sub esp, 0x400 ; push esi`: `string` is at `esp+4`, 0x400 bytes, and the return address comes right after it. No `/GS` cookie.
- `0x2008fd8e`: `vsprintf( string + 7, fmt, args )`, three arguments, no size.

Any write past the 1024 bytes reaches the return address: with a 1-character name the text already overwrites it.

Windows, intermission branch of `ClientCommand` (`0x2007fed5`, after the test of the `CANNOT_TASK_INTERMISSION` list): `Cmd_Say_f` is inlined. It calls `ConcatArgs(0)` (`0x2007b3a0`), then `G_Say( ent, NULL, 0, text )` (`0x2007d240`), which calls `G_LogPrintf` with `say: %s: %s\n` at `0x2007d2a6`.

### Who can trigger it

Any client in the game, spectators included, during every intermission. Defaults are enough.

### Other caller: `ClientUserinfoChanged: %i %s`

With `g_logClientInfo 1` (default 0), [`ClientUserinfoChanged`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_client.c#L2231-L2234) logs its `CS_PLAYERS` string. With the proxy's userinfo validation, the required keys take about 210 of the 1023 characters, and that string only repeats part of the userinfo. By my count it stays under about 985 bytes with the prefix. Not a reliable trigger. F5 adds margin anyway.

### Other engines

All of them bound the write:

- OpenJK [`g_main.c:1506-1523`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_main.c#L1506-L1523) (`Q_vsnprintf`). It also cuts `say` to `MAX_SAY_TEXT` ([`g_cmds.c:1652-1666`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_cmds.c#L1652-L1666)) and answers `Unknown command` instead of turning it into chat ([`g_cmds.c:3453-3458`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_cmds.c#L3453-L3458)).
- TaystJK [`g_main.c:1667-1684`](https://github.com/taysta/TaystJK/blob/7d99efb4a77a174d1a7853f2a8024daf78fa47b7/codemp/game/g_main.c#L1667-L1684), JKA_Anti-cheat [`g_main.c:1656-1673`](https://github.com/Yberion/JKA_Anti-cheat/blob/48900270370c2a5275edaf72bbc4b7783c556502/codemp/game/g_main.c#L1656-L1673), JK2MV [`g_main.c:1678-1693`](https://github.com/mvdevs/mvsdk/blob/4c7c463638192aead1ff3de8a88cc5de266bf3a2/code/game/g_main.c#L1678-L1693), ioq3 [`g_main.c:1109-1124`](https://github.com/ioquake/ioq3/blob/87a99d18206e878a20289f05ac3c16005dcae5c3/code/game/g_main.c#L1109-L1124): `Q_vsnprintf`.

### Proxy fix

In `Allowed` ([ClientCommands.cpp:106-120](https://github.com/Yberion/JKA_YBEProxy/blob/ae62510d6857be29351812da153aceafe919ab11/src/JKA_YBEProxy/Features/ClientCommands.cpp#L106-L120)), drop any command line (name, space and arguments) longer than 512 characters, right after `argsConcat` is computed:

```cpp
// Longest command line (name, space and arguments) a client may send to the game. The game logs a command line in a
// buffer of 1024 bytes: at intermission it turns every command it does not know into chat, and logs it whole
static constexpr std::size_t maxCommandLength = 512;

if (std::strlen(cmd) + 1 + std::strlen(argsConcat) > maxCommandLength)
{
	return false;
}
```

- The overflow starts at about 970 characters: 512 leaves a wide margin.
- No command of the base game needs that much. Chat is cut at 150, and the proxy already drops `say` over 256.
- Another option would be to drop only unknown commands at intermission, as OpenJK does. The proxy would have to know when the game is in intermission. The single limit is simpler and covers any other path.

### Applied

As written above, in `Allowed` ([ClientCommands.cpp](../../src/JKA_YBEProxy/Features/ClientCommands.cpp)): a command line longer than 512 characters is dropped without a message, after the anti-cheat and mute checks and before the other ones. Not compiled.

## F2. Crash with `callvote clientkick <n>` (High)

Status: **confirmed (binary)**. Not reproduced on a server.

### Cause

[`Cmd_CallVote_f`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_cmds.c#L2084-L2101), `clientkick` branch: the number is only compared with `MAX_CLIENTS` (32, line 2088), then `g_entities[n].client->pers.connected` is read (line 2094).

[`G_InitGame`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_main.c#L978-L989) clears `g_entities`, then sets `client` only for `i < level.maxclients`. From `sv_maxclients` to 31, `client` is `NULL`: reading it crashes the server.

### Retail binary

`Cmd_CallVote_f__FP9gentity_s` at `0x12cfc4`:

- `0x12d45a`: `atoi`, then `0x12d467` `cmp eax, 0x20 ; jge` (invalid number).
- `0x12d470` `imul edi, eax, 0x5ec`, `0x12d476` `mov ecx, [edi + g_entities + 0x360]` (`client`).
- `0x12d47c` `mov edx, [ecx + 0x610]` (`pers.connected`), with no test of `ecx`. A `NULL` client reads address `0x610`.

Windows, `clientkick` branch of `Cmd_CallVote_f` (found from its `invalid client number` string): `0x2007dd97` `cmp eax, 0x20 ; jge`, then `0x2007dda4` `mov edx, [ecx + 0x20631340]` (`g_entities[n].client`) and `0x2007ddb0` `mov ecx, [edx + 0x610]`, with no test of `edx`. Same crash.

### Who can trigger it

Any player who can vote, with the defaults:

- `g_allowVote` is 1 by default ([`g_main.c:380`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_main.c#L380));
- not a spectator, except in duel and power duel, where spectators can vote ([`g_cmds.c:1997-2004`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_cmds.c#L1997-L2004));
- no vote in progress, not at intermission;
- `sv_maxclients` below 32.

### Other engines

OpenJK compares with `level.maxclients` ([`G_VoteClientkick`, g_cmds.c:1886-1897](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_cmds.c#L1886-L1897)).

### Proxy fix

In the `callvote` block of `Allowed` ([ClientCommands.cpp:157-215](https://github.com/Yberion/JKA_YBEProxy/blob/ae62510d6857be29351812da153aceafe919ab11/src/JKA_YBEProxy/Features/ClientCommands.cpp#L157-L215)), after the length check:

```cpp
// The game only compares the number with MAX_CLIENTS, then reads g_entities[n].client, which is NULL from
// sv_maxclients up to MAX_CLIENTS - 1
if (!Q_stricmp(cmd_arg1, "clientkick") &&
	(cmdArg2NumberValue < 0 || cmdArg2NumberValue >= proxy.trap->Cvar_VariableIntegerValue("sv_maxclients")))
{
	return false;
}
```

- `atoi`, as the game does.
- `sv_maxclients` is latched: its value only changes at the map change, when the game reads it again. The `gc` check already reads it the same way.
- The vote is dropped without a message. The game would print `invalid client number`.

### Applied

Written with the range check that the other votes of `Allowed` already use ([ClientCommands.cpp](../../src/JKA_YBEProxy/Features/ClientCommands.cpp)): `clientkick` accepts `0` to `sv_maxclients - 1`, the same rule as the sketch above. Added to the vote table of [features.md](../features.md#client-commands). Not compiled.

## F3. Crash with `gc <player> 7` (High on Linux)

Status:

- Bug: **confirmed (binary)** on Linux and Windows. **Reproduced** by the author on both, without the fix.
- Severity: **corrected**. The first pass said Medium, "crash or memory sent as text". It depends on what the compiler put after the array:
  - Linux retail server: the entry is not a pointer. The author reproduced the crash.
  - Windows retail server (1.0.1.0), tested by the author without the fix: `gc 1 7` sends `Free For All` to the player, no crash. `Free For All` is the first entry of `gameNames`, the array that follows `gc_orders` in `g_cmds.c`: the Windows DLL keeps both arrays one after the other.

### Cause

[`Cmd_GameCommand_f`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_cmds.c#L1812-L1840) tests `order > sizeof(gc_orders)/sizeof(char *)` (line 1835). The array has 7 entries, so 7 passes, and `gc_orders[7]`, one pointer past the end, goes to `G_Say` as the text.

### Retail binary

`Cmd_GameCommand_f__FP9gentity_s` at `0x12cc34`:

- `0x12cca5` `test esi, esi ; jl` and `0x12cca9` `cmp esi, 0x20 ; jge`: the player must be 0 to 31.
- `0x12ccae` `test ebx, ebx ; jl` and `0x12ccb2` `cmp ebx, 7 ; jbe`: the order 7 passes.
- `0x12ccd7` `mov esi, [ebx*4 + 0x1d31a4]`: `gc_orders` is at `0x1d31a4`.

`0x1d31a4` to `0x1d31bc` hold the 7 relocated pointers. `0x1d31c0` holds `0x6e697270`, with no relocation: the bytes `"prin"`, start of the string stored after the array, `print "there is no client named '%s' currently on the server.\n"` (this module has no `.rodata`: gcc put the strings in `.data`). `G_Say` copies the text with `Q_strncpyz` and reads at `0x6e697270`, an address the process does not map in normal use. The server crashes.

Windows: `gc_orders` is at `0x2010ae5c` (found from `hold your position`). The entry after the 7 orders, `0x2010ae78`, is the first pointer of `gameNames`, to `Free For All`. That is what the author's test showed: no crash, the text `Free For All` sent to the player.

### Who can trigger it

Any client in the game, any team, outside intermission. The player number must be below `sv_maxclients`, which the proxy already enforces.

### Other engines

- OpenJK ([`g_cmds.c:1804-1819`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_cmds.c#L1804-L1819)) and TaystJK ([`g_cmds.c:2572-2588`](https://github.com/taysta/TaystJK/blob/7d99efb4a77a174d1a7853f2a8024daf78fa47b7/codemp/game/g_cmds.c#L2572-L2588)): `order >= numgc_orders`, `order` unsigned.
- JK2MV still has the bug ([`g_cmds.c:1407`](https://github.com/mvdevs/mvsdk/blob/4c7c463638192aead1ff3de8a88cc5de266bf3a2/code/game/g_cmds.c#L1407)).

### Proxy fix

Replace the `gc` check of `Allowed` ([ClientCommands.cpp:128-132](https://github.com/Yberion/JKA_YBEProxy/blob/ae62510d6857be29351812da153aceafe919ab11/src/JKA_YBEProxy/Features/ClientCommands.cpp#L128-L132)):

```cpp
// Number of entries of gc_orders in the game
static constexpr int gcOrdersCount = 7;

// The player must be a client slot, and the order an index of gc_orders: the game also accepts 7, one past the end
if (!Q_stricmp(cmd, "gc") &&
	(atoi(cmd_arg1) >= proxy.trap->Cvar_VariableIntegerValue("sv_maxclients") ||
	 atoi(cmd_arg2) < 0 || atoi(cmd_arg2) >= gcOrdersCount))
{
	return false;
}
```

The first pass also dropped a negative player. Not needed: the game already returns on a negative player (`0x12cca5`).

### Applied

The existing `gc` check of `Allowed` ([ClientCommands.cpp](../../src/JKA_YBEProxy/Features/ClientCommands.cpp)) also drops an order of `gcOrdersCount` (7) or more. A negative order is not tested: the game already refuses it (`0x12ccae`). Not compiled.

## F4. `forcepowers` with 16 or 17 digits of powers (Medium)

Status:

- Bug: **confirmed (binary)**. The crash depends on the stack content: not reproduced.
- Gap in the proxy check: **confirmed** (all forms counted).

### Cause

[`BG_LegalizedForcePowers`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/bg_misc.c#L439-L566) declares `int final_Powers[NUM_FORCE_POWERS]` without a value (line 452). The read loop (lines 503-511) stops at the end of the string. With fewer than 18 digits after the second `-`, the last entries keep what was on the stack. They are then used as a loop counter and as the column of `bgForcePowerCost[i][countDown]` (lines 543-566): a large value reads far past the array, and the server crashes.

The proxy check ([Userinfo.cpp:313-370](https://github.com/Yberion/JKA_YBEProxy/blob/ae62510d6857be29351812da153aceafe919ab11/src/JKA_YBEProxy/Features/Userinfo.cpp#L313-L370)) accepts 22 to 24 characters, digits, and exactly two `-` at positions 1 to 5, not next to each other. That is 18 forms, and 8 of them have fewer than 18 digits:

- 22 characters with the second `-` at position 4 (17 digits) or 5 (16 digits): 2 + 3 forms;
- 23 characters with the second `-` at position 5 (17 digits): 3 forms.

Example of the shape: `123-1-` followed by 16 digits. Entries 16 and 17 (`FP_SABER_DEFENSE`, `FP_SABERTHROW`) are then not set. They are neutral powers, so the side check does not reset them to 0.

A digit from 4 to 9 is also used as a column before it is cut to 3 (line 553): it reads at most 20 bytes past the end of `bgForcePowerCost`, in `.data`. No crash.

### Retail binary

`BG_LegalizedForcePowers__FPci8qbooleanN32` at `0xea224`: `final_Powers` is at `ebp-0xf8`. The first write to it is the read loop at `0xea400`. Nothing sets it before: no `memset`, no store.

Windows, `BG_LegalizedForcePowers` at `0x2004a030` (found from its `7-1-032330000000001333` string): `final_Powers` is at `esp+0x20`, right below `powerBuf`. The first write to it is the read loop at `0x2004a206`. Nothing sets it before.

### Other engines

- OpenJK ([`bg_misc.c:429`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/bg_misc.c#L429)), TaystJK ([`bg_misc.c:570`](https://github.com/taysta/TaystJK/blob/7d99efb4a77a174d1a7853f2a8024daf78fa47b7/codemp/game/bg_misc.c#L570)) and JKA_Anti-cheat ([`bg_misc.c:537`](https://github.com/Yberion/JKA_Anti-cheat/blob/48900270370c2a5275edaf72bbc4b7783c556502/codemp/game/bg_misc.c#L537)): `final_Powers[NUM_FORCE_POWERS] = {0}`.
- OpenJK also stops reading at any character outside `0` to `3` ([`bg_misc.c:478-487`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/bg_misc.c#L478-L487)).
- JK2MV has the same uninitialised array ([`bg_misc.c:187`](https://github.com/mvdevs/mvsdk/blob/4c7c463638192aead1ff3de8a88cc5de266bf3a2/code/game/bg_misc.c#L187)).

### Proxy fix

In the `forcepowers` check, remember where the powers start and require 18 digits:

```cpp
std::size_t powersStart = 0;

// in the loop, on each '-'
powersStart = i + 1;

// after the test of seps
// The game reads NUM_FORCE_POWERS digits after the second '-'. With fewer, BG_LegalizedForcePowers keeps the missing
// entries uninitialised and uses them as an index
else if (len - powersStart < static_cast<std::size_t>(NUM_FORCE_POWERS))
{
	badForce = qtrue;
}
```

Decision of the author: also refuse a power digit outside `0` to `3`, as OpenJK. In the loop, after the second `-`:

```cpp
// A level above 3 is used as a column of bgForcePowerCost before the game cuts it to 3. OpenJK stops at such a digit
if (seps == 2 && (forcePowers[i] < '0' || forcePowers[i] > '3'))
{
	badForce = qtrue;
	break;
}
```

The retail client always sends `<rank>-<side>-` followed by 18 digits from `0` to `3` (22 characters): nothing legitimate is refused.

### Applied

As written above, in the `forcepowers` check of `Sanitize` ([Userinfo.cpp](../../src/JKA_YBEProxy/Features/Userinfo.cpp)): at least 18 digits after the second `-`, and each of them from `0` to `3`. The rank and side digits are not limited, as before: the game replaces the rank and checks the side itself. Checked with a port of the rule on every shape of 22 to 24 characters (10 accepted, all with 18 digits or more), and on the 37 `forcepowers` of the base bots (`botfiles/*.jkb` in `assets1.pk3`), which all pass. Not compiled.

## F5. `color1` and `color2` fill the gamestate of every client (Medium)

Status:

- Bug: **confirmed (source)**.
- Impact: **corrected**. All clients are disconnected, not only the new ones.
- Proxy fix of the first pass: **corrected**. It removes the TaystJK and EternalJK cosmetics.

### Cause

[`ClientUserinfoChanged`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_client.c#L2187-L2215) copies `color1` and `color2` as they are (`strcpy` into `c1[MAX_INFO_STRING]`, line 1897, no overflow) into the `CS_PLAYERS` configstring of the player. That configstring is part of the gamestate of every client.

The client keeps all configstrings in 16000 characters ([`MAX_GAMESTATE_CHARS`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/q_shared.h#L2046)). On each configstring change it rebuilds them, and above the limit it stops with `ERR_DROP` "MAX_GAMESTATE_CHARS exceeded" ([`client/cl_cgame.cpp:320-345`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/client/cl_cgame.cpp#L320-L345)). A new client stops the same way when it loads the gamestate ([`client/cl_parse.cpp:614`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/client/cl_parse.cpp#L614)). Once the total passes 16000, every connected client is dropped at the next change, and nobody can join.

### Who can trigger it

Several players together. With the proxy's validation, the required keys take about 210 of the 1023 characters of the userinfo, which leaves about 800 for the two colors per player. The number of players needed depends on the other configstrings (`systeminfo` with the pk3 list, models, sounds): about 10 to 15 by estimate, so 4 to 5 addresses with `proxy_sv_maxClientsPerIP 3`. Not measured on a server.

### What the clients read

- Retail cgame: `atoi` only, a saber color ([`cgame/cg_players.c:1554-1562`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/cgame/cg_players.c#L1554-L1562)). The first pass cited it from memory: it is right.
- TaystJK, and EternalJK that it comes from: the color, then the name of a hat in `color1` and of a cape in `color2`, as `<color><name>`. The cgame reads the color with `atoi` and the name after the digits ([`cg_players.c:2088-2140`](https://github.com/taysta/TaystJK/blob/7d99efb4a77a174d1a7853f2a8024daf78fa47b7/codemp/cgame/cg_players.c#L2088-L2140)). The UI writes it ([`ui_main.c:6790`](https://github.com/taysta/TaystJK/blob/7d99efb4a77a174d1a7853f2a8024daf78fa47b7/codemp/ui/ui_main.c#L6790)). A name is 13 characters at most ([`cg_local.h:327`](https://github.com/taysta/TaystJK/blob/7d99efb4a77a174d1a7853f2a8024daf78fa47b7/codemp/cgame/cg_local.h#L327)).

### Other engines

- OpenJK cuts each color to 15 characters (`color1[16]`, [`g_client.c:2115`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_client.c#L2115) and [`2307-2308`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_client.c#L2307-L2308)).
- TaystJK cuts them to 63 (`MAX_QPATH`, [`g_client.c:2690-2691`](https://github.com/taysta/TaystJK/blob/7d99efb4a77a174d1a7853f2a8024daf78fa47b7/codemp/game/g_client.c#L2690-L2691)).

### Proxy fix

The first pass replaced a color longer than 3 characters by its `atoi` value. That removes the hat and the cape of TaystJK and EternalJK players. Recommended instead: cut at 15 characters, as OpenJK. It keeps the cosmetics (1 digit + 13 characters) and bounds the two colors to 30 characters per player. In `Sanitize` ([Userinfo.cpp](https://github.com/Yberion/JKA_YBEProxy/blob/ae62510d6857be29351812da153aceafe919ab11/src/JKA_YBEProxy/Features/Userinfo.cpp#L313)), before the `forcepowers` check:

```cpp
// color1 and color2 go as they are into the CS_PLAYERS configstring of the player, which is part of the gamestate of
// every client (16000 characters at most). Same limit as OpenJK: TaystJK and EternalJK put a hat or a cape name after
// the color
for (const char* colorKey : { "color1", "color2" })
{
	val = Info_ValueForKey(userinfo, colorKey);

	if (std::strlen(val) > 15)
	{
		char color[16];

		Q_strncpyz(color, val, sizeof(color));

		if (!Info_SetValueForKey(userinfo, colorKey, color))
		{
			return "Userinfo string length exceeded.";
		}
	}
}
```

Decision of the author: 15 characters, as OpenJK.

### Applied

As written above, in `Sanitize` ([Userinfo.cpp](../../src/JKA_YBEProxy/Features/Userinfo.cpp)), before the `forcepowers` check. The keys are walked through a `static const char* const` array rather than a braced list, which would need `<initializer_list>`. Not compiled.

## F6. Vote parsing and name lookup of the game (Low, not security)

Status:

- Both bugs: **confirmed (source)**.
- Explanation of the vote bug: **corrected**.

### F6.1 Vote parsing

[`Cmd_Vote_f`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_cmds.c#L2163-L2189) and [`Cmd_TeamVote_f`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_cmds.c#L2370-L2401) test `msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1'`, a typo kept from Quake 3.

Correction: the first pass said that `msg[1]` is not set when the argument is one character long. It is set: `trap_Argv` ends the string, so `msg[1]` is `\0`. `msg[1]` is only read unset when the argument is missing (`vote` alone), and that read is one byte of the stack, no crash.

Effect: `vote Y` and `vote 1` count as no. The UI sends `vote yes` and `vote no`, which work.

ioq3 ([`g_cmds.c:1453`](https://github.com/ioquake/ioq3/blob/87a99d18206e878a20289f05ac3c16005dcae5c3/code/game/g_cmds.c#L1453)) and OpenJK ([`g_cmds.c:2313`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/game/g_cmds.c#L2313)) test `tolower( msg[0] ) == 'y' || msg[0] == '1'`.

### F6.2 Name lookup for `callvote kick <name>`

[`G_ClientNumberFromName`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_cmds.c#L1871-L1890) and [`G_ClientNumberFromStrippedName`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_cmds.c#L1946-L1965) walk the first `level.numConnectedClients` slots, a count, not the highest slot in use. They do not test `pers.connected`, and `ClientDisconnect` keeps the name of a slot that is left.

Effect:

- a player in a slot after an empty one is not found;
- the old name of a player who left can match, and the vote then targets an empty slot.

No crash: the index stays below `level.maxclients`. OpenJK replaced both with `ClientNumberFromString`.

### Proxy fix

Decision of the author: fix both in the proxy, without changing the game module, with the design below.

The game reads each argument of a command with `trap_Argv`, the syscall `G_ARGV`, and every syscall of the game goes through the proxy ([GameImports.cpp](https://github.com/Yberion/JKA_YBEProxy/blob/ae62510d6857be29351812da153aceafe919ab11/src/JKA_YBEProxy/Module/GameImports.cpp)). So the proxy can give the game other arguments for one command, on the retail engine and on OpenJK alike:

- the proxy's `ClientCommand` sets the replacement arguments;
- a `G_ARGV` case in `Syscall` returns them to the game;
- `ClientCommandDone` clears them.

The proxy's own checks read the arguments from the engine (`proxy.trap->Argv`), so they still see what the client sent.

F6.1, `vote`: argument 1 becomes `yes` when it starts with `y`, `Y` or `1`, and `no` otherwise, also when it is missing. That is the rule of ioq3 and OpenJK. `teamvote` needs nothing: `callteamvote` is disabled, so the game returns before it reads the argument ("no team vote in progress", [`g_cmds.c:2382`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_cmds.c#L2382)).

F6.2, `callvote kick <name>`: the proxy looks the name up itself.

- It checks every slot below `sv_maxclients` whose `CS_PLAYERS` configstring is not empty. The game empties it when a client leaves ([`g_client.c:3929`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_client.c#L3929)).
- Names are compared without colors and without case, as OpenJK.
- Found: the game gets `callvote clientkick <n>`. Its `clientkick` branch builds the same vote string (`clientkick <n>`) and the same text (`kick <name>`) as its `kick` branch.
- Not found: the proxy prints the game's message, `there is no client named '<name>' currently on the server.`, and drops the command.

Limit: when the name is not found, that message comes before the game's own checks (vote in progress, spectator…).

## F7. `modelFullPath` overflow, only with the cheat cvar `d_perPlayerGhoul2` (Low)

Status: **confirmed (binary)** for the branch, on Linux only. The overflow itself is **confirmed (source)**.

### Cause

[`SetupGameGhoul2Model`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_client.c#L1519-L1653) builds `models/players/<name>/model.glm` with `strcpy` into `char modelFullPath[MAX_QPATH]` (64, lines 1557 and 1653). `<name>` is the part of the player's model before the `/`. Beyond 38 characters (64 - 1 - 15 - 10) it writes past the buffer. The proxy replaces a model of 64 characters or more, so the copy can write at most 25 bytes past the end.

### Who can trigger it

Nobody on a normal server. A player's model only reaches this branch (line 1554) when one of these holds:

- `d_perPlayerGhoul2` is set. It is a `CVAR_CHEAT` cvar, 0 by default ([`g_main.c:288`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_main.c#L288)), so only with cheats on.
- `G_PlayerHasCustomSkeleton` returns true. Its body is commented out ([`g_utils.c:162-188`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/game/g_utils.c#L162-L188)). In the binary, `G_PlayerHasCustomSkeleton__FP9gentity_s` at `0x1705c4` is `xor eax, eax ; ret`.

The NPCs take the branch, with model names from the server files.

### Proxy fix

In the model check of `Sanitize` ([Userinfo.cpp:280-311](https://github.com/Yberion/JKA_YBEProxy/blob/ae62510d6857be29351812da153aceafe919ab11/src/JKA_YBEProxy/Features/Userinfo.cpp#L280-L311)), replace with `kyle` a model whose part before the `/` is longer than 38 characters:

```cpp
// Longest model name (the part before the '/') that still fits in "models/players/<name>/model.glm" in 64 bytes
static constexpr std::size_t maxModelBaseLength = 38;

const char* modelSlash = std::strchr(val, '/');
const std::size_t modelBaseLength = modelSlash ? static_cast<std::size_t>(modelSlash - val) : len;

// added to the conditions that set "kyle"
modelBaseLength > maxModelBaseLength ||
```

Decision of the author: not fixed.

## H1. Refuse `"` and control characters in `say`, `say_team` and `tell` (Refuted)

Status: **refuted**. Nothing to do.

### The claim

The game puts the chat text between quotes in the server command it sends to every client (`chat "name: text"`). The first pass said that the server's tokenizer keeps a `"` placed in the middle of a word, so that a player could split that command into more arguments. It proposed to drop chat that contains a `"` or a control character.

### Why it is wrong

The tokenizer never keeps a `"` in an argument:

- a quoted argument ends at the next `"`;
- an unquoted word also ends at a `"`.

That holds for the retail engine ([`qcommon/cmd_common.cpp:445-470`](https://github.com/jedis/jediacademy/blob/4bebb8ec23200ee150a9aa566cea6122c19eba44/codemp/qcommon/cmd_common.cpp#L445-L470)), the proxy's tokenizer ([cmd.cpp:80-103](https://github.com/Yberion/JKA_YBEProxy/blob/ae62510d6857be29351812da153aceafe919ab11/src/JKA_YBEProxy/Engine/Hooks/qcommon/cmd.cpp#L80-L103)), OpenJK ([`qcommon/cmd.cpp:592`](https://github.com/JACoders/OpenJK/blob/260c59c2907187af555a676fe0cc798893bf7757/codemp/qcommon/cmd.cpp#L592)) and ioq3 ([`qcommon/cmd.c:576`](https://github.com/ioquake/ioq3/blob/87a99d18206e878a20289f05ac3c16005dcae5c3/code/qcommon/cmd.c#L576)). `ConcatArgs` joins those arguments, so it never contains a `"`: the check could never trigger.

A control character can only come inside a quoted argument. `\r` and `\n` are already dropped ([ClientCommands.cpp:217-220](https://github.com/Yberion/JKA_YBEProxy/blob/ae62510d6857be29351812da153aceafe919ab11/src/JKA_YBEProxy/Features/ClientCommands.cpp#L217-L220)). No effect was found for the others.

## Already covered by the proxy

Two open points of the first pass, checked in the proxy:

- `SV_UpdateUserinfo_f` copies the userinfo before its flood check (original engine). The proxy's rewrite ([sv_client.cpp:674-704](https://github.com/Yberion/JKA_YBEProxy/blob/ae62510d6857be29351812da153aceafe919ab11/src/JKA_YBEProxy/Engine/Hooks/server/sv_client.cpp#L674-L704)) tests the empty argument and the flood before the copy. `FINAL_BUILD`, which enables the flood check, is defined in every build except Debug ([src/CMakeLists.txt:19](https://github.com/Yberion/JKA_YBEProxy/blob/ae62510d6857be29351812da153aceafe919ab11/src/CMakeLists.txt#L19)).
- `ClientUserinfoChanged` runs on each `userinfo` command even when `sv_floodProtect` ignores the client's text, because `userinfo` is an engine command, not a game command. Each run sends the `CS_PLAYERS` configstring to every client. The same rewrite limits it to 3 changes in 6 seconds.
