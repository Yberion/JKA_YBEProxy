#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Engine/NetStatus.hpp"
#include "JKA_YBEProxy/Engine/EngineState.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

// ==================================================
// Network estimates: net ping and timenudge
// --------------------------------------------------
// What the client does (retail cl_main.cpp, cl_input.cpp, cl_cgame.cpp):
// - CL_Frame calls CL_SendCmd before CL_SetCGameTime. A usercmd made in a frame holds the cl.serverTime of the end of
//   the previous frame, so it is one client frame (1000 / fps) older than the packet that carries it.
// - cl.serverTime = realtime + serverTimeDelta - cl_timeNudge (cl_timeNudge is limited to +-30).
// - serverTimeDelta goes up by 1 on each snapshot, and down by 2 when, before it, realtime + serverTimeDelta reached
//   the newest snapshot time - 5 (CL_AdjustTimeDelta). So the client time (without the nudge) stays just short of the
//   newest snapshot time - 5 when the next snapshot arrives: about one snapshot interval and the downstream delay behind
//   the server time.
// - A snapshot holds svs.time, the server time of the start of its frame, and leaves after the game frame and the
//   snapshots of the clients before it: late ms later. For the client that is downstream delay.
// With cmd the newest usercmd of a packet, received at server time R:
//   cmd.serverTime - R = simulated - (roundTrip + late) - timenudge
// where simulated is cmd.serverTime - R + roundTrip for a client with no network delay and no timenudge (the round trip
// is measured from the real send, so late is added back: 26 bots raised the estimate by 2.7 ms without it). The client tests
// the extrapolation once per frame, so where it settles depends on how the snapshot interval falls on its frames: a fixed
// half frame instead of simulated was off by -1 ms with 50 ms snapshots at 125 fps, -3 ms with 33 ms ones, -6 ms with
// 33 ms ones at 60 fps. Proxy_Engine_NetStatus_SimulateClient runs the client rules.
// The round trip to subtract is the mean of the round trips, not the smallest: the client settles behind its late
// packets, so a path with jitter, or a delay in both directions, made the smallest one too short (measured on a
// localhost with clumsy and on a Linux VM with netem, the error was 10 to 26 ms; with the mean it is about 4 ms). The
// mean holds the wait of the client before it sends an acknowledgement, half the time between two of its packets
// The delay of the usercmds is the mean too: a packet that waits on the way to the server lowers the delay and raises the
// round trip by the same amount, so the jitter of that path cancels out. The 90th percentile of the delay kept only the
// fastest packets: off by -4 ms with 0 to 15 ms of jitter on that path, in a simulation
// Measured with a TaystJK client only
// ==================================================

// Server time, continuous (not rounded to the server frame), minus the real clock. Updated at each game frame
static struct
{
	float	offset;
	bool	valid;
} continuousClock;

// Inside GAME_RUN_FRAME, SV_Frame has just moved one frame from sv.timeResidual to svs.time. The sum of the two does
// not change, and is the server time as it was when SV_Frame started: svs.time rounded down to the frame is not
void Proxy_Engine_NetStatus_FrameClock(void)
{
	const float offset = (float)(server.svs->time + server.sv->timeResidual - proxy.trap->Milliseconds());

	// First call, or the server time jumped (map change)
	if (!continuousClock.valid || std::fabs(offset - continuousClock.offset) > 50.0f)
	{
		continuousClock.offset = offset;
		continuousClock.valid = true;

		return;
	}

	continuousClock.offset += (offset - continuousClock.offset) / 8.0f;
}

void Proxy_Engine_NetStatus_SnapshotSent(client_t* client)
{
	netEstimate_t& e = proxy.clientData[getClientNumFromAddr(client)].netStatus.estimate;
	const int slot = client->netchan.outgoingSequence & PACKET_MASK;
	const int now = proxy.trap->Milliseconds();

	e.sentTime[slot] = now;
	e.sentSequence[slot] = client->netchan.outgoingSequence;

	// Only the snapshots: the game state and the messages before the game starts come at other intervals
	if (client->state == CS_ACTIVE)
	{
		const int interval = now - e.lastSnapshotTime;

		if (e.lastSnapshotTime && interval > 0 && interval <= 1000)
		{
			e.snapshotInterval += ((float)interval - e.snapshotInterval) / 8.0f;
		}

		e.lastSnapshotTime = now;

		// Real send time minus the real time of svs.time. The difference of the two integer clocks first, as for the delay
		if (continuousClock.valid)
		{
			e.lateSum += (float)(now - server.svs->time) + continuousClock.offset;
			e.lateCount++;
		}
	}
}

