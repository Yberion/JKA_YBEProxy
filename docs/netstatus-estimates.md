# Estimating a client's cl_timenudge and network ping from the server

State: 2026-10-28. Code: `src/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_NetStatus.cpp`, shown by `/netStatus` (`Proxy_Engine_ClientCommand.cpp`). Simulations: [`netstatus-simulation.py`](netstatus-simulation.py).

## 1. Summary

The proxy estimates two client values that the server does not receive: the `cl_timenudge` setting of the client, and its network round trip.

- **`netPing`** is the smallest round trip of the last 4 seconds. On a Linux server it equals the ping of the operating system (15 to 16 ms against 15 to 16 ms).
- **`timeNudge`** compares the time written in the usercmds of the client with the server clock. It removes the network delay with the mean round trip, and removes the client's own offset with a simulation of the client time rules. On a Linux server, with TaystJK and retail clients, it gave the set `cl_timenudge` back with a mean error of +0.2 to +1.0 ms, +0.6 to +0.8 ms with 26 bots on the server, and no dependency on `sv_fps` (20, 30) or the frame rate (60, 125, 250). With 31 ms of delay added in either direction it stayed within -0.5 to +3.6 ms.

The method has three parts:

1. a model of the client time rules, taken from the client source code;
2. an estimator whose terms cancel the network delay, chosen so that the jitter of the path to the server also cancels;
3. a simulation of the client, run inside the proxy, for the part of the client offset that depends on its frame rate and on the snapshot rate.

The model was checked against measurements before it was used: it predicted the error of a constant correction at 60 and 250 fps before those cases were measured (section 5).

## 2. The client time model

The time rules are read from the retail source (jediacademy `codemp`). They are the same in OpenJK and TaystJK; retail and OpenJK clamp `cl_timeNudge` to ±30, TaystJK does not.

| Rule | Source |
| --- | --- |
| A frame sends its usercmd before it updates the client time: `CL_SendCmd`, then `CL_SetCGameTime` | `client/cl_main.cpp:2348`, `:2354` |
| A usercmd holds `cl.serverTime`, the client time computed at the end of the previous frame | `client/cl_input.cpp:1370` |
| A packet leaves when `1000 / cl_maxpackets` ms (integer division, `cl_maxpackets` 15..100) went by since the last one, so on a frame; a LAN client sends at every frame | `client/cl_input.cpp:1529` |
| `cl.serverTime = realtime + serverTimeDelta - cl_timeNudge`, never decreasing | `client/cl_cgame.cpp:1980` |
| On each frame, if `realtime + serverTimeDelta >= snap.serverTime - 5`, the next snapshot is flagged as extrapolated | `client/cl_cgame.cpp:2063` |
| On each new snapshot, with `deltaDelta = abs(snap.serverTime - realtime - serverTimeDelta)`: above 500 the delta is reset, above 100 the difference is halved (fast adjust), otherwise the delta moves by -2 if flagged and +1 if not | `client/cl_cgame.cpp:1870` (`CL_AdjustTimeDelta`) |
| The snapshot time is the server time `svs.time` | `server/sv_snapshot.cpp:144` |
| The server limits `snaps` to 1..30 and sends snapshots on its own frames | `server/sv_client.cpp:1491`, `server/sv_snapshot.cpp:692` |

Notation: `I` the snapshot interval, `F` the client frame, `k` the frames per packet, `d` and `u` the network delays from and to the server, `tn` the `cl_timenudge`.

**Balance.** The +1 / -2 steps settle where one snapshot in three is flagged. The client time without the nudge then sits about `I + d + 5` ms behind the server time, minus a part of a frame `s`. `s` is not a constant: the arrival of the snapshots moves along the client frames by `I mod F` per snapshot, and that movement couples with the +1 steps (33 = 4 × 8 + 1 and 50 = 6 × 8 + 2 do not settle at the same place).

**Server lateness.** A snapshot holds `svs.time`, the server time of the start of its frame, but leaves after the game frame and after the snapshots of the clients before it in `SV_SendClientMessages`: `L` ms later. For the client, `L` is more downstream delay.

