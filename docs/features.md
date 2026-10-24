---
tags: JKA_YBEProxy
---

# JKA_YBEProxy Features

## 1) Cvars

### Cvars affecting original engine modifications

> [!WARNING]
> Those CVars require original engine.

#### `proxy_sv_pingFix`

> [!NOTE]
> **Description:** Allow to have a more accurate ping calculation.
>
> **Values**
> - `0` (*default*) disable the ping fix
> - `1` enable the ping fix
>
> No restart needed.

> [!WARNING]
> If you disable it, the `cl_timenudge` calculation in `/netstatus` won't work anymore.

> [!WARNING]
> Only available with original engine.

#### `proxy_sv_enableRconCmdCooldown`

> [!NOTE]
> **Description:** Allow only `1` rcon command per `500ms`.
>
> **Values**
> - `0` (*default*) disable the cooldown (fix rcon disabler)
> - `1` enable the cooldown
>
> No restart needed.

> [!WARNING]
> If this is disabled (`0`) then an attacker can bruteforce your `rconpassword` but will prevent `rcon disabler`, set a strong `rconpassword`.

> [!WARNING]
> Only available with original engine.

#### `proxy_sv_enableNetStatus`

> [!NOTE]
> **Description:** enable the `netStatus` / `showNet` command.
>
> **Values**
> - `0` (*default*) disable the commands
> - `1` enable the commands
>
> No restart needed.

> [!WARNING]
> Only available with original engine.

---

### Cvars related to the proxy not affecting original engine modifications

#### `proxy_sv_maxCallVoteMapRestartValue`

> [!NOTE]
> **Description:** Max value of the `map_restart` callvote timer.
>
> **Values**
> - numeric value
> - `60` (*default*)
>
> No restart needed.

---

#### `proxy_sv_disableKillCmd`

> [!NOTE]
> **Description:** Disable the `kill` command.
>
> **Values**
> - `0` (*default*) enable the command
> - `1` disable the command
>
> No restart needed.

---

### Cvars related to the proxy and original engine modifications (it means that it will partially work with a custom engine)

#### `proxy_sv_modelPathLength`

> [!NOTE]
> **Description:** Max model length.
>
> **Values**
> - numeric value
> - `64` (*default*)
>
> No restart needed.

---

## 2) Commands

#### `/netStatus` `/showNet`

> [!NOTE]
> **Description:** Display net settings of every clients on the server.

> [!WARNING]
> Only available with original engine.

> [!WARNING]
> For `cl_timenudge` calculation:
>
> - Enable `proxy_sv_pingFix`
> - ~~Supported range of `sv_fps` from `20` to `80`~~
> - Support `sv_fps` `20` and `30`
>     - ~~(Windows) **Not** supported `sv_fps`: `60`, `61`, `62`~~
>     - ~~(Linux) **Not** supported `sv_fps`: `31`, `32`, `33`, `34`, `41`, `42`, `43`, `44`, `59`, `60`, `61`, `62`~~

#### `/myratio`

> [!NOTE]
> **Description:** Display your personal stats (kills, deaths with suicides and your ratio).
