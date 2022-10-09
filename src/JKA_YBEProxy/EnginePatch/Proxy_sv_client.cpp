#include "Proxy_EnginePatch.hpp"

/*
==================
SV_UserMove

The message usually contains all the movement commands 
that were in the last three packets, so that the information
in dropped packets can be recovered.

On very fast clients, there may be multiple usercmd packed into
each of the backup packets.
==================
*/

void (*Original_SV_UserMove)(client_t*, msg_t*, qboolean);
void Proxy_SV_UserMove(client_t* client, msg_t* msg, qboolean delta)
{
	// Proxy -------------->
#if defined(_WIN32) && !defined(MINGW32)
	__asm2__(mov [client], EBX); // client_t* client
	__asm1__(push ECX);
	__asm2__(mov ECX, DWORD PTR SS : [ESP + 0x3A4]);
	__asm2__(mov [msg], ECX); // msg_t* msg
	__asm1__(pop ECX);
	__asm2__(mov [delta], EAX); // qboolean delta
#endif
	// Proxy <--------------

	int			i, key;
	int			cmdCount;
	usercmd_t	nullcmd;
	usercmd_t	cmds[MAX_PACKET_USERCMDS];
	usercmd_t* cmd, * oldcmd;

	if (delta)
	{
		client->deltaMessage = client->messageAcknowledge;
	}
	else
	{
		client->deltaMessage = -1;
	}

	cmdCount = server.common.functions.MSG_ReadByte(msg);

	if (cmdCount < 1)
	{
		Proxy_Common_Com_Printf("cmdCount < 1\n");
		return;
	}

	if (cmdCount > MAX_PACKET_USERCMDS)
	{
		Proxy_Common_Com_Printf("cmdCount > MAX_PACKET_USERCMDS\n");
		return;
	}

	// use the checksum feed in the key
	key = server.sv->checksumFeed;
	// also use the message acknowledge
	key ^= client->messageAcknowledge;
	// also use the last acknowledged server command in the key
	key ^= server.common.functions.Com_HashKey(client->reliableCommands[client->reliableAcknowledge & (MAX_RELIABLE_COMMANDS - 1)], 32);

	Com_Memset(&nullcmd, 0, sizeof(nullcmd));
	oldcmd = &nullcmd;
	for (i = 0; i < cmdCount; i++)
	{
		cmd = &cmds[i];
		server.common.functions.MSG_ReadDeltaUsercmdKey(msg, key, oldcmd, cmd);
		oldcmd = cmd;
	}

	// save time for ping calculation
	// Proxy -------------->
	if (!proxy.cvarsOldAPI.sv_pingFix.integer || client->frames[client->messageAcknowledge & PACKET_MASK].messageAcked == -1)
	{
		//cl->frames[cl->messageAcknowledge & PACKET_MASK].messageAcked = server.svs->time;
		client->frames[client->messageAcknowledge & PACKET_MASK].messageAcked = (proxy.cvarsOldAPI.sv_pingFix.integer ? proxy.trap->Milliseconds() : server.svs->time);
	}
	// Proxy <--------------

	// if this is the first usercmd we have received
	// this gamestate, put the client into the world
	if (client->state == CS_PRIMED)
	{
		server.functions.SV_ClientEnterWorld(client, &cmds[0]);
		// the moves can be processed normaly
	}

	if (server.cvars.sv_pure->integer != 0 && client->pureAuthentic == 0)
	{
		server.functions.SV_DropClient(client, "Cannot validate pure client!");
		return;
	}

	if (client->state != CS_ACTIVE)
	{
		client->deltaMessage = -1;
		return;
	}

	// Proxy -------------->
	int		packetIndex;

	packetIndex = proxy.clientData[getClientNumFromAddr(client)].cmdIndex;
	// Proxy <--------------

	// usually, the first couple commands will be duplicates
	// of ones we have previously received, but the servertimes
	// in the commands will cause them to be immediately discarded
	for (i = 0; i < cmdCount; i++)
	{
		// if this is a cmd from before a map_restart ignore it
		if (cmds[i].serverTime > cmds[cmdCount - 1].serverTime)
		{
			continue;
		}
		// extremely lagged or cmd from before a map_restart
		//if ( cmds[i].serverTime > svs.time + 3000 ) {
		//	continue;
		//}
		// don't execute if this is an old cmd which is already executed
		// these old cmds are included when cl_packetdup > 0
		if (cmds[i].serverTime <= client->lastUsercmd.serverTime)
		{
			continue;
		}

		server.functions.SV_ClientThink(client, &cmds[i]);

		// Proxy -------------->
		if (client->ping < 1)
		{
			continue;
		}

		Proxy_Server_UpdateUcmdStats(getClientNumFromAddr(client), &cmds[i], packetIndex);
		Proxy_Server_UpdateTimenudge(client, &cmds[i], proxy.trap->Milliseconds());
		// Proxy <--------------
	}
}

/*
================
SV_SendClientGameState

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each new map load.

It will be resent if the client acknowledges a later message but has
the wrong gamestate.
================
*/

