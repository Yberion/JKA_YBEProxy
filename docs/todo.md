# Todo

> [!NOTE]
> Bugs and improvements found during the audit are tracked separately in [audit/](audit/README.md). Items below that the audit covers link to it.

## Open

### Short term

- Persist bans across server restarts, in a file.
- Check the IP buffer overflow fix for the userinfo in `SV_DirectConnect()`.
- Extend the `jedi_/red` model check to every `jedi_*` model. Only the base Jedi models should be allowed; anything else becomes `kyle`.
  - <details><summary>Reference implementation: base Jedi model list</summary>

    ```c
    static const char *baseJediModels[] = {
        // NOTE: tailing slash is required
        "jedi_hf/",
        "jedi_hm/",
        "jedi_kdm/",
        "jedi_rm/",
        "jedi_tf/",
        "jedi_zf/",
    };
    ```

    </details>
  - <details><summary>Reference implementation: check</summary>

    ```c
    if ( g_blockUnknownJediModelsInTeamGametypes.integer && g_gametype.integer >= GT_TEAM && !Q_stricmpn(model, "jedi_", strlen("jedi_")) )
    {
        int i;
        qboolean isBaseJediModel = qfalse;
        for ( i = 0; i < ARRAY_LEN(baseJediModels); i++ )
        { // Check if it's one of the known base jedi models
            if ( !Q_stricmpn(model, baseJediModels[i], strlen(baseJediModels[i])) )
            {
                isBaseJediModel = qtrue;
                break;
            }
        }
        // Set to kyle if it's not a known model
        if ( !isBaseJediModel ) Q_strncpyz( model, "kyle/default", sizeof(model) ); // Gets adjusted to red/blue later
    }
    ```

    </details>
- Log to the server console when the OpenJK API is used, as with the original API. Cause: `InitGame` is not wrapped, see F4 in [audit/fixes.md](audit/fixes.md).
- Fix `/myratio` on the OpenJK API. Cause: the `RunFrame` wrapper never updates `svsTime`, see F4 in [audit/fixes.md](audit/fixes.md).
- **Redo the CMake project.** See also A5 in [audit/improvements.md](audit/improvements.md). Possible templates:
  - https://github.com/cpp-best-practices/cmake_conan_boilerplate_template
  - https://github.com/cpp-best-practices/cmake_template (newer)
- **Create a plugin system.** See A6 in [audit/improvements.md](audit/improvements.md).
- Store player IPs, as OpenJK does.
- Patch `NET_SendPacket` to log packets. Maybe only packets under about 300 bytes, because some packets, such as info responses, are large.
- Add a mute system (look at `SV_SendServerCommand`).
- Make `dumpuser` accept a client number.
- Make `/rcon forceteam -1 spectator` move everyone to spectators (catch the call with `GAME_CONSOLE_COMMAND`).
- ffa3 crash exploit with events (look at `SV_AddEntitiesVisibleFromPoint`). Needs a high FPS. EternalJK fixed it, see O9 in [audit/other-engines.md](audit/other-engines.md).
- Rate policy.
- Snaps policy.
- Rate limiter (anti-DDoS). See F6 in [audit/fixes.md](audit/fixes.md).
- Improve the OpenJK API support. There is no fallback to the original API yet, so make it possible to disable it (JSON or INI file?). See F4 in [audit/fixes.md](audit/fixes.md) and A2 in [audit/improvements.md](audit/improvements.md).
- Only apply hooks on x86 (check the defines).

### Bugs

- `rcon map mp/ffa3` (any map) and `rcon map_restart 0` sometimes crash the server since the hooked `SVC_RemoteCommand` was added. It happened more often with Discord running on the server machine. The likely cause is identified, not reproduced yet, see F1 in [audit/fixes.md](audit/fixes.md).

### Ban system

Stop treating `0` as a wildcard in IP bans.

Vanilla JKA treats a `0` in a ban as a wildcard. Example with two players:

- player A: `152.0.24.34`
- player B: `152.99.35.26`

To range-ban player A, an admin bans `152.0`. JKA reads the `0` as `*` and bans `152.*.*.*`, so player B is banned too. A player whose IP contains a `.0` cannot be range-banned without banning far more people than intended.

Wanted behaviour: banning `152.0` bans `152.0.*.*` only, and player B can still play.

### Long term

- Features from JKA_Anti-cheat: https://hackmd.io/CtIPZON0QHKuqjdxXRw8uw

## Done

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

## Won't do

- Uptime in the new `rcon status`: it would need a hook on `SV_Init()`, which is not possible.