**What the server observes.** For the newest usercmd of a packet, received at server time `R`:

```
delay = cmd.serverTime - R = -(I + d + L + 5 - s) - F - u - tn
```

A round trip (a message sent, then the first packet that acknowledges it), dated from the real send, is `d + u + w`, where `w` is the wait of the client before its next packet: on average half the time between two packets.

## 3. Estimators

### 3.1 Server time base

Inside `GAME_RUN_FRAME`, `svs.time + sv.timeResidual` is the server time as it was when `SV_Frame` started, not rounded to the server frame. Its difference with `Sys_Milliseconds` is the clock offset, smoothed by 1/8 at each game frame and reset when it jumps by more than 50 ms (map change). A delay sample is `(cmd.serverTime - now) - offset`, with the difference of the two integer clocks taken first: a `float` holds them to the millisecond for 2^24 ms (4.6 hours) of uptime only.

### 3.2 netPing

`netPing` is the smallest round trip of the last 4 seconds. A round trip is the real time of the first packet that acknowledges a message, minus the real time the message was sent. The smallest one has no client wait `w` and no jitter, so it compares with the ping of the operating system. At fewer than about 10 snapshots per second there are too few round trips in 4 seconds for the minimum to remove `w` (`snaps 1` showed 24 for an OS ping of 15).