void (*Original_SV_SendClientGameState)(client_t*);
void Proxy_SV_SendClientGameState(client_t* client)
{
	int				start;
	entityState_t*	base, nullstate;
	msg_t			msg;
	byte			msgBuffer[MAX_MSGLEN];

	// MW - my attempt to fix illegible server message errors caused by 
	// packet fragmentation of initial snapshot.
	while (client->state && client->netchan.unsentFragments)
	{
		// send additional message fragments if the last message
		// was too large to send at once

		Proxy_Common_Com_Printf("[ISM]SV_SendClientGameState() [2] for %s, writing out old fragments\n", client->name);
		server.common.functions.Netchan_TransmitNextFragment(&client->netchan);
	}

	server.common.functions.Com_DPrintf("SV_SendClientGameState() for %s\n", client->name);
	server.common.functions.Com_DPrintf("Going from CS_CONNECTED to CS_PRIMED for %s\n", client->name);
	
	// Proxy -------------->
	if (client->state == CS_CONNECTED)
	{
		client->state = CS_PRIMED;
	}
	// Proxy <--------------

	client->pureAuthentic = 0;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	client->gamestateMessageNum = client->netchan.outgoingSequence;

	server.common.functions.MSG_Init(&msg, msgBuffer, sizeof(msgBuffer));

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	server.common.functions.MSG_WriteLong(&msg, client->lastClientCommand);

	// send any server commands waiting to be sent first.
	// we have to do this cause we send the client->reliableSequence
	// with a gamestate and it sets the clc.serverCommandSequence at
	// the client side
	server.functions.SV_UpdateServerCommandsToClient(client, &msg);

	// send the gamestate
	server.common.functions.MSG_WriteByte(&msg, svc_gamestate);
	server.common.functions.MSG_WriteLong(&msg, client->reliableSequence);

	// write the configstrings
	for (start = 0; start < MAX_CONFIGSTRINGS; start++)
	{
		if (server.sv->configstrings[start][0])
		{
			server.common.functions.MSG_WriteByte(&msg, svc_configstring);
			server.common.functions.MSG_WriteShort(&msg, start);
			server.common.functions.MSG_WriteBigString(&msg, server.sv->configstrings[start]);
		}
	}

	// write the baselines
	Com_Memset(&nullstate, 0, sizeof(nullstate));

	for (start = 0; start < MAX_GENTITIES; start++)
	{
		base = &server.sv->svEntities[start].baseline;

		if (!base->number)
		{
			continue;
		}

		server.common.functions.MSG_WriteByte(&msg, svc_baseline);
		server.common.functions.MSG_WriteDeltaEntity(&msg, &nullstate, base, qtrue);
	}

	server.common.functions.MSG_WriteByte(&msg, svc_EOF);

	server.common.functions.MSG_WriteLong(&msg, client - server.svs->clients);

	// write the checksum feed
	server.common.functions.MSG_WriteLong(&msg, server.sv->checksumFeed);

	/*
	//rwwRMG - send info for the terrain
	if (TheRandomMissionManager)
	{
		z_stream zdata;

		// Send the height map
		memset(&zdata, 0, sizeof(z_stream));
		deflateInit(&zdata, Z_MAX_COMPRESSION);

		unsigned char heightmap[15000];
		zdata.next_out = (unsigned char*)heightmap;
		zdata.avail_out = 15000;
		zdata.next_in = TheRandomMissionManager->GetLandScape()->GetHeightMap();
		zdata.avail_in = TheRandomMissionManager->GetLandScape()->GetRealArea();
		deflate(&zdata, Z_SYNC_FLUSH);

		server.common.MSG_WriteShort(&msg, (unsigned short)zdata.total_out);
		MSG_WriteBits(&msg, 1, 1);
		MSG_WriteData(&msg, heightmap, zdata.total_out);

		deflateEnd(&zdata);

		// Send the flatten map
		memset(&zdata, 0, sizeof(z_stream));
		deflateInit(&zdata, Z_MAX_COMPRESSION);

		zdata.next_out = (unsigned char*)heightmap;
		zdata.avail_out = 15000;
		zdata.next_in = TheRandomMissionManager->GetLandScape()->GetFlattenMap();
		zdata.avail_in = TheRandomMissionManager->GetLandScape()->GetRealArea();
		deflate(&zdata, Z_SYNC_FLUSH);

		server.common.MSG_WriteShort(&msg, (unsigned short)zdata.total_out);
		MSG_WriteBits(&msg, 1, 1);
		MSG_WriteData(&msg, heightmap, zdata.total_out);

		deflateEnd(&zdata);

		// Seed is needed for misc ents and noise
		server.common.MSG_WriteLong(&msg, TheRandomMissionManager->GetLandScape()->get_rand_seed());

		SV_WriteRMGAutomapSymbols(&msg);
	}
	else
	{
		server.common.MSG_WriteShort(&msg, 0);
	}
	*/

	// Proxy -------------->
	// For old RMG system.
	server.common.functions.MSG_WriteShort(&msg, 0);
	// Proxy <--------------

	// deliver this to the client
	Proxy_SV_SendMessageToClient(&msg, client);
}

/*
=================
SV_UserinfoChanged
Pull specific info from a newly changed userinfo string
into a more C friendly form.
=================
*/
void (*Original_SV_UserinfoChanged)(client_t*);
void Proxy_SV_UserinfoChanged(client_t* cl) {
	char* val = nullptr;

	val = Info_ValueForKey(cl->userinfo, "model");

	// Fix model length crash on some custom clients
	// This check is needed here first on the connection, before it goes to SV_UpdateUserinfo_f -> ClientUserinfoChanged
	// There's also a check done in ClientUserinfoChanged
	if (val)
	{
		const size_t modelLen = (int)strlen(val);

		if (modelLen > MAX_QPATH)
		{
			Info_SetValueForKey(cl->userinfo, "model", "kyle");
		}
	}

	Original_SV_UserinfoChanged(cl);
}
