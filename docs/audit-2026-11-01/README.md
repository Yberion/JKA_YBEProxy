# JKA_YBEProxy audit, second round

Review done on 2026-11-01 against commit [`ae62510`](https://github.com/Yberion/JKA_YBEProxy/commit/ae62510d6857be29351812da153aceafe919ab11) (stateless challenges). A first pass, written by another model from a reading of jediacademy `codemp` and of the proxy, listed 7 server bugs (F1 to F7) and one hardening, with proxy fixes. Every claim of that pass was checked again here: on the retail source, on the retail Linux game module (disassembled), in the proxy, and in the other engines. The first pass is not kept. What it got wrong is marked below, so that nobody adds it back.

The IDs (F1 to F7, H1) are those of the first pass and only apply to this folder. They are not the IDs of [audit-2026-10-25](../audit-2026-10-25/README.md).

The links to the proxy sources point to commit `ae62510`.

## Files

- [fixes.md](fixes.md): server bugs that a player can trigger, with the proxy fix for each, ranked by severity (F1 to F7). Also the refuted hardening (H1), and the points of the first pass that the proxy already covers.

## Scope

- Checked: each claim of the first pass, in the retail source, the proxy and the other engines. F1 to F4 were also checked on the retail game modules, Linux and Windows, and F7 on the Linux one.
- Tested by the author: F3 without the fix, on a Windows server (`gc 1 7` sends `Free For All`, no crash) and on a Linux server (crash).
- Not checked:
  - Apart from that test, nothing was run: no other reproduction on a server.
  - The leads listed in [Not checked](#not-checked).

## Sources

Multiplayer code only (`codemp`). Each repository was read at the commit below, and the links in the audit files are pinned to it:

| Repository | Commit read | Notes |
| --- | --- | --- |
| [jedis/jediacademy](https://github.com/jedis/jediacademy) | [`4bebb8e`](https://github.com/jedis/jediacademy/commit/4bebb8ec23200ee150a9aa566cea6122c19eba44) | Raven's JKA source. Close to the retail binaries, but not identical |
| [JACoders/OpenJK](https://github.com/JACoders/OpenJK) | [`260c59c`](https://github.com/JACoders/OpenJK/commit/260c59c2907187af555a676fe0cc798893bf7757) | |
| [taysta/TaystJK](https://github.com/taysta/TaystJK) | [`7d99efb`](https://github.com/taysta/TaystJK/commit/7d99efb4a77a174d1a7853f2a8024daf78fa47b7) | Fork of EternalJK |
| [Yberion/JKA_Anti-cheat](https://github.com/Yberion/JKA_Anti-cheat) | [`4890027`](https://github.com/Yberion/JKA_Anti-cheat/commit/48900270370c2a5275edaf72bbc4b7783c556502) | |
| [mvdevs/jk2mv](https://github.com/mvdevs/jk2mv) | [`7d60145`](https://github.com/mvdevs/jk2mv/commit/7d601454c3db68492289d4d4e3dc30bff39e4246) | JK2, not JKA |
| [mvdevs/mvsdk](https://github.com/mvdevs/mvsdk) | [`4c7c463`](https://github.com/mvdevs/mvsdk/commit/4c7c463638192aead1ff3de8a88cc5de266bf3a2) | JK2MV's game module, a submodule of jk2mv (`src/mvsdk`) at that commit. The JK2MV game code links point to it |
| [ioquake/ioq3](https://github.com/ioquake/ioq3) | [`87a99d1`](https://github.com/ioquake/ioq3/commit/87a99d18206e878a20289f05ac3c16005dcae5c3) | |

A path without a repository root, such as `game/g_cmds.c:1835`, is in jediacademy `codemp`.

## Retail binaries

Both game modules are in `tmp_src` (not in git). Read with capstone, pefile and pyelftools, nothing executed.

- Linux `jampgamei386.so`: 2026696 bytes, dated 2003-11-10 (same date as `linuxjampded` 1.0.1.1), SHA-1 `5adc80bf7309ceded9846d2c6c006451b9326278`. Built with gcc 2.95, non-PIC (calls resolved by `.rel.text` relocations), symbols kept (mangled names). In the test install `E:\Git\jka\server\base` it is renamed `JKA_YBEProxy.so`, and `jampgamei386.so` is the proxy.
- Windows `jampgamex86.dll`: 1200128 bytes, built 2003-11-06, SHA-1 `af88d9528cb2cd214cfe3520acd5ddd3c54f8a1f`. MSVC 7.0 linker, image base `0x20000000`, no symbols: each function was found from a string it uses. No `/GS` stack cookie.

Facts checked on the Linux module:

- `G_LogPrintf` (`0x880c4`): `vsprintf` without a size at `string + 7`, in a 1024-byte stack buffer right below the saved registers and the return address (F1).
- `G_Printf` (`0x85ce4`): same `vsprintf` in 1024 bytes (F1).
- `ClientCommand` (`0x130024`): at intermission, any command outside its list calls `Cmd_Say_f(ent, 0, 1)`, which calls `ConcatArgs(0)` (F1).
- `Cmd_CallVote_f` (`0x12cfc4`): `clientkick` compares the number with 32, then reads `g_entities[n].client->pers.connected` without testing `client` (F2).
- `Cmd_GameCommand_f` (`0x12cc34`): accepts order 7, and `gc_orders[7]` is the value `0x6e697270`, the text `"prin"` that follows the array (F3).
- `BG_LegalizedForcePowers` (`0xea224`): nothing writes `final_Powers` before the read loop (F4).
- `G_PlayerHasCustomSkeleton` (`0x1705c4`): always returns 0 (F7).

Facts checked on the Windows module:

- `G_LogPrintf` (`0x2008fd10`): `vsprintf` without a size at `string + 7`, in a 1024-byte stack buffer right below the return address (F1).
- `ClientCommand`, intermission branch (`0x2007fed5`): `ConcatArgs(0)`, then `G_Say`, which calls `G_LogPrintf` (F1).
- `Cmd_CallVote_f`, `clientkick` (`0x2007dd97`): compares the number with 32, then reads `client->pers.connected` without testing `client` (F2).
- `gc_orders` (`0x2010ae5c`): the entry after the 7 orders points to `Free For All`, the first of `gameNames` (F3).
- `BG_LegalizedForcePowers` (`0x2004a030`): nothing writes `final_Powers` before the read loop (F4).

## Verdict

The first pass found real bugs: F1 to F5 are confirmed, and F1 and F3 are more serious than it said. Its write-up has errors:

- F3 is a crash on the Linux retail server, not a "medium" read past the array. On Windows it only sends `Free For All` (tested by the author, and seen in the DLL).
- F5 disconnects every client, not only the new ones, and its proxy fix removes the hats and capes of TaystJK and EternalJK players.
- F6: the explanation of the vote bug is wrong (the argument is missing, not one character long).
- H1 rests on a false claim about the tokenizer: its check can never trigger.

## Progress

✅ done, ❌ refused (not done, by decision of the author), ⏳ in progress or not done yet.

### [fixes.md](fixes.md)

- ✅ F1: stack overflow in `G_LogPrintf` with a long command at intermission (Critical)
- ✅ F2: crash with `callvote clientkick <n>` (High)
- ✅ F3: crash with `gc <player> 7` (High on Linux, harmless text on Windows)
- ✅ F4: `forcepowers` with 16 or 17 digits of powers (Medium)
- ✅ F5: `color1` and `color2` fill the gamestate of every client (Medium)
- ✅ F6: vote parsing and name lookup of the game (Low, not security)
- ❌ F7: `modelFullPath` overflow, only with the cheat cvar `d_perPlayerGhoul2` (Low)
- H1: refuse `"` and control characters in chat. Refuted, nothing to do.

## Suggested order

1. F1, F3 and F2: three crashes that any player can cause, each a few lines in `Features/ClientCommands.cpp`. F1 first: it runs at every map end.
2. F4: one test added to the `forcepowers` check in `Features/Userinfo.cpp`.
3. F5: cut `color1` and `color2` at 15 characters.
4. F6: argument replacement through `G_ARGV`, for `vote` and `callvote kick <name>`.

## Decisions of the author

- F5: cut `color1` and `color2` at 15 characters, as OpenJK. Not the integer value of the first pass, which removes the TaystJK and EternalJK cosmetics.
- F4: also refuse a power digit outside `0` to `3`, as OpenJK.
- F7: not fixed.
- F6: fixed in the proxy, without changing the game module: the proxy replaces the arguments that the game reads (`G_ARGV`) for `vote` and `callvote kick <name>`. Design in [fixes.md](fixes.md#f6-vote-parsing-and-name-lookup-of-the-game-low-not-security).

## Open questions for the author

None left.

## Not checked

Leads of the first pass that nobody checked yet:

- Entity exhaustion: `G_Spawn` ends with `G_Error("G_Spawn: no free entities")` when the 1024 entities are used. The first pass listed 8 sites that create a `G_TempEntity(..., EV_SABER_BLOCK)` (`game/g_missile.c`, `game/w_saber.c`, `game/g_weapon.c`) and could not show that a player can fill the entities with them. JK2MV fixes a related case for turrets (`mv_fixturretcrash`).
- JK2MV fixes not checked on JKA, also listed in [audit-2026-10-25/other-engines.md](../audit-2026-10-25/other-engines.md): `mv_fixturretcrash`, `mv_fixsaberstealing`, `mv_fixnamecrash`.
- What the clients receive: configstrings (`CS_PLAYERS`, vote texts), server commands (`chat`, `tchat`, `print`, `cp`, `scores`…), snapshots and downloads. `client/`, `cgame/` and `ui/` were not read, except the lines cited in F5. The proxy could filter what another player controls, through `G_SEND_SERVER_COMMAND`, `G_SET_CONFIGSTRING` or engine hooks.
- Points that the first pass read and judged safe, not read again here: `Netchan_Process` (fragments), `SV_VerifyPaks_f`, `va()`, `DeathmatchScoreboardMessage`, `ConcatArgs`, `generic_cmd`, the `weapon` of a usercmd, `ClientCleanName`, `G_ReadSessionData`, `SV_UserMove`, taking over a slot by qport, the cheat commands (`g_cheats`), the sabers (`G_SetSaber` cuts the name at 63 characters), the vehicles, `siegeclass`, the console `say` (`strcpy` without a limit, console and rcon only), `SV_AddServerCommand` (drops a client with 128 unacknowledged commands).