The clock filter of NTP keeps the sample of the smallest round trip for the same reason: its offset error is at most half of the extra delay ([NTPsec clock filter](https://docs.ntpsec.org/latest/filter.html)). That choice is right for the round trip, not for the timenudge (section 3.3): the client settles behind its late snapshots, not its fast ones.

`gamePing` is the ping of the engine (scoreboard, `status`): the mean round trip of the last 32 messages, with `w` and the jitter. It is the time a command of the player takes on average, with the settings of the player.

### 3.3 timeNudge

```
timeNudge = -(D + T + L - S)
```

- `D`: the mean delay of the newest usercmd of each packet over a window of one second (at most 128 packets).
- `T`: the mean round trip of the last 4 seconds minus `500 / packets per second` (the mean of `w`), never below the smallest round trip.
- `L`: the mean server lateness of the snapshots of the window: `(now - svs.time) + offset` when each one is sent.
- `S`: the same `D + T` for the same client with no network delay and no timenudge, from the simulation (section 4). The simulation dates its round trips from the server time of the snapshots, so it has no `L`.

With the model of section 2, `D = D_S - (d + L + u) - tn` and `T = T_S + (d + u)`, so `-(D + T + L - S) = tn`: the network delay and the server lateness cancel. An error `e` on the clock offset (section 3.1) lowers `D` by `e` and raises `L` by `e`, so it cancels too.

The server lateness grows with the load of the server. On the VPS, with 26 bots and two clients at `cl_timenudge 0`, the estimate without `L` was +3.8 (TaystJK) and +4.0 (retail) against +1.2 and +0.9 without bots, and spread over 3 ms instead of 1. With `L`, the same setup gave +0.6 (TaystJK, -1 to +2) and +0.8 (retail, 0 to +1) over 10 captures.

**Statistic.** `D` and `T` are both means. A packet that waits on the way to the server lowers its delay and raises the round trip by the same amount, so the jitter of that path cancels on average. A percentile of the delay keeps only the fastest packets and does not cancel it. Simulated error in ms with 7.5 ms each way plus the jitter (table 3 of the script, `sv_fps 20` / `sv_fps 30`, 125 fps, 2 frames per packet):

| Jitter | 90th percentile | 75th | median | mean |
| --- | --- | --- | --- | --- |
| none | 0.0 / 0.0 | 0.0 / 0.0 | 0.0 / 0.0 | 0.0 / 0.0 |
| 0-15 ms, server to client | +0.6 / +1.9 | +2.3 / +2.2 | +3.0 / +1.9 | +2.6 / +2.3 |
| 0-15 ms, client to server | -4.1 / -3.4 | -1.8 / -1.9 | +0.9 / -0.1 | +0.7 / +0.3 |
| 0-15 ms, both | -2.7 / -1.8 | 0.0 / -0.1 | +2.8 / +1.7 | +2.6 / +2.2 |
| 10 % of +30 ms spikes, both | -4.8 / -2.2 | -3.8 / -1.9 | -3.8 / -2.2 | -0.9 / +1.0 |

The mean has the smallest largest error (2.6 ms) and keeps one sign. What is left comes from the jitter of the path from the server: the client settles behind its late snapshots. The server sees only the sum of both directions, so it cannot remove that part.

**Output.** The median of the last three windows, rounded, limited to ±100.

**Validity.** The client is about `I + 5` ms behind its newest snapshot. From 100 ms `CL_AdjustTimeDelta` halves the difference instead of the +1 / -2 steps, and the client time wanders by tens of milliseconds over tens of seconds. One second of samples cannot follow it (table 4 of the script, shown value over 110 s, 125 fps, 7.5 ms each way):

| Snapshot interval (ms) | 50 | 66 | 75 | 83 | 90 | 95 | 98 | 100 | 111 | 200 | 1000 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Shown value, min to max | 0 | 0 | -1 to 0 | -1 to +1 | 0 | -7 to +7 | 0 | -9 to +6 | -10 to +5 | -15 to +9 | -62 to +16 |

`timeNudge` shows `-` when `I + 5 >= 100`: with `sv_fps 20` that is `snaps` 10 to 19 (one snapshot every two server frames), with `sv_fps 30` `snaps` 10 to 14. It also shows `-` until the first estimate, and for bots.

### 3.4 Other columns

- `fps` and `packets`: usercmds run and packets received during the last second, counted by walking the ring of packets back from the newest.
- `snaps`: what the client asked for, limited by `sv_fps`. `trueSnaps`: snapshots sent during the last second; lower than `snaps` when the request falls between two server frame rates (`sv_fps 40` with `snaps 30` gives 20) or when the client is short of `rate`.

## 4. Simulation of the client

`Proxy_Engine_NetStatus_SimulateClient(I, F, k)` runs the rules of section 2 with no network delay and no timenudge, and returns `D + T` computed as in section 3.3. Its inputs are measured:

- `I`: the snapshot interval of this client, the mean of the real send intervals (smoothed by 1/8, intervals of 1 to 1000 ms), rounded;
- `F`: the median difference between two usercmds in a row over the window;
- `k`: `round(fps / packets)`.

The run is in integer milliseconds, as the client. It covers 4 phases between the client frames and the snapshots. Each phase starts near the balance (`serverTimeDelta = -I - 5`), lets 30 snapshots settle, then measures 100 snapshots and at least 10 packets. It keeps a sum of the delays, not the values.

It runs again only when one of its three inputs changes. A player at 125 fps with `sv_fps 20` costs about 3 000 client frames once. The largest case inside the validity limit is about 49 000 frames (94 ms snapshots at 1000 fps).

The simulation is needed because the part of a frame `s` depends on `I`, `F` and `k`. Table 1 of the script compares the model with measurements of an estimator that used a constant `s` (half a frame) with the 90th percentile of the delay (error in ms):

| Case | model | measured |
| --- | --- | --- |
| VPS, `sv_fps 20`, 125 fps | -1.0 | -0.3 |
| VPS, `sv_fps 30`, 125 fps | -3.0 | -2.7 |
| VPS, `sv_fps 30`, 60 fps | -6.0 | -5.8 |
| VPS, `sv_fps 30`, 250 fps | -1.0 | -0.5 |
| localhost, `sv_fps` 20 and 40, 125 fps | -1.0 | -0.5 to -1.8 |
| localhost, `sv_fps 30`, 125 fps | -3.0 | -2.7 to -3.3 |

The values for 60 and 250 fps were predicted before they were measured. The model matches within 0.8 ms, and a constant correction cannot: the error moves from -1 to -6 ms with `sv_fps` and the frame rate.

## 5. Reference simulations

[`netstatus-simulation.py`](netstatus-simulation.py) (Python 3, standard library, fixed seeds, about 2 seconds):

```
python docs/netstatus-simulation.py
```

It holds two models of the client, both with server time equal to real time:

- `simulate_client`: the copy of `Proxy_Engine_NetStatus_SimulateClient`, line for line.
- `run_client`: the same rules with a network. Each snapshot and each packet gets a delay from a distribution per direction, and a share of the frames can be 1 ms longer. The snapshots the client receives out of order are dropped, as the client does. It returns what the proxy sees, the delay and round trip samples with their arrival times, and `estimate` computes `timeNudge` from them as the proxy does.

Its output:

1. **Self-check.** With fixed delays, `run_client` gives a `cl_timenudge` of 0, -15 and +15 back within 0.6 ms, for `sv_fps 20` at 125 fps and `sv_fps 30` at 125 and 60 fps.
2. **Table 1** (section 4).
3. **Table 2.** The error expected from frames that are not regular, and from a little jitter (section 6.4).
4. **Table 3** (section 3.3).
5. **Table 4** (section 3.3).
6. **Table 5.** Phase classes (section 6.4).

## 6. Experimental validation

### 6.1 Setup

- Server: `linuxjampded` with the proxy, on a Debian VPS 15 to 16 ms from the client (ping of the operating system). Launch options: `+set net_forcenonlocal 1 +set proxy_sv_enableNetStatus 1 +set proxy_sv_pingFix 1 +set sv_fps 20` (`sv_fps` changed with `rcon`). For table 1 also: `jampDed.exe` on the same Windows machine as the client (localhost).
- Clients: TaystJK (`4c1ec5208`) and retail `jamp.exe`, Windows 11. `net_forcenonlocal 1`, `cl_maxpackets 100` (62 packets per second at 125 fps), `snaps` equal to `sv_fps`. Without `net_forcenonlocal 1`, the retail server treats `127.0.0.1` and `192.168.x.x` as LAN and ignores `snaps` and `rate`.
- Each case: wait 10 seconds after a change, run `/netStatus` 5 or 6 times. The error is the mean of the shown `timeNudge` minus the set `cl_timenudge`.
- Added delay: clumsy 0.3 on the client, "Lag" 31 ms. clumsy releases packets on the 15.6 ms Windows timer, so 31 ms is stable and 15 ms is not.

### 6.2 timeNudge without added delay

TaystJK and retail `jamp.exe` at the same time, `sv_fps 20`, `com_maxfps 125` (TaystJK 124 fps / 62 packets, retail 126 / 63):

| Load | Client | cl_timenudge | timeNudge (captures) | error |
| --- | --- | --- | --- | --- |
| none | TaystJK | 0 | 0 to 1 (5) | +0.2 |
| none | TaystJK | -15 | -14 (5) | +1.0 |
| none | retail | 0 | 0 to 1 (10) | +0.2 |
| 26 bots | TaystJK | 0 | -1 to +2 (10) | +0.6 |
| 26 bots | retail | 0 | 0 to +1 (10) | +0.8 |

Without the server lateness `L` in the estimator, the same setup gave +1.2 and +0.9 without load, +3.8 and +4.0 with 26 bots.

Frame rate and `sv_fps`, TaystJK alone, measured before `L` was added (`L` does not depend on the client):

| sv_fps / snaps | com_maxfps (fps / packets) | cl_timenudge | timeNudge | error |
| --- | --- | --- | --- | --- |
| 20 / 20 | 125 (124 / 62) | 0 | 1 to 2 | +1.8 |
| 20 / 20 | 125 | -15 | -15 to -14 | +0.8 |
| 20 / 20 | 125 | -30 | -29 | +1.0 |
| 20 / 20 | 125 | +15 | 16 to 17 | +1.2 |
| 30 / 30 | 125 (124 / 62) | -15 | -14 | +1.0 |
| 30 / 30 | 60 (62 / 62) | 0 | 0 to 1 | +0.6 |
| 30 / 30 | 250 (249 / 83) | 0 | 0 | 0 |

The errors do not depend on `sv_fps` or the frame rate (under the constant correction of section 4 the same cases spanned -0.3 to -5.8 ms). The retail client gives the same results as TaystJK.

### 6.3 timeNudge with added delay

TaystJK, `sv_fps 20`, 125 fps, `cl_timenudge 0`, clumsy 31 ms:

| clumsy filter | netPing | gamePing | timeNudge | error |
| --- | --- | --- | --- | --- |
| `outbound and udp.DstPort == 29070` (client to server) | 47 to 48 | 57 to 58 | 1 to 2 | +1.3 |
| `inbound and udp.SrcPort == 29070` (server to client) | 50 to 56 | 67 to 70 | 3 to 5 | +3.6 |
| both | 80 to 86 | 100 to 103 | -1 to 0 | -0.5 |

Delay to the server adds nothing, as section 3.3 expects. Delay from the server adds about +2.4 ms, the part the server cannot remove (the 15.6 ms steps of clumsy probably act as jitter on that path). With both directions the result is not the sum of the two cases alone; the timing of clumsy is not modelled in detail.

### 6.4 Accuracy

The remaining bias of about +0.5 ms comes from two things that the simulation does not include. Table 2 of the script, error in ms:

| Case | frames 1 ms longer at the measured rate | and 0 to 1 ms of jitter | measured |
| --- | --- | --- | --- |
| `sv_fps 20`, 124 fps | +0.5 | +0.9 | +0.2 to +1.0 |
| `sv_fps 30`, 124 fps | +0.2 | +0.7 | +1.0 (before `L`) |
| `sv_fps 30`, 62 fps | 0.0 | +0.4 | +0.6 (before `L`) |
| `sv_fps 30`, 249 fps | 0.0 | +0.4 | 0 (before `L`) |

At `sv_fps 20` and 124 fps, adding `L` lowered the mean error by about 0.7 ms without load: the server lateness of an idle server.

- **Irregular frames.** A client at `com_maxfps 125` runs 124 frames per second, so about 6 % of its frames are 9 ms; the simulation runs regular frames.
- **Jitter.** A little jitter from the server makes the client settle a little behind.

**Phase classes.** With perfectly regular frames, some settings keep the client in one phase between its packets and the snapshots. Each phase settles at its own place. With `sv_fps 30`, 250 fps and a packet every 3 frames there are three such classes (table 5 of the script, error in ms):

| Frames of 5 ms instead of 4 | Mean of 12 runs | Values |
| --- | --- | --- |
| 0 % | -1.13 | -2.0, -0.9, 0.0 |
| 0.4 % | -0.17 | -1.9 to +0.1 |
| 2 % | +0.06 | 0.0, +0.1 |

A real client's frames are irregular enough to mix the classes: that case measured 0.

**Packet phase lock.** When the time between two packets of the client is close to a simple fraction of the snapshot interval, the wait of the client between a snapshot and the packet that acknowledges it takes only a few values, set by a phase that the server does not see. `T` then holds that phase instead of the mean wait, and `netPing` too. Measured with TaystJK at 120 fps and `cl_maxpackets 30` (24 packets per second, 5 frames per packet, `sv_fps 20`), the same settings at two times:

| Time | cl_timenudge | timeNudge | error | netPing (OS 15) |
| --- | --- | --- | --- | --- |
| 23:39 | 0 | -5 to -4 | -4.8 | 21 |
| 23:41 | -15 | -22 | -7.0 | 24 |
| 23:45 | 0 | 1 to 2 | +1.4 | 15 |

The retail client at 125 fps (25 packets) gave +1.0 and +1.7 in the same minutes, and both clients gave +0.6 to +1.0 with `cl_maxpackets 50` (41 packets, 3 frames per packet). The client model reproduces it with frames that repeat a fixed pattern, not with random irregular frames:

| Frames (`sv_fps 20`, 7.5 ms each way) | Frames per packet | error over 10 phases | netPing (network 15) |
| --- | --- | --- | --- |
| 8 ms | 5 | -8.3 to -0.1 | 16 to 24 |
| 8, 8, 9 ms (120 fps) | 5 | -3.9 to -2.9 | 16 to 22 |
| 10 ms (100 fps, `cl_maxpackets 20`: a packet every 50 ms, as the snapshots) | 5 | -43.5 to -3.5 | 22 to 58 |

The phase moves slowly with the clock drift between client and server, so the error comes and goes. `cl_maxpackets 30` is the default of the retail client (OpenJK 63, TaystJK 100).

### 6.5 Few snapshots

- `snaps 10` with `sv_fps 20` (100 ms), TaystJK: without the validity limit the value moved from -8 to +2 within 10 seconds (table 4: -9 to +6). With it, `timeNudge` shows `-`, and the value comes back after `snaps 20`.
- `snaps 1` at 1000 fps: without the limit `timeNudge` stayed at -100 and `netPing` was 24 (4 round trips in 4 seconds). With it, `timeNudge` shows `-`.

### 6.6 netPing

`netPing` was 15 to 16 ms in every case without added delay, the same as the ping of the operating system (minimum 15, maximum 16). `gamePing` was 20 to 24 ms: `netPing` plus half the 16 ms between two packets, plus the jitter. With 31 ms of delay one way `netPing` rose by 31 to 41 ms, both ways by 64 to 71 ms.

## 7. Limits

- The part of the delay from the server that varies (jitter) adds a positive error: about +2.5 ms in the model with 0 to 15 ms of jitter, +3.6 ms measured with clumsy 31 ms.
- `timeNudge` is not given below about 10 snapshots per second (`I + 5 >= 100`).
- Packet phase lock (section 6.4): with stable frames and packets close to a simple fraction of the snapshot interval, `timeNudge` and `netPing` can be off by several ms for a while (-7 and +9 measured with `cl_maxpackets 30`).
- The part of the server lateness that varies from frame to frame acts as jitter from the server: only its mean is removed.
- Not modelled: a client limited by its `rate` (irregular snapshot intervals), packet loss, and clients that change the time code. Measured clients: TaystJK and retail.
- The output is limited to ±100 ms. The retail client limits `cl_timeNudge` to ±30, TaystJK to ±2000.

## 8. Implementation

| Function | Hook | Work |
| --- | --- | --- |
| `Proxy_Engine_NetStatus_FrameClock` | `GAME_RUN_FRAME` (`Proxy_Main.cpp`) | clock offset, each game frame |
| `Proxy_Engine_NetStatus_SnapshotSent` | `SV_SendMessageToClient` (`Patches/server/Proxy_sv_snapshot.cpp`) | send time and sequence of each message; snapshot interval and server lateness |
| `Proxy_Engine_NetStatus_PacketReceived` | `SV_UserMove` (`Patches/server/Proxy_sv_client.cpp`) | round trip, packet count, delay and frame samples; once per second: `netPing`, `timeNudge` |
| `Proxy_Engine_NetStatus_SimulateClient` | | section 4, when its inputs change |
| `Proxy_Engine_NetStatus_GetRates` | `/netStatus` | `fps`, `packets`, `trueSnaps` |

All of it runs only with `proxy_sv_enableNetStatus 1` and the original engine. Per client: about 10 KB (`netEstimate_t`: 64 round trips, 128 window samples, 1 024 packets). Per packet: a few operations. Per second and client: a walk over the round trips (64) and the packets of the last second, and a partial sort of at most 128 frame lengths (`std::nth_element`).

## 9. References

- Client: jediacademy `codemp/client/cl_main.cpp` (`CL_Frame`), `cl_input.cpp` (`CL_CreateCmd`, `CL_ReadyToSendPacket`), `cl_cgame.cpp` (`CL_AdjustTimeDelta`, `CL_SetCGameTime`); the same functions in OpenJK and TaystJK `codemp/client`.
- Server: jediacademy `codemp/server/sv_snapshot.cpp` (`SV_SendMessageToClient`), `sv_client.cpp` (`SV_UserinfoChanged`), `sv_main.cpp` (`SV_CalcPings`).
- [NTPsec, Clock Filter Algorithm](https://docs.ntpsec.org/latest/filter.html): minimum-delay sample selection.
