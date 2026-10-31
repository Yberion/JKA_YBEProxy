#pragma once

#include "sdk/qcommon/q_shared.hpp"
#include "sdk/server/server.hpp"

#include <cstddef>

namespace ybeproxy::engine::netstatus
{

// What /netStatus shows, over the last second
struct NetRates
{
	int packets;		// packets received from the client
	int fps;			// usercmds received from the client, one per client frame
	int snaps;			// snapshots sent to the client
	int rate;			// bytes sent to the client, counted as the engine's rate does
};

// Net ping and timenudge
void FrameClock(void);
void SnapshotSent(client_t* client);
void PacketReceived(client_t* client, const usercmd_t* cmds, int cmdCount);
void GetRates(client_t* client, NetRates* rates);

} // namespace ybeproxy::engine::netstatus
