#pragma once

#include "sdk/qcommon/q_shared.hpp"
#include "sdk/server/server.hpp"

#include <cstddef>

// What /netStatus shows, over the last second
struct NetRates_t
{
	int packets;		// packets received from the client
	int fps;			// usercmds received from the client, one per client frame
	int snaps;			// snapshots sent to the client
	int rate;			// bytes sent to the client, counted as the engine's rate does
};

// Net ping and timenudge
void Proxy_Engine_NetStatus_FrameClock(void);
void Proxy_Engine_NetStatus_SnapshotSent(client_t* client);
void Proxy_Engine_NetStatus_PacketReceived(client_t* client, const usercmd_t* cmds, int cmdCount);
void Proxy_Engine_NetStatus_GetRates(client_t* client, NetRates_t* rates);