static int Proxy_Median3(int a, int b, int c)
{
	return std::max(std::min(a, b), std::min(std::max(a, b), c));
}

// Runs the client rules (CL_SendCmd, CL_AdjustTimeDelta, CL_SetCGameTime) for a client with no network delay and no
// timenudge, a snapshot every interval ms, a frame every frameMs ms and a packet every framesPerPacket frames. Returns
// delay + roundTrip as Proxy_Engine_NetStatus_PacketReceived computes them for that client
static float Proxy_Engine_NetStatus_SimulateClient(int interval, int frameMs, int framesPerPacket)
{
	// A few phases between the client frames and the snapshots. The client starts near its balance, and its packets are
	// measured after WarmSnapshots, until Snapshots and at least 10 packets
	constexpr int Phases = 4;
	constexpr int WarmSnapshots = 30;
	constexpr int Snapshots = 130;
	constexpr int Packets = 10;

	double delaySum = 0.0;
	int delayCount = 0;
	int rttMin = -1;
	int rttSum = 0;
	int rttCount = 0;

	for (int phase = 0; phase < Phases; ++phase)
	{
		int realtime = phase * frameMs / Phases;
		int snapshots = 0;
		int snapTime = 0;
		bool newSnapshot = false;
		int serverTimeDelta = -interval - 5;
		bool extrapolated = false;
		int clServerTime = 0;
		int lastAcked = -1;
		int packets = 0;

		for (int frame = 0; snapshots < Snapshots || packets < Packets; ++frame, realtime += frameMs)
		{
			// The snapshots that arrived before this frame
			while (snapshots * interval <= realtime)
			{
				snapTime = snapshots * interval;
				newSnapshot = true;
				++snapshots;
			}

			// CL_SendCmd: the usercmd holds cl.serverTime of the previous frame
			if (snapshots > WarmSnapshots && frame % framesPerPacket == 0)
			{
				delaySum += clServerTime - realtime;
				++delayCount;
				++packets;

				if (snapTime != lastAcked)
				{
					const int rtt = realtime - snapTime;

					lastAcked = snapTime;
					rttSum += rtt;
					++rttCount;

					if (rttMin < 0 || rtt < rttMin)
					{
						rttMin = rtt;
					}
				}
			}

			// CL_AdjustTimeDelta
			if (newSnapshot)
			{
				const int newDelta = snapTime - realtime;
				const int deltaDelta = std::abs(newDelta - serverTimeDelta);

				newSnapshot = false;

				if (deltaDelta > 500)
				{
					serverTimeDelta = newDelta;
				}
				else if (deltaDelta > 100)
				{
					serverTimeDelta = (serverTimeDelta + newDelta) >> 1;
				}
				else if (extrapolated)
				{
					extrapolated = false;
					serverTimeDelta -= 2;
				}
				else
				{
					serverTimeDelta++;
				}
			}

			// CL_SetCGameTime: the time never goes back
			clServerTime = std::max(clServerTime, realtime + serverTimeDelta);

			if (realtime + serverTimeDelta >= snapTime - 5)
			{
				extrapolated = true;
			}
		}
	}

	const float hold = (float)(framesPerPacket * frameMs) / 2.0f;

	return (float)(delaySum / delayCount) + std::max((float)rttMin, (float)rttSum / (float)rttCount - hold);
}

// Packets and usercmds received in the last second. The ring is in time order: walk back from the newest
static void Proxy_Engine_NetStatus_CountPackets(const netEstimate_t& e, int now, int* packets, int* cmds)
{
	*packets = 0;
	*cmds = 0;

	for (int n = 1; n <= netEstimate_t::PacketSamples; ++n)
	{
		const int i = (e.packetIndex - n + netEstimate_t::PacketSamples) % netEstimate_t::PacketSamples;

		if (!e.packetTime[i] || now - e.packetTime[i] >= 1000)
		{
			break;
		}

		++*packets;
		*cmds += e.packetCmds[i];
	}
}

