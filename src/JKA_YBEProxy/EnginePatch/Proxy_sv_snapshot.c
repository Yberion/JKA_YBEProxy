#include "Proxy_EnginePatch.h"

/*
=======================
SV_SendMessageToClient

Called by SV_SendClientSnapshot and SV_SendClientGameState
=======================
*/
void Proxy_SV_SendMessageToClient(msg_t* msg, client_t* client)
{
	//WIP
	int			rateMsec;

	// MW - my attempt to fix illegible server message errors caused by 
	// packet fragmentation of initial snapshot.
	while (client->state && client->netchan.unsentFragments)
	{
		// send additional message fragments if the last message
		// was too large to send at once
		Com_Printf("[ISM]SV_SendClientGameState() [1] for %s, writing out old fragments\n", client->name);
		proxy.server.functions.Netchan_TransmitNextFragment(&client->netchan);
	}

	// record information about the message
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSize = msg->cursize;
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = proxy.trap->Milliseconds();
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageAcked = -1;

	// send the datagram
	proxy.server.functions.SV_Netchan_Transmit(client, msg);	//msg->cursize, msg->data );

	// set nextSnapshotTime based on rate and requested number of updates

	// local clients get snapshots every frame
	if (client->netchan.remoteAddress.type == NA_LOOPBACK || proxy.server.functions.Sys_IsLANAddress(client->netchan.remoteAddress))
	{
		client->nextSnapshotTime = proxy.server.svs->time - 1;
		return;
	}

	// normal rate / snapshotMsec calculation
	rateMsec = proxy.server.functions.SV_RateMsec(client, msg->cursize);

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

	client->nextSnapshotTime = proxy.server.svs->time + rateMsec;

	// don't pile up empty snapshots while connecting
	if (client->state != CS_ACTIVE)
	{
		// a gigantic connection message may have already put the nextSnapshotTime
		// more than a second away, so don't shorten it
		// do shorten if client is downloading
		if (!*client->downloadName && client->nextSnapshotTime < proxy.server.svs->time + 1000)
		{
			client->nextSnapshotTime = proxy.server.svs->time + 1000;
		}
	}
}