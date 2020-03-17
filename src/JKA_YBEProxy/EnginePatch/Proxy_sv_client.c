#include "Proxy_EnginePatch.h"

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

void Proxy_SV_UserMove(client_t* client, msg_t* msg, qboolean delta)
{
	__asm2__(mov dword ptr ds : [client] , ebx);
	__asm2__(mov dword ptr ss : [msg] , ESP);
	__asm2__(mov dword ptr ds : [delta] , eax);

	int			i, key;
	int			cmdCount;
	usercmd_t	nullcmd;
	usercmd_t	cmds[MAX_PACKET_USERCMDS];
	usercmd_t* cmd, * oldcmd;

	proxy.server.functions.Com_Printf("AAAAAAAAAAAAAAAAAAAAAA\n");

	proxy.server.functions.Com_Printf("CLIENT NAME: %s\n", client->name);

	if (delta)
	{
		proxy.server.functions.Com_Printf("A\n");
		client->deltaMessage = client->messageAcknowledge;
		proxy.server.functions.Com_Printf("B\n");
	}
	else
	{
		proxy.server.functions.Com_Printf("C\n");
		client->deltaMessage = -1;
		proxy.server.functions.Com_Printf("D\n");
	}

	proxy.server.functions.Com_Printf("E\n");

	proxy.server.functions.Com_Printf("MAXSIZE: %d\n", msg->maxsize);

	//__asm2__(mov ECX, [msg]);
	//__asm1__(push EBP);
	//__asm1__(push ECX);
	cmdCount = proxy.server.functions.MSG_ReadByte(msg);
	__asm2__(ADD ESP, 0x4);

	proxy.server.functions.Com_Printf("F\n");

	if (cmdCount < 1)
	{
		proxy.server.functions.Com_Printf("cmdCount < 1\n");
		return;
	}

	proxy.server.functions.Com_Printf("G\n");

	if (cmdCount > MAX_PACKET_USERCMDS)
	{
		proxy.server.functions.Com_Printf("cmdCount > MAX_PACKET_USERCMDS\n");
		return;
	}

	proxy.server.functions.Com_Printf("H\n");

	// use the checksum feed in the key
	key = proxy.server.sv->checksumFeed;
	// also use the message acknowledge
	key ^= client->messageAcknowledge;
	// also use the last acknowledged server command in the key
	key ^= proxy.server.functions.Com_HashKey(client->reliableCommands[client->reliableAcknowledge & (MAX_RELIABLE_COMMANDS - 1)], 32);

	proxy.server.functions.Com_Printf("I\n");

	Com_Memset(&nullcmd, 0, sizeof(nullcmd));
	oldcmd = &nullcmd;
	for (i = 0; i < cmdCount; i++)
	{
		cmd = &cmds[i];
		proxy.server.functions.MSG_ReadDeltaUsercmdKey(msg, key, oldcmd, cmd);
		oldcmd = cmd;
	}

	proxy.server.functions.Com_Printf("G\n");

	// save time for ping calculation
	//cl->frames[cl->messageAcknowledge & PACKET_MASK].messageAcked = proxy.server.svs->time;

    if (client->frames[client->messageAcknowledge & PACKET_MASK].messageAcked == -1)
    {
		client->frames[client->messageAcknowledge & PACKET_MASK].messageAcked = proxy.trap->Milliseconds();
    }

	// if this is the first usercmd we have received
	// this gamestate, put the client into the world
	if (client->state == CS_PRIMED)
	{
		proxy.server.functions.SV_ClientEnterWorld(client, &cmds[0]);
		// the moves can be processed normaly
	}

	if (proxy.server.cvar.sv_pure->integer != 0 && client->pureAuthentic == 0)
	{
		proxy.server.functions.SV_DropClient(client, "Cannot validate pure client!");
		return;
	}

	if (client->state != CS_ACTIVE)
	{
		client->deltaMessage = -1;
		return;
	}

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
	}
}