void Proxy_Engine_NetStatus_PacketReceived(client_t* client, const usercmd_t* cmds, int cmdCount)
{
	ClientData_t* clientData = &proxy.clientData[getClientNumFromAddr(client)];
	netEstimate_t& e = clientData->netStatus.estimate;
	const int now = proxy.trap->Milliseconds();

	// Round trip: the first packet that acknowledges a snapshot, minus the real time that snapshot was sent
	// It includes the wait of the client before it sends the next packet, which is kept small by taking the minimum
	const int ack = client->messageAcknowledge;
	const int slot = ack & PACKET_MASK;

	if (e.sentSequence[slot] == ack && e.lastAckedSequence != ack)
	{
		const int rtt = now - e.sentTime[slot];

		e.lastAckedSequence = ack;

		if (rtt >= 0 && rtt <= 2000)
		{
			e.rttTime[e.rttIndex] = now;
			e.rttValue[e.rttIndex] = rtt;
			e.rttIndex = (e.rttIndex + 1) % netEstimate_t::RttSamples;
		}
	}

	// Packets and usercmds per second. The usercmds that SV_UserMove runs: not the copies of older ones that the client
	// repeats (cl_packetdup), and not the ones from before a map_restart
	int newCmds = 0;

	for (int i = 0; i < cmdCount; ++i)
	{
		if (cmds[i].serverTime <= cmds[cmdCount - 1].serverTime && cmds[i].serverTime > client->lastUsercmd.serverTime)
		{
			++newCmds;
		}
	}

	e.packetTime[e.packetIndex] = now;
	e.packetCmds[e.packetIndex] = newCmds;
	e.packetIndex = (e.packetIndex + 1) % netEstimate_t::PacketSamples;

	// Delay and frame lag, from the newest usercmd only: the older ones of the packet were made earlier
	if (continuousClock.valid && cmdCount > 0)
	{
		const usercmd_t& newest = cmds[cmdCount - 1];

		if (newest.serverTime > client->lastUsercmd.serverTime && e.delayCount < netEstimate_t::WindowSamples)
		{
			// The difference of the two integer clocks first: a float holds them to the millisecond for 4.6 hours only
			e.delaySum += (float)(newest.serverTime - now) - continuousClock.offset;
			e.delayCount++;

			// The usercmd before the newest: two usercmds in a row are one client frame apart
			const int previous = cmdCount >= 2 ? cmds[cmdCount - 2].serverTime : e.lastCmdServerTime;
			const int frame = newest.serverTime - previous;

			if (previous && frame >= 1 && frame <= 100)
			{
				e.frameLag[e.lagCount++] = frame;
			}
		}

		if (newest.serverTime > e.lastCmdServerTime)
		{
			e.lastCmdServerTime = newest.serverTime;
		}
	}

	if (now - e.windowStart < 1000)
	{
		return;
	}

	e.windowStart = now;

	// Net ping: the smallest round trip of the last 4 seconds. The mean is for the timenudge
	int rttMin = -1;
	int rttCount = 0;
	int rttSum = 0;

	for (int i = 0; i < netEstimate_t::RttSamples; ++i)
	{
		if (e.rttTime[i] && now - e.rttTime[i] <= 4000)
		{
			++rttCount;
			rttSum += e.rttValue[i];

			if (rttMin < 0 || e.rttValue[i] < rttMin)
			{
				rttMin = e.rttValue[i];
			}
		}
	}

	if (rttMin >= 0)
	{
		clientData->netStatus.netPing = rttMin;
	}

	// The samples of the window, which starts again
	const float delaySum = e.delaySum;
	const int delayCount = e.delayCount;
	const float late = e.lateCount ? e.lateSum / (float)e.lateCount : 0.0f;
	const int lagCount = e.lagCount;

	e.delaySum = 0.0f;
	e.delayCount = 0;
	e.lateSum = 0.0f;
	e.lateCount = 0;
	e.lagCount = 0;

	// Not enough packets (the client sends at least 15 per second), no round trip yet, or no frame length (the client
	// runs below 10 frames per second)
	if (delayCount < 10 || rttMin < 0 || !lagCount)
	{
		return;
	}

	// The length of a client frame: the median of the frame lags
	std::nth_element(e.frameLag, e.frameLag + lagCount / 2, e.frameLag + lagCount);

	const int frameMs = e.frameLag[lagCount / 2];

	// The client sends its acknowledgement with its next packet: on average half the time between two packets
	int packetsPerSecond;
	int cmdsPerSecond;

	Proxy_Engine_NetStatus_CountPackets(e, now, &packetsPerSecond, &cmdsPerSecond);

	const float hold = packetsPerSecond > 0 ? 500.0f / (float)packetsPerSecond : 0.0f;
	const float roundTrip = std::max((float)rttMin, (float)rttSum / (float)rttCount - hold);
	const int interval = std::max(1, e.snapshotInterval > 0.0f ? (int)std::lround(e.snapshotInterval) : 1000 / std::max(server.cvars.sv_fps->integer, 1));

	// The client is about interval + 5 ms behind its newest snapshot. From 100 ms it moves its time by halves (fast adjust,
	// CL_AdjustTimeDelta), and its time wanders by tens of ms: one second of samples cannot tell its timenudge (in a
	// simulation, the shown value swung by ±8 ms at 100 ms, by -58 to +20 at 1000 ms, and stayed within 1 ms up to 90 ms)
	if (interval + 5 >= 100)
	{
		clientData->netStatus.timenudgeKnown = false;
		e.historyIndex = 0;
		e.historyFilled = false;

		return;
	}

	const int framesPerPacket = std::max(1, (int)std::lround((float)cmdsPerSecond / (float)std::max(packetsPerSecond, 1)));

	// Up to about 50 000 client frames (94 ms snapshots at 1000 fps), and the inputs rarely change
	if (interval != e.simInterval || frameMs != e.simFrameMs || framesPerPacket != e.simFramesPerPacket)
	{
		e.simInterval = interval;
		e.simFrameMs = frameMs;
		e.simFramesPerPacket = framesPerPacket;
		e.simulated = Proxy_Engine_NetStatus_SimulateClient(interval, frameMs, framesPerPacket);
	}

	// late: the simulation dates its round trips from the server time of the snapshots, the proxy from their real send
	const float estimate = -(delaySum / (float)delayCount + roundTrip + late - e.simulated);

	// The retail client limits cl_timeNudge to +-30, other clients allow more (TaystJK: +-2000). A value this far is noise
	const int nudge = std::max(-100, std::min(100, (int)std::lround(estimate)));

	// The median of the last three windows hides a single bad one
	e.history[e.historyIndex] = nudge;
	e.historyIndex = (e.historyIndex + 1) % 3;

	if (!e.historyFilled && e.historyIndex == 0)
	{
		e.historyFilled = true;
	}

	clientData->netStatus.timenudge = e.historyFilled ? Proxy_Median3(e.history[0], e.history[1], e.history[2]) : nudge;
	clientData->netStatus.timenudgeKnown = true;
}


