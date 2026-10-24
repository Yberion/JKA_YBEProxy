---
tags: JKA_YBEProxy
---

# JKA_YBEProxy Todo

## Short term

- ~~donedl exploit~~
- ~~q3infoboom~~ (+ check fix ip buffer overflow in userinfo `SV_DirectConnect()`)
- ~~rcon disabler (check in `SVC_RemoteCommand()`)~~
    - ~~Add ability to enable back the timer (`proxy_sv_enableRconCmdCooldown`)~~
- ~~model length crash~~
    - ~~`ClientUserinfoChanged()`~~
    - ~~`SV_UserinfoChanged()` (another check inside this function this way even connecting people won't affect other players)~~
    - ~~Add ability to remove that restriction (`proxy_sv_modelPathLength` 64)~~
- ~~Fix high callvote `map_restart` value~~
    - ~~Add `proxy_sv_maxCallVoteMapRestartValue`~~
- ~~Fix negative callvote `map_restart` value~~
- ~~writeconfig wrong files~~
- ~~q3dirtrav~~
- ~~Fix linux startup crash~~
- ~~Fix timenudge calcul with "any" `sv_fps` (range from `20` to `80`, check the `Features` doc)~~
- ~~Add ability to enable/disable `netStatus` with `proxy_sv_enableNetStatus`~~
- ~~Fix `rcon status` truncated with too many players~~
- ~~**Refactor hook/asm utils**~~
- ~~**Refactor the proxy**~~
- Persist bans across server restart (in a file)
- Chek for `jedi_*/red`
  - with `*` being anything
  - <details><summary>Code</summary>

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
  - <details><summary>Code</summary>

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
- Add log in server console when using OpenJK API (same as Original API)
- Fix `/myratio` on OpenJK API
  - `svs.time` problem?
- **Redo the CMake project**
  - https://github.com/cpp-best-practices/cmake_conan_boilerplate_template
  - new: https://github.com/cpp-best-practices/cmake_template
- **Create a plugin system**
- Enhance the readme
- Add a way to store IP's like on OpenJK
- Patch `NET_SendPacket` to display the packet (only if the size is not more than 300? there's some big packets such as info)
- Add mute system (check `SV_SendServerCommand`)
- make `dumpuser` usable with clientNum
- `/rcon forceteam -1 spectator` force everyone to spectate (catch the call with `GAME_CONSOLE_COMMAND`)
- ffa3 crash exploit with events (check in `SV_AddEntitiesVisibleFromPoint`) (**edit:** can't reproduce for now, **edit2**: require high fps)
- rate policy
- snaps policy
- rate limiter (anti ddos)
- ~~/netstatus~~
- ~~/myratio~~
- ~~Add uptime to new rcon status (require to hook `SV_Init()`)~~ **edit:** not possible
- Improve new API support (there's no fallback to the old API for now so make it able to be disabled (json/ini file?))
- ~~Fix model ghost bug~~
- ~~better `rcon status`~~
- Hooking: Should only apply to x86 (check for defines)
- ~~Hooking: Should only apply to original jampDed.exe and linuxjampded (check for `version` cvar)~~

## Other

- ~~Reduce the scope of the force exploit fix~~
- ~~Move order:~~
    - ~~On `GAME_INIT`~~
        - ~~Load the original game module~~
        - ~~Patch the engine (if it's the good version)~~
    - ~~On `GAME_SHUTDOWN`~~
        - ~~Unpatch the engine (if it's the good version)~~
        - ~~Unload the original game module~~
- ~~bug:~~
    - ~~command `netstatus` not working when there's a bot~~
    - with the new `Proxy_SVC_RemoteCommand` it seems there's a problem on `rcon map mp/ffa3` (any map) and `rcon map_restart 0`
        - some due to Discord being open or not on the server machine (not 100% sure why and if it's only cause of Discord)
    - ~~debug build is not working, in `Proxy_SV_UserMove()` parameter `msg` seems not to be valid~~

## Revamp/fix ban system

Stop interpreting `0` as a wildcard.

**Explaination**
Okay I just retried it, it really is like that on Vanilla JKA.
Let us say 2 people join your server, one with `152.0.24.34` and the other guy has `152.99.35.26`
You want to range-ban the `152.0` guy So you ban `152.0`
Now JKA will use the `0` as a wildcard (`*`). And ban `152.X.X.X`
Means you ban the guy with `152.99.35.26` aswell.
In other words, people with `.0` in their IPs cannot be range-banned without banning far more than intended.

**Question**
What you want to do is banning `152.0.*.*` so `152.99.35.26` can still play?

**Answer**
Yep.
Exactly.

## Long term

- From JKA_Anti-cheat: https://hackmd.io/CtIPZON0QHKuqjdxXRw8uw
