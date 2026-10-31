#!/usr/bin/env python3
"""
Simulations behind the /netStatus timenudge estimate. See docs/netstatus-estimates.md.

Run: python docs/netstatus-simulation.py
Python 3.10+, standard library only, deterministic (fixed seeds), a few seconds.

Two models of the client time, both copies of the retail code (jediacademy codemp/client, the same in OpenJK and
TaystJK), with server time = real time:
- simulate_client: no network, regular frames, integers. The same code as netstatus::SimulateClient.
- run_client: the same rules with network delays per direction, frames that are sometimes 1 ms longer, and a
  cl_timenudge. It plays the real client, and the proxy estimate is computed from what it sends.
"""
import functools
import random
import statistics
from collections.abc import Callable, Sequence
from typing import Literal

# The statistic taken over the delays of the usercmds
Statistic = Literal["mean", "median", "p75", "p90"]
# The network delay of one packet in ms, drawn from the random generator of the run
Delay = Callable[[random.Random], float]
# (arrival time at the server, value) in ms
Sample = tuple[float, float]


def adjust_time_delta(delta: int, new_delta: int, extrapolated: bool) -> tuple[int, bool]:
    """Apply CL_AdjustTimeDelta (client/cl_cgame.cpp) for one new snapshot.

    Args:
        delta: The current cl.serverTimeDelta, in ms.
        new_delta: The snapshot server time minus the client real time, in ms.
        extrapolated: Whether a frame since the previous snapshot reached the newest snapshot time - 5
            (cl.extrapolatedSnapshot).

    Returns:
        The new serverTimeDelta and the new extrapolated flag. Above 500 ms of difference the delta is reset, above
        100 ms it moves halfway (fast adjust), otherwise it moves by -2 if the flag was set (which clears it) and by +1
        if not.
    """
    delta_delta = abs(new_delta - delta)
    if delta_delta > 500:  # RESET_TIME
        return new_delta, extrapolated
    if delta_delta > 100:  # fast adjust
        return (delta + new_delta) >> 1, extrapolated
    if extrapolated:
        return delta - 2, False
    return delta + 1, False