void Proxy_Engine_NetStatus_GetRates(client_t* client, NetRates_t* rates)
{
	const netEstimate_t& e = proxy.clientData[getClientNumFromAddr(client)].netStatus.estimate;
	const int now = proxy.trap->Milliseconds();

	*rates = NetRates_t();

	Proxy_Engine_NetStatus_CountPackets(e, now, &rates->packets, &rates->fps);

	// The snapshots of the last second. The ring keeps the last 32: when the client gets more than 32 per second the
	// rate is the one of the span they cover
	int count = 0;
	int bytes = 0;
	int oldest = now;
	int newest = 0;

	for (int i = 0; i < PACKET_BACKUP; ++i)
	{
		if (e.sentSequence[i] && now - e.sentTime[i] < 1000)
		{
			++count;
			// The slot of the message, as SV_SendMessageToClient wrote it. 48 bytes of header, as SV_RateMsec counts, so
			// a client that is short of rate shows about its rate
			bytes += client->frames[i].messageSize + 48;
			oldest = std::min(oldest, e.sentTime[i]);
			newest = std::max(newest, e.sentTime[i]);
		}
	}

	const float perSecond = (count >= 2 && newest > oldest) ? (float)(count - 1) * 1000.0f / (float)(newest - oldest) : (float)count;

	rates->snaps = (int)std::lround(perSecond);
	// The mean size of these messages times their rate
	rates->rate = count ? (int)std::lround((float)bytes / (float)count * perSecond) : 0;
}
