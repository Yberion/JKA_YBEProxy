#include "Proxy_EnginePatch.hpp"

/*
====================
SV_RateMsec
Return the number of msec a given size message is supposed
to take to clear, based on the current rate
====================
*/
#define	HEADER_RATE_BYTES	48		// include our header, IP header, and some overhead
static int Proxy_SV_RateMsec(client_t* client, int messageSize) {
	int		rate;
	int		rateMsec;

	// individual messages will never be larger than fragment size
	if (messageSize > 1500) {
		messageSize = 1500;
	}
	rate = client->rate;
	if (server.cvars.sv_maxRate->integer) {
		if (server.cvars.sv_maxRate->integer < 1000) {
			proxy.trap->Cvar_Set("sv_MaxRate", "1000");
		}
		if (server.cvars.sv_maxRate->integer < rate) {
			rate = server.cvars.sv_maxRate->integer;
		}
	}
	rateMsec = (messageSize + HEADER_RATE_BYTES) * 1000 / rate;

	return rateMsec;
}

/*
=======================
SV_SendMessageToClient

Called by SV_SendClientSnapshot and SV_SendClientGameState
=======================
*/

void (*Original_SV_SendMessageToClient)(msg_t*, client_t*);
void Proxy_SV_SendMessageToClient(msg_t* msg, client_t* client)
{
	int			rateMsec;

	// MW - my attempt to fix illegible server message errors caused by 
	// packet fragmentation of initial snapshot.
	while (client->state && client->netchan.unsentFragments)
	{
		// send additional message fragments if the last message
		// was too large to send at once
		Proxy_Common_Com_Printf("[ISM]SV_SendClientGameState() [1] for %s, writing out old fragments\n", client->name);
		server.common.functions.Netchan_TransmitNextFragment(&client->netchan);
	}

	// record information about the message
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSize = msg->cursize;
	// Proxy -------------->
	// client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = svs.time;
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = (proxy.cvarsOldAPI.proxy_sv_pingFix.integer ? proxy.trap->Milliseconds() : server.svs->time);
	// Proxy <--------------
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageAcked = -1;

	// send the datagram
	server.functions.SV_Netchan_Transmit(client, msg);	//msg->cursize, msg->data );

	// set nextSnapshotTime based on rate and requested number of updates

	// local clients get snapshots every frame
	if (client->netchan.remoteAddress.type == NA_LOOPBACK || server.common.functions.Sys_IsLANAddress(client->netchan.remoteAddress))
	{
		client->nextSnapshotTime = server.svs->time - 1;
		return;
	}

	// normal rate / snapshotMsec calculation
	rateMsec = Proxy_SV_RateMsec(client, msg->cursize);

	if (rateMsec < client->snapshotMsec)
	{
		// never send more packets than this, no matter what the rate is at
		rateMsec = client->snapshotMsec;
		client->rateDelayed = qfalse;
	}
	else
	{
		client->rateDelayed = qtrue;
	}

	client->nextSnapshotTime = server.svs->time + rateMsec;

	// don't pile up empty snapshots while connecting
	if (client->state != CS_ACTIVE)
	{
		// a gigantic connection message may have already put the nextSnapshotTime
		// more than a second away, so don't shorten it
		// do shorten if client is downloading
		if (!*client->downloadName && client->nextSnapshotTime < server.svs->time + 1000)
		{
			client->nextSnapshotTime = server.svs->time + 1000;
		}
	}
}
