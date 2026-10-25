#!/usr/bin/env python3
"""
Simulations behind the /netStatus timenudge estimate. See docs/netstatus-estimates.md.

Run: python docs/netstatus-simulation.py
Python 3, standard library only, deterministic (fixed seeds), about a minute.

Two models of the client time, both copies of the retail code (jediacademy codemp/client, the same in OpenJK and
TaystJK), with server time = real time:
- simulate_client: no network, regular frames, integers. The same code as Proxy_Engine_NetStatus_SimulateClient.
- run_client: the same rules with network delays per direction, frames that are sometimes 1 ms longer, and a
  cl_timenudge. It plays the real client, and the proxy estimate is computed from what it sends.
"""
import functools
import random
import statistics


def adjust_time_delta(delta, new_delta, extrapolated):
    """CL_AdjustTimeDelta, at each new snapshot. Returns (serverTimeDelta, extrapolatedSnapshot)."""
    delta_delta = abs(new_delta - delta)
    if delta_delta > 500:  # RESET_TIME
        return new_delta, extrapolated
    if delta_delta > 100:  # fast adjust
        return (delta + new_delta) >> 1, extrapolated
    if extrapolated:
        return delta - 2, False
    return delta + 1, False


def pick(values, statistic):
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
def simulate_client(interval, frame_ms, frames_per_packet, statistic="mean",
                    phases=4, warm=30, snapshots=130, packets=10):
    """Proxy_Engine_NetStatus_SimulateClient. Returns (delay statistic, roundTrip) of a client with no network delay and
    no timenudge, as Proxy_Engine_NetStatus_PacketReceived computes them."""
    delays, rtts = [], []
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


def run_client(interval, frame_ms, frames_per_packet, down, up, seconds, seed, long_frames=0.0, timenudge=0):
    """The client with a network. down and up return the delay of one packet (ms) from a random.Random. long_frames
    is the share of frames 1 ms longer. Returns what the proxy sees: [(arrival, delay)], [(arrival, round trip)]."""
    rng = random.Random(seed)
    first = rng.randrange(interval)
    realtime = rng.randrange(frame_ms)
    in_flight = []  # (arrival at the client, snapshot serverTime)
    sent_snapshots = 0
    delta = None
    extrapolated = False
    cl_server_time = 0
    snap_time = None
    new_snapshot = False
    last_acked = None
    delays, rtts = [], []
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
        if delta is not None and frame % frames_per_packet == 0:
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


def estimate(delays, rtts, interval, frame_ms, frames_per_packet, statistic="mean", start=10000, end=None):
    """The proxy estimate over the samples that arrived in [start, end). Round trips: the last 4 seconds."""
    end = end if end is not None else max(t for t, _ in delays) + 1
    d = [x for t, x in delays if start <= t < end]
    r = [x for t, x in rtts if max(start, end - 4000) <= t < end]
    hold = 500 / (len(d) * 1000 / (end - start))
    round_trip = max(min(r), statistics.mean(r) - hold)
    ref_delay, ref_round_trip = simulate_client(interval, frame_ms, frames_per_packet, statistic)
    return -(pick(d, statistic) + round_trip - (ref_delay + ref_round_trip))


def fixed(ms):
    return lambda rng: ms


def uniform(base, spread):
    return lambda rng: base + rng.uniform(0, spread)


def spikes(base, share, extra):
    return lambda rng: base + (extra if rng.random() < share else 0)


def self_check():
    """The two models agree, and a cl_timenudge comes back with its sign. Regular frames: see table_phase_classes for
    the settings where regular frames lock the client in one phase."""
    for interval, frame_ms, k in [(50, 8, 2), (33, 8, 2), (33, 16, 1)]:
        for timenudge in (0, -15, 15):
            errors = []
            for seed in range(4):
                d, r = run_client(interval, frame_ms, k, fixed(7.5), fixed(7.5), 40, seed, timenudge=timenudge)
                errors.append(estimate(d, r, interval, frame_ms, k) - timenudge)
            assert abs(statistics.mean(errors)) < 0.6, (interval, frame_ms, k, timenudge, errors)
    print("Self-check: the network model with fixed delays gives the set cl_timenudge back within 0.6 ms\n")


def table_fixed_half_frame():
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


def table_third_version():
    print("2. Third version (simulation, mean), error in ms")
    cases = [
        ("sv_fps 20, 124 fps", 50, 8, 2, 0.06, "+0.8 to +1.8"),
        ("sv_fps 30, 124 fps", 33, 8, 2, 0.06, "+1.0"),
        ("sv_fps 30, 62 fps", 33, 16, 1, 0.0, "+0.6"),
        ("sv_fps 30, 249 fps", 33, 4, 3, 0.004, "0"),
    ]
    for label, interval, frame_ms, k, long_frames, measured in cases:
        row = []
        for spread in (0.0, 1.0):
            errors = []
            for seed in range(6):
                d, r = run_client(interval, frame_ms, k, uniform(7.5, spread), uniform(7.5, spread), 40, seed,
                                  long_frames=long_frames)
                errors.append(estimate(d, r, interval, frame_ms, k))
            row.append(statistics.mean(errors))
        print(f"   {label:20} irregular frames {row[0]:+5.1f}   and 0-1 ms jitter {row[1]:+5.1f}   measured {measured}")
    print()


def table_statistics():
    print("3. Statistic of the delay under jitter, 7.5 ms each way plus the jitter, error in ms (sv_fps 20 / sv_fps 30)")
    scenarios = [
        ("none", fixed(7.5), fixed(7.5)),
        ("0-15 ms, server to client", uniform(7.5, 15), fixed(7.5)),
        ("0-15 ms, client to server", fixed(7.5), uniform(7.5, 15)),
        ("0-15 ms, both", uniform(7.5, 15), uniform(7.5, 15)),
        ("10 % of +30 ms spikes, both", spikes(7.5, 0.1, 30), spikes(7.5, 0.1, 30)),
    ]
    names = ("p90", "p75", "median", "mean")
    print(f"   {'jitter':30} " + " ".join(f"{s:>13}" for s in names))
    for label, down, up in scenarios:
        cells = []
        for statistic in names:
            pair = []
            for interval in (50, 33):
                errors = []
                for seed in range(6):
                    d, r = run_client(interval, 8, 2, down, up, 40, seed)
                    errors.append(estimate(d, r, interval, 8, 2, statistic))
                pair.append(statistics.mean(errors))
            cells.append(f"{pair[0]:+5.1f} / {pair[1]:+5.1f}")
        print(f"   {label:30} " + " ".join(f"{c:>13}" for c in cells))
    print()


def table_phase_classes():
    print("5. Phase classes: sv_fps 30, 250 fps, a packet every 3 frames, 7.5 ms each way, 12 runs of 40 s, error in ms")
    for long_frames in (0.0, 0.004, 0.02):
        errors = []
        for seed in range(12):
            d, r = run_client(33, 4, 3, fixed(7.5), fixed(7.5), 40, seed, long_frames=long_frames)
            errors.append(round(estimate(d, r, 33, 4, 3), 1))
        print(f"   {long_frames * 100:4.1f} % of 5 ms frames: mean {statistics.mean(errors):+5.2f}   "
              f"values {sorted(set(errors))}")
    print()


def table_few_snapshots():
    print("4. Shown value (windows of 1 s, median of 3) against the snapshot interval, 125 fps, 7.5 ms each way")
    for interval in (50, 66, 75, 83, 90, 95, 98, 100, 111, 200, 1000):
        d, r = run_client(interval, 8, 2, fixed(7.5), fixed(7.5), 120, 1)
        windows = []
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
