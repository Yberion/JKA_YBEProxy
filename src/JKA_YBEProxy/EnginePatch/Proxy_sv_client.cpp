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

	cmdCount = proxy.server.functions.MSG_ReadByte(msg);

	if (cmdCount < 1)
	{
		proxy.server.functions.Com_Printf("cmdCount < 1\n");
		return;
	}

	if (cmdCount > MAX_PACKET_USERCMDS)
	{
		proxy.server.functions.Com_Printf("cmdCount > MAX_PACKET_USERCMDS\n");
		return;
	}

	// use the checksum feed in the key
	key = proxy.server.sv->checksumFeed;
	// also use the message acknowledge
	key ^= client->messageAcknowledge;
	// also use the last acknowledged server command in the key
	key ^= proxy.server.functions.Com_HashKey(client->reliableCommands[client->reliableAcknowledge & (MAX_RELIABLE_COMMANDS - 1)], 32);

	Com_Memset(&nullcmd, 0, sizeof(nullcmd));
	oldcmd = &nullcmd;
	for (i = 0; i < cmdCount; i++)
	{
		cmd = &cmds[i];
		proxy.server.functions.MSG_ReadDeltaUsercmdKey(msg, key, oldcmd, cmd);
		oldcmd = cmd;
	}

	// save time for ping calculation
	// Proxy -------------->
	if (client->frames[client->messageAcknowledge & PACKET_MASK].messageAcked == -1)
	{
	//cl->frames[cl->messageAcknowledge & PACKET_MASK].messageAcked = proxy.server.svs->time;
		client->frames[client->messageAcknowledge & PACKET_MASK].messageAcked = proxy.trap->Milliseconds();
	}
	// Proxy <--------------

	// if this is the first usercmd we have received
	// this gamestate, put the client into the world
	if (client->state == CS_PRIMED)
	{
		proxy.server.functions.SV_ClientEnterWorld(client, &cmds[0]);
		// the moves can be processed normaly
	}

	if (proxy.server.cvars.sv_pure->integer != 0 && client->pureAuthentic == 0)
	{
		proxy.server.functions.SV_DropClient(client, "Cannot validate pure client!");
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

		proxy.server.functions.SV_ClientThink(client, &cmds[i]);

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