def pick(values: Sequence[float], statistic: Statistic) -> float:
    """Take a statistic of a list of samples.

    The percentiles are values of the sorted list: p90 is the one at index n - 1 - n // 10, p75 at n - 1 - n // 4.

    Args:
        values: The samples. Must not be empty.
        statistic: "mean", "median", "p75" or "p90".

    Returns:
        The statistic of the samples.

    Raises:
        ValueError: The statistic is unknown.
    """
    s = sorted(values)
    n = len(s)
    if statistic == "mean":
        return statistics.mean(s)
    if statistic == "median":
        return s[n // 2]
    if statistic == "p75":
        return s[n - 1 - n // 4]
    if statistic == "p90":
        return s[n - 1 - n // 10]
    raise ValueError(statistic)


@functools.lru_cache(maxsize=None)
def simulate_client(interval: int, frame_ms: int, frames_per_packet: int, statistic: Statistic = "mean",
                    phases: int = 4, warm: int = 30, snapshots: int = 130, packets: int = 10) -> tuple[float, float]:
    """Run the client time rules with no network delay and no timenudge, as netstatus::SimulateClient.

    Server time and client time are the same clock, in integer ms. Each phase shifts the first client frame by a part
    of a frame against the snapshots, starts near the balance (serverTimeDelta = -interval - 5), lets warm snapshots
    settle, then records every packet until snapshots snapshots and at least packets packets.

    Args:
        interval: The time between two snapshots, in ms.
        frame_ms: The length of a client frame, in ms.
        frames_per_packet: The client sends a packet every frames_per_packet frames.
        statistic: The statistic of the delays, as in pick.
        phases: The number of phases between the client frames and the snapshots.
        warm: The snapshots received before the packets are recorded.
        snapshots: The snapshots received before the run of a phase ends.
        packets: The packets recorded before the run of a phase ends, at least.

    Returns:
        The statistic of the delays (cmd.serverTime minus the time the packet arrives) and the round trip as
        netstatus::PacketReceived computes it: the mean round trip minus half the time between two
        packets, never below the smallest one. Their sum is S in docs/netstatus-estimates.md.
    """
    delays: list[int] = []
    rtts: list[int] = []
    for phase in range(phases):
        realtime = phase * frame_ms // phases
        n = 0
        snap_time = 0
        new_snapshot = False
        delta = -interval - 5  # start near the balance
        extrapolated = False
        cl_server_time = 0
        last_acked = -1
        sent = 0
        frame = 0
        while n < snapshots or sent < packets:
            while n * interval <= realtime:  # the snapshots that arrived before this frame
                snap_time = n * interval
                new_snapshot = True
                n += 1
            if n > warm and frame % frames_per_packet == 0:  # CL_SendCmd: cl.serverTime of the previous frame
                delays.append(cl_server_time - realtime)
                sent += 1
                if snap_time != last_acked:
                    last_acked = snap_time
                    rtts.append(realtime - snap_time)
            if new_snapshot:
                delta, extrapolated = adjust_time_delta(delta, snap_time - realtime, extrapolated)
                new_snapshot = False
            cl_server_time = max(cl_server_time, realtime + delta)  # CL_SetCGameTime, never back
            if realtime + delta >= snap_time - 5:
                extrapolated = True
            frame += 1
            realtime += frame_ms
    hold = frames_per_packet * frame_ms / 2
    return pick(delays, statistic), max(min(rtts), statistics.mean(rtts) - hold)


def run_client(interval: int, frame_ms: int, frames_per_packet: int, down: Delay, up: Delay, seconds: int, seed: int,
               long_frames: float = 0.0, timenudge: int = 0) -> tuple[list[Sample], list[Sample]]:
    """Run the client time rules with a network, and record what the server sees.

    The server sends a snapshot every interval ms from a random start. Each snapshot reaches the client after down(rng)
    ms; a snapshot older than the newest one received is dropped, as the client does. Each packet reaches the server
    after up(rng) ms. A share long_frames of the client frames is 1 ms longer.

    Args:
        interval: The time between two snapshots, in ms.
        frame_ms: The length of a client frame, in ms.
        frames_per_packet: The client sends a packet every frames_per_packet frames.
        down: The delay of each snapshot from the server to the client.
        up: The delay of each packet from the client to the server.
        seconds: The length of the run.
        seed: The seed of the random generator of the run.
        long_frames: The share of the frames that are 1 ms longer, from 0 to 1.
        timenudge: The cl_timenudge of the client, in ms.

    Returns:
        The delay samples (cmd.serverTime minus the arrival time of the packet) and the round trip samples (arrival
        time of the first packet that acknowledges a snapshot minus the time the snapshot was sent), each as
        (arrival time at the server, value) in ms.
    """
    rng = random.Random(seed)
    first = rng.randrange(interval)
    realtime = rng.randrange(frame_ms)
    in_flight: list[tuple[float, int]] = []  # (arrival at the client, snapshot serverTime)
    sent_snapshots = 0
    delta: int | None = None
    extrapolated = False
    cl_server_time = 0
    snap_time: int | None = None
    new_snapshot = False
    last_acked = -1
    delays: list[Sample] = []
    rtts: list[Sample] = []
    frame = 0
    while realtime < seconds * 1000:
        while first + sent_snapshots * interval <= realtime + 500:
            server_time = first + sent_snapshots * interval
            in_flight.append((server_time + down(rng), server_time))
            sent_snapshots += 1
        in_flight.sort()
        while in_flight and in_flight[0][0] <= realtime:
            _, server_time = in_flight.pop(0)
            if snap_time is None or server_time > snap_time:
                snap_time = server_time
                new_snapshot = True
        # delta is only set once a snapshot arrived
        if delta is not None and snap_time is not None and frame % frames_per_packet == 0:
            arrival = realtime + up(rng)
            delays.append((arrival, cl_server_time - arrival))
            if snap_time != last_acked:
                last_acked = snap_time
                rtts.append((arrival, arrival - snap_time))
        if snap_time is not None:
            if delta is None:
                delta = snap_time - realtime
            elif new_snapshot:
                delta, extrapolated = adjust_time_delta(delta, snap_time - realtime, extrapolated)
            new_snapshot = False
            cl_server_time = max(cl_server_time, realtime + delta - timenudge)
            if realtime + delta >= snap_time - 5:
                extrapolated = True
        frame += 1
        realtime += frame_ms + (1 if rng.random() < long_frames else 0)
    return delays, rtts


def estimate(delays: list[Sample], rtts: list[Sample], interval: int, frame_ms: int, frames_per_packet: int,
             statistic: Statistic = "mean", start: float = 10000, end: float | None = None) -> float:
    """Compute the timenudge estimate of the proxy from the samples of run_client.

    timeNudge = -(D + T - S): D the statistic of the delays that arrived in [start, end), T the mean round trip of the
    last 4 seconds before end minus half the time between two packets (never below the smallest), S the same sum from
    simulate_client. The server lateness of the proxy is 0 here: server time is real time.

    Args:
        delays: The delay samples of run_client.
        rtts: The round trip samples of run_client.
        interval: The time between two snapshots, in ms, as given to run_client.
        frame_ms: The length of a client frame, in ms, as given to run_client.
        frames_per_packet: As given to run_client.
        statistic: The statistic of the delays, as in pick.
        start: The start of the window, in ms. The default skips the first 10 s, while the client settles.
        end: The end of the window, in ms. None: after the last sample.

    Returns:
        The estimate of the cl_timenudge of the client, in ms, not rounded.
    """
    end = end if end is not None else max(t for t, _ in delays) + 1
    d = [x for t, x in delays if start <= t < end]
    r = [x for t, x in rtts if max(start, end - 4000) <= t < end]
    hold = 500 / (len(d) * 1000 / (end - start))
    round_trip = max(min(r), statistics.mean(r) - hold)
    ref_delay, ref_round_trip = simulate_client(interval, frame_ms, frames_per_packet, statistic)
    return -(pick(d, statistic) + round_trip - (ref_delay + ref_round_trip))


def fixed(ms: float) -> Delay:
    """Make a network delay that is always the same.

    Args:
        ms: The delay, in ms.

    Returns:
        A delay for run_client.
    """
    return lambda rng: ms


def uniform(base: float, spread: float) -> Delay:
    """Make a network delay with a uniform jitter.

    Args:
        base: The smallest delay, in ms.
        spread: The width of the jitter, in ms: the delay is uniform in [base, base + spread].

    Returns:
        A delay for run_client.
    """
    return lambda rng: base + rng.uniform(0, spread)


def spikes(base: float, share: float, extra: float) -> Delay:
    """Make a network delay with occasional spikes.

    Args:
        base: The delay of most packets, in ms.
        share: The share of the packets that get the spike, from 0 to 1.
        extra: The delay added by a spike, in ms.

    Returns:
        A delay for run_client.
    """
    return lambda rng: base + (extra if rng.random() < share else 0)


def self_check() -> None:
    """Check that the two client models agree.

    With fixed delays and regular frames, the estimate computed from run_client gives back a cl_timenudge of 0, -15
    and +15 within 0.6 ms, at sv_fps 20 (125 fps) and sv_fps 30 (125 and 60 fps). The settings where regular frames
    lock the client in one phase are left out (see table_phase_classes).

    Raises:
        AssertionError: A case is off by 0.6 ms or more.
    """
    for interval, frame_ms, k in [(50, 8, 2), (33, 8, 2), (33, 16, 1)]:
        for timenudge in (0, -15, 15):
            errors: list[float] = []
            for seed in range(4):
                d, r = run_client(interval, frame_ms, k, fixed(7.5), fixed(7.5), 40, seed, timenudge=timenudge)
                errors.append(estimate(d, r, interval, frame_ms, k) - timenudge)
            assert abs(statistics.mean(errors)) < 0.6, (interval, frame_ms, k, timenudge, errors)
    print("Self-check: the network model with fixed delays gives the set cl_timenudge back within 0.6 ms\n")


def table_fixed_half_frame() -> None:
    """Print table 1: the client model against an estimator with a fixed half frame instead of the simulation.

    That estimator, -(p90 + interval + 5 + roundTrip + frame / 2), was measured on a server; the model gives its
    error from simulate_client alone. The measured values are written in the table.
    """
    print("1. Client model against the second version (90th percentile, fixed half frame), error in ms")
    cases = [
        ("VPS sv_fps 20, 125 fps", 50, 8, 2, "-0.3"),
        ("VPS sv_fps 30, 125 fps", 33, 8, 2, "-2.7"),
        ("VPS sv_fps 30, 60 fps", 33, 16, 1, "-5.8"),
        ("VPS sv_fps 30, 250 fps", 33, 4, 3, "-0.5"),
        ("localhost sv_fps 20 and 40, 125 fps", 50, 8, 2, "-0.5 to -1.8"),
        ("localhost sv_fps 30, 125 fps", 33, 8, 2, "-2.7 to -3.3"),
    ]
    for label, interval, frame_ms, k, measured in cases:
        p90, round_trip = simulate_client(interval, frame_ms, k, "p90")
        model = -(p90 + interval + 5 + round_trip + 0.5 * frame_ms)
        print(f"   {label:38} model {model:+5.1f}   measured {measured}")
    print()


def table_third_version() -> None:
    """Print table 2: the error the estimator keeps, from irregular frames and a little jitter.

    Each case uses the share of 1 ms longer frames of the measured frame rate, without jitter and then with 0 to 1 ms of
    jitter each way, 6 runs of 40 s. The measured values are written in the table.
    """
    print("2. Third version (simulation, mean), error in ms")
    cases = [
        ("sv_fps 20, 124 fps", 50, 8, 2, 0.06, "+0.8 to +1.8"),
        ("sv_fps 30, 124 fps", 33, 8, 2, 0.06, "+1.0"),
        ("sv_fps 30, 62 fps", 33, 16, 1, 0.0, "+0.6"),
        ("sv_fps 30, 249 fps", 33, 4, 3, 0.004, "0"),
    ]
    for label, interval, frame_ms, k, long_frames, measured in cases:
        row: list[float] = []
        for spread in (0.0, 1.0):
            errors: list[float] = []
            for seed in range(6):
                d, r = run_client(interval, frame_ms, k, uniform(7.5, spread), uniform(7.5, spread), 40, seed,
                                  long_frames=long_frames)
                errors.append(estimate(d, r, interval, frame_ms, k))
            row.append(statistics.mean(errors))
        print(f"   {label:20} irregular frames {row[0]:+5.1f}   and 0-1 ms jitter {row[1]:+5.1f}   measured {measured}")
    print()


def table_statistics() -> None:
    """Print table 3: the error of the estimator for each statistic of the delays, under several kinds of jitter.

    7.5 ms each way plus the jitter, 125 fps, 2 frames per packet, sv_fps 20 and 30, 6 runs of 40 s for each cell.
    """
    print("3. Statistic of the delay under jitter, 7.5 ms each way plus the jitter, error in ms (sv_fps 20 / sv_fps 30)")
    scenarios: list[tuple[str, Delay, Delay]] = [
        ("none", fixed(7.5), fixed(7.5)),
        ("0-15 ms, server to client", uniform(7.5, 15), fixed(7.5)),
        ("0-15 ms, client to server", fixed(7.5), uniform(7.5, 15)),
        ("0-15 ms, both", uniform(7.5, 15), uniform(7.5, 15)),
        ("10 % of +30 ms spikes, both", spikes(7.5, 0.1, 30), spikes(7.5, 0.1, 30)),
    ]
    names: tuple[Statistic, ...] = ("p90", "p75", "median", "mean")
    print(f"   {'jitter':30} " + " ".join(f"{s:>13}" for s in names))
    for label, down, up in scenarios:
        cells: list[str] = []
        for statistic in names:
            pair: list[float] = []
            for interval in (50, 33):
                errors: list[float] = []
                for seed in range(6):
                    d, r = run_client(interval, 8, 2, down, up, 40, seed)
                    errors.append(estimate(d, r, interval, 8, 2, statistic))
                pair.append(statistics.mean(errors))
            cells.append(f"{pair[0]:+5.1f} / {pair[1]:+5.1f}")
        print(f"   {label:30} " + " ".join(f"{c:>13}" for c in cells))
    print()


def table_phase_classes() -> None:
    """Print table 5: the phase classes of regular frames, and how a few longer frames mix them.

    sv_fps 30, 250 fps, a packet every 3 frames, 7.5 ms each way, 12 runs of 40 s for each share of 5 ms frames.
    """
    print("5. Phase classes: sv_fps 30, 250 fps, a packet every 3 frames, 7.5 ms each way, 12 runs of 40 s, error in ms")
    for long_frames in (0.0, 0.004, 0.02):
        errors: list[float] = []
        for seed in range(12):
            d, r = run_client(33, 4, 3, fixed(7.5), fixed(7.5), 40, seed, long_frames=long_frames)
            errors.append(round(estimate(d, r, 33, 4, 3), 1))
        print(f"   {long_frames * 100:4.1f} % of 5 ms frames: mean {statistics.mean(errors):+5.2f}   "
              f"values {sorted(set(errors))}")
    print()


def table_few_snapshots() -> None:
    """Print table 4: the value /netStatus would show against the snapshot interval.

    As the proxy: one estimate per window of one second, the median of the last three, rounded and limited to +-100.
    One run of 120 s for each interval, 125 fps, 2 frames per packet, 7.5 ms each way.
    """
    print("4. Shown value (windows of 1 s, median of 3) against the snapshot interval, 125 fps, 7.5 ms each way")
    for interval in (50, 66, 75, 83, 90, 95, 98, 100, 111, 200, 1000):
        d, r = run_client(interval, 8, 2, fixed(7.5), fixed(7.5), 120, 1)
        windows: list[float] = []
        for start in range(10000, 119000, 1000):
            if sum(1 for t, _ in d if start <= t < start + 1000) < 10:
                continue
            if not [x for t, x in r if start + 1000 - 4000 <= t < start + 1000]:
                continue
            windows.append(estimate(d, r, interval, 8, 2, start=start, end=start + 1000))
        shown = [max(-100, min(100, round(sorted(windows[i - 2:i + 1])[1]))) for i in range(2, len(windows))]
        print(f"   interval {interval:4} ms: min {min(shown):+4}   max {max(shown):+4}")
    print()


if __name__ == "__main__":
    self_check()
    table_fixed_half_frame()
    table_third_version()
    table_statistics()
    table_few_snapshots()
    table_phase_classes()
