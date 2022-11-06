#include "Proxy_EnginePatch.hpp"

/*
===============
SV_ClientCommand
===============
*/
static qboolean Proxy_SV_ClientCommand(client_t* cl, msg_t* msg) {
	int		seq;
	const char* s;
	qboolean clientOk = qtrue;

	seq = server.common.functions.MSG_ReadLong(msg);
	s = server.common.functions.MSG_ReadString(msg);

	// see if we have already executed it
	if (cl->lastClientCommand >= seq) {
		return qtrue;
	}

	server.common.functions.Com_DPrintf("clientCommand: %s : %i : %s\n", cl->name, seq, s);

	// drop the connection if we have somehow lost commands
	if (seq > cl->lastClientCommand + 1) {
		server.common.functions.Com_Printf("Client %s lost %i clientCommands\n", cl->name,
			seq - cl->lastClientCommand + 1);
		server.functions.SV_DropClient(cl, "Lost reliable commands");
		return qfalse;
	}

	// malicious users may try using too many string commands
	// to lag other players.  If we decide that we want to stall
	// the command, we will stop processing the rest of the packet,
	// including the usercmd.  This causes flooders to lag themselves
	// but not other people
	// We don't do this when the client hasn't been active yet since its
	// normal to spam a lot of commands when downloading
	if (!server.common.cvars.com_cl_running->integer &&
		cl->state >= CS_ACTIVE &&
		server.cvars.sv_floodProtect->integer &&
		server.svs->time < cl->nextReliableTime) {
		// ignore any other text messages from this client but let them keep playing
		clientOk = qfalse;
		server.common.functions.Com_DPrintf("client text ignored for %s\n", cl->name);
		//return qfalse;	// stop processing
	}

	// don't allow another command for one second
	cl->nextReliableTime = server.svs->time + 1000;

	server.functions.SV_ExecuteClientCommand(cl, s, clientOk);

	cl->lastClientCommand = seq;
	Com_sprintf(cl->lastClientCommandString, sizeof(cl->lastClientCommandString), "%s", s);

	return qtrue;		// continue procesing
}

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
static void Proxy_SV_UserMove(client_t* client, msg_t* msg, qboolean delta)
{
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
		server.common.functions.Com_Printf("cmdCount < 1\n");
		return;
	}

	if (cmdCount > MAX_PACKET_USERCMDS)
	{
		server.common.functions.Com_Printf("cmdCount > MAX_PACKET_USERCMDS\n");
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
	if (!proxy.originalEngineCvars.proxy_sv_pingFix.integer || client->frames[client->messageAcknowledge & PACKET_MASK].messageAcked == -1)
	{
		//cl->frames[cl->messageAcknowledge & PACKET_MASK].messageAcked = server.svs->time;
		client->frames[client->messageAcknowledge & PACKET_MASK].messageAcked = (proxy.originalEngineCvars.proxy_sv_pingFix.integer ? proxy.trap->Milliseconds() : server.svs->time);
	}
	// Proxy <--------------

	// if this is the first usercmd we have received
	// this gamestate, put the client into the world
	if (client->state == CS_PRIMED)
	{
		server.functions.SV_ClientEnterWorld(client, &cmds[0]);
		// the moves can be processed normaly
	}

	// a bad cp command was sent, drop the client
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

		if (proxy.originalEngineCvars.proxy_sv_enableNetStatus.integer)
		{
			Proxy_Server_UpdateUcmdStats(getClientNumFromAddr(client), &cmds[i], packetIndex);
			Proxy_Server_UpdateTimenudge(client, &cmds[i], proxy.trap->Milliseconds());
		}
		// Proxy <--------------
	}
}

/*
===================
SV_ExecuteClientMessage

Parse a client packet
===================
*/
void (*Original_SV_ExecuteClientMessage)(client_t*, msg_t*);
void Proxy_SV_ExecuteClientMessage(client_t* cl, msg_t* msg) {
	int			c;
	int			serverId;

	server.common.functions.MSG_Bitstream(msg);

	serverId = server.common.functions.MSG_ReadLong(msg);
	cl->messageAcknowledge = server.common.functions.MSG_ReadLong(msg);

	if (cl->messageAcknowledge < 0) {
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
		//server.functions.SV_DropClient( cl, "illegible client message" );
		return;
	}

	cl->reliableAcknowledge = server.common.functions.MSG_ReadLong(msg);

	// NOTE: when the client message is fux0red the acknowledgement numbers
	// can be out of range, this could cause the server to send thousands of server
	// commands which the server thinks are not yet acknowledged in SV_UpdateServerCommandsToClient
	if (cl->reliableAcknowledge < cl->reliableSequence - MAX_RELIABLE_COMMANDS) {
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
		//server.functions.SV_DropClient( cl, "illegible client message" );
		cl->reliableAcknowledge = cl->reliableSequence;
		return;
	}
	// if this is a usercmd from a previous gamestate,
	// ignore it or retransmit the current gamestate
	//
	// if the client was downloading, let it stay at whatever serverId and
	// gamestate it was at.  This allows it to keep downloading even when
	// the gamestate changes.  After the download is finished, we'll
	// notice and send it a new game state
	//
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=536
	// don't drop as long as previous command was a nextdl, after a dl is done, downloadName is set back to ""
	// but we still need to read the next message to move to next download or send gamestate
	// I don't like this hack though, it must have been working fine at some point, suspecting the fix is somewhere else
	if (serverId != server.sv->serverId && !*cl->downloadName && !strstr(cl->lastClientCommandString, "nextdl")) {
		if (serverId >= server.sv->restartedServerId && serverId < server.sv->serverId) { // TTimo - use a comparison here to catch multiple map_restart
			// they just haven't caught the map_restart yet
			server.common.functions.Com_DPrintf("%s : ignoring pre map_restart / outdated client message\n", cl->name);
			return;
		}
		// if we can tell that the client has dropped the last
		// gamestate we sent them, resend it
		// Fix for https://bugzilla.icculus.org/show_bug.cgi?id=6324
		if (cl->state != CS_ACTIVE && cl->messageAcknowledge > cl->gamestateMessageNum) {
			server.common.functions.Com_DPrintf("%s : dropped gamestate, resending\n", cl->name);
			server.functions.SV_SendClientGameState(cl);
		}
		return;
	}

	// read optional clientCommand strings
	do {
		c = server.common.functions.MSG_ReadByte(msg);
		if (c == clc_EOF) {
			break;
		}
		if (c != clc_clientCommand) {
			break;
		}
		if (!Proxy_SV_ClientCommand(cl, msg)) {
			return;	// we couldn't execute it because of the flood protection
		}
		if (cl->state == CS_ZOMBIE) {
			return;	// disconnect command
		}
	} while (1);

	// read the usercmd_t
	if (c == clc_move) {
		Proxy_SV_UserMove(cl, msg, qtrue);
	}
	else if (c == clc_moveNoDelta) {
		Proxy_SV_UserMove(cl, msg, qfalse);
	}
	else if (c != clc_EOF) {
		server.common.functions.Com_Printf("WARNING: bad command byte for client %i\n", getClientNumFromAddr(cl));
	}
	//	if ( msg->readcount != msg->cursize ) {
	//		server.common.functions.Com_Printf( "WARNING: Junk at end of packet for client %i\n", cl - svs.clients );
	//	}
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

		server.common.functions.Com_Printf("[ISM]SV_SendClientGameState() [2] for %s, writing out old fragments\n", client->name);
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
	server.functions.SV_SendMessageToClient(&msg, client);
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
		const size_t modelLen = strlen(val);

		if (!Q_IsValidAsciiStr(val) || modelLen >= /*MAX_QPATH*/ proxy.cvars.proxy_sv_modelPathLength.integer)
		{
			Info_SetValueForKey(cl->userinfo, "model", "kyle");
		}
	}

	Original_SV_UserinfoChanged(cl);
}

/*
==================
SV_BeginDownload_f
==================
*/
void (*Original_SV_BeginDownload_f)(client_t*);
void Proxy_SV_BeginDownload_f(client_t* cl)
{
	if (cl->state == CS_ACTIVE)
		return;
	
	const char* fileName = server.common.functions.Cmd_Argv(1);
	const size_t fileNameLength = strlen(fileName);

	if (fileNameLength < 5 || FS_CheckDirTraversal(fileName) || Q_stricmpn(&fileName[fileNameLength - 4], ".pk3", 4) != 0) {
		return;
	}

	Original_SV_BeginDownload_f(cl);
}

/*
==================
SV_CloseDownload

clear/free any download vars
==================
*/
static void Proxy_SV_CloseDownload(client_t* cl) {
	int i;

	// EOF
	if (cl->download) {
		server.common.functions.FS_FCloseFile(cl->download);
	}
	cl->download = 0;
	*cl->downloadName = 0;

	// Free the temporary buffer space
	for (i = 0; i < MAX_DOWNLOAD_WINDOW; i++) {
		if (cl->downloadBlocks[i]) {
			server.common.functions.Z_Free(cl->downloadBlocks[i]);
			cl->downloadBlocks[i] = NULL;
		}
	}
}

/*
==================
SV_NextDownload_f

The argument will be the last acknowledged block from the client, it should be
the same as cl->downloadClientBlock
==================
*/
static void Proxy_SV_NextDownload_f(client_t* cl)
{
	if (cl->state == CS_ACTIVE)
		return;

	int block = atoi(server.common.functions.Cmd_Argv(1));

	if (block == cl->downloadClientBlock) {
		server.common.functions.Com_DPrintf("clientDownload: %d : client acknowledge of block %d\n", cl - server.svs->clients, block);

		// Find out if we are done.  A zero-length block indicates EOF
		if (cl->downloadBlockSize[cl->downloadClientBlock % MAX_DOWNLOAD_WINDOW] == 0) {
			server.common.functions.Com_Printf("clientDownload: %d : file \"%s\" completed\n", cl - server.svs->clients, cl->downloadName);
			Proxy_SV_CloseDownload(cl);
			return;
		}

		cl->downloadSendTime = server.svs->time;
		cl->downloadClientBlock++;
		return;
	}
	// We aren't getting an acknowledge for the correct block, drop the client
	// FIXME: this is bad... the client will never parse the disconnect message
	//			because the cgame isn't loaded yet
	server.functions.SV_DropClient(cl, "broken download");
}

/*
==================
SV_StopDownload_f

Abort a download if in progress
==================
*/
static void Proxy_SV_StopDownload_f(client_t* cl)
{
	if (cl->state == CS_ACTIVE)
		return;

	if (*cl->downloadName)
		server.common.functions.Com_DPrintf("clientDownload: %d : file \"%s\" aborted\n", cl - server.svs->clients, cl->downloadName);

	Proxy_SV_CloseDownload(cl);
}

/*
==================
SV_DoneDownload_f

Downloads are finished
==================
*/
void (*Original_SV_DoneDownload_f)(client_t*);
void Proxy_SV_DoneDownload_f(client_t* cl)
{
	if (cl->state == CS_ACTIVE)
		return;

	Original_SV_DoneDownload_f(cl);
}

/*
=================
SV_ResetPureClient_f
=================
*/
static void Proxy_SV_ResetPureClient_f(client_t* cl) {
	cl->pureAuthentic = 0;
}

/*
=================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately  FIXME: move to game?
=================
*/
const char* Proxy_SV_GetStringEdString(char* refSection, char* refName);
static void Proxy_SV_Disconnect_f(client_t* cl) {
	//	SV_DropClient( cl, "disconnected" );
	server.functions.SV_DropClient(cl, Proxy_SV_GetStringEdString("MP_SVGAME", "DISCONNECTED"));
}

/*
==================
SV_UpdateUserinfo_f
==================
*/
#define INFO_CHANGE_MIN_INTERVAL	6000 //6 seconds is reasonable I suppose
#define INFO_CHANGE_MAX_COUNT		3 //only allow 3 changes within the 6 seconds

static void Proxy_SV_UpdateUserinfo_f(client_t* cl) {
	char* arg = server.common.functions.Cmd_Argv(1);

	// Stop random empty /userinfo calls without hurting anything
	if (!arg || !*arg)
		return;

#ifdef FINAL_BUILD
	if (cl->lastUserInfoChange > server.svs->time)
	{
		cl->lastUserInfoCount++;

		if (cl->lastUserInfoCount >= INFO_CHANGE_MAX_COUNT)
		{
			//	SV_SendServerCommand(cl, "print \"Warning: Too many info changes, last info ignored\n\"\n");
			server.functions.SV_SendServerCommand(cl, "print \"@@@TOO_MANY_INFO\n\"\n");
			return;
		}
	}
	else
#endif
	{
		cl->lastUserInfoCount = 0;
		cl->lastUserInfoChange = server.svs->time + INFO_CHANGE_MIN_INTERVAL;
	}

	Q_strncpyz(cl->userinfo, arg, sizeof(cl->userinfo));
	server.functions.SV_UserinfoChanged(cl);
	// call prog code to allow overrides
	Proxy_OldAPI_VM_Call(GAME_CLIENT_USERINFO_CHANGED, cl - server.svs->clients);
}

/*
=================
SV_VerifyPaks_f

If we are pure, disconnect the client if they do no meet the following conditions:

1. the first two checksums match our view of cgame and ui
2. there are no any additional checksums that we do not have

This routine would be a bit simpler with a goto but i abstained

=================
*/
static void Proxy_SV_VerifyPaks_f(client_t* cl) {
	int nChkSum1, nChkSum2, nClientPaks, nServerPaks, i, j, nCurArg;
	int nClientChkSum[1024];
	int nServerChkSum[1024];
	const char* pPaks, * pArg;
	qboolean bGood = qtrue;

	// if we are pure, we "expect" the client to load certain things from 
	// certain pk3 files, namely we want the client to have loaded the
	// ui and cgame that we think should be loaded based on the pure setting
	//
	if (server.cvars.sv_pure->integer != 0) {

		bGood = qtrue;
		nChkSum1 = nChkSum2 = 0;
		// we run the game, so determine which cgame and ui the client "should" be running
		//dlls are valid too now -rww
		if (server.common.functions.Cvar_VariableValue("vm_cgame"))
		{
			bGood = (qboolean)(server.common.functions.FS_FileIsInPAK("vm/cgame.qvm", &nChkSum1) == 1);
		}
		else
		{
			bGood = (qboolean)(server.common.functions.FS_FileIsInPAK("cgamex86.dll", &nChkSum1) == 1);
		}

		if (bGood)
		{
			if (server.common.functions.Cvar_VariableValue("vm_ui"))
			{
				bGood = (qboolean)(server.common.functions.FS_FileIsInPAK("vm/ui.qvm", &nChkSum2) == 1);
			}
			else
			{
				bGood = (qboolean)(server.common.functions.FS_FileIsInPAK("uix86.dll", &nChkSum2) == 1);
			}
		}

		nClientPaks = *server.common.vars.cmd_argc;

		// start at arg 1 ( skip cl_paks )
		nCurArg = 1;

		// we basically use this while loop to avoid using 'goto' :)
		while (bGood) {

			// must be at least 6: "cl_paks cgame ui @ firstref ... numChecksums"
			// numChecksums is encoded
			if (nClientPaks < 6) {
				bGood = qfalse;
				break;
			}
			// verify first to be the cgame checksum
			pArg = server.common.functions.Cmd_Argv(nCurArg++);
			if (!pArg || *pArg == '@' || atoi(pArg) != nChkSum1) {
				bGood = qfalse;
				break;
			}
			// verify the second to be the ui checksum
			pArg = server.common.functions.Cmd_Argv(nCurArg++);
			if (!pArg || *pArg == '@' || atoi(pArg) != nChkSum2) {
				bGood = qfalse;
				break;
			}
			// should be sitting at the delimeter now
			pArg = server.common.functions.Cmd_Argv(nCurArg++);
			if (*pArg != '@') {
				bGood = qfalse;
				break;
			}
			// store checksums since tokenization is not re-entrant
			for (i = 0; nCurArg < nClientPaks; i++) {
				nClientChkSum[i] = atoi(server.common.functions.Cmd_Argv(nCurArg++));
			}

			// store number to compare against (minus one cause the last is the number of checksums)
			nClientPaks = i - 1;

			// make sure none of the client check sums are the same
			// so the client can't send 5 the same checksums
			for (i = 0; i < nClientPaks; i++) {
				for (j = 0; j < nClientPaks; j++) {
					if (i == j)
						continue;
					if (nClientChkSum[i] == nClientChkSum[j]) {
						bGood = qfalse;
						break;
					}
				}
				if (bGood == qfalse)
					break;
			}
			if (bGood == qfalse)
				break;

			// get the pure checksums of the pk3 files loaded by the server
			pPaks = server.common.functions.FS_LoadedPakPureChecksums();
			server.common.functions.Cmd_TokenizeString(pPaks);
			nServerPaks = *server.common.vars.cmd_argc;
			if (nServerPaks > 1024)
				nServerPaks = 1024;

			for (i = 0; i < nServerPaks; i++) {
				nServerChkSum[i] = atoi(server.common.functions.Cmd_Argv(i));
			}

			// check if the client has provided any pure checksums of pk3 files not loaded by the server
			for (i = 0; i < nClientPaks; i++) {
				for (j = 0; j < nServerPaks; j++) {
					if (nClientChkSum[i] == nServerChkSum[j]) {
						break;
					}
				}
				if (j >= nServerPaks) {
					bGood = qfalse;
					break;
				}
			}
			if (bGood == qfalse) {
				break;
			}

			// check if the number of checksums was correct
			nChkSum1 = server.sv->checksumFeed;
			for (i = 0; i < nClientPaks; i++) {
				nChkSum1 ^= nClientChkSum[i];
			}
			nChkSum1 ^= nClientPaks;
			if (nChkSum1 != nClientChkSum[nClientPaks]) {
				bGood = qfalse;
				break;
			}

			// break out
			break;
		}

		if (bGood) {
			cl->pureAuthentic = 1;
		}
		else {
			cl->pureAuthentic = 0;
			cl->nextSnapshotTime = -1;
			cl->state = CS_ACTIVE;
			server.functions.SV_SendClientSnapshot(cl);
			server.functions.SV_DropClient(cl, "Unpure client detected. Invalid .PK3 files referenced!");
		}
	}
}

/*
==================
SV_ExecuteClientCommand
Also called by bot code
==================
*/
typedef struct {
	char* name;
	void	(*func)(client_t* cl);
} ucmd_t;

static ucmd_t ucmds[] = {
	{"userinfo", Proxy_SV_UpdateUserinfo_f},
	{"disconnect", Proxy_SV_Disconnect_f},
	{"cp", Proxy_SV_VerifyPaks_f},
	{"vdr", Proxy_SV_ResetPureClient_f},
	{"download", Proxy_SV_BeginDownload_f},
	{"nextdl", Proxy_SV_NextDownload_f},
	{"stopdl", Proxy_SV_StopDownload_f},
	{"donedl", Proxy_SV_DoneDownload_f},
	{NULL, NULL}
};

void (*Original_SV_ExecuteClientCommand)(client_t*, const char*, qboolean);
void Proxy_SV_ExecuteClientCommand(client_t* cl, const char* s, qboolean clientOK) {
	ucmd_t* u;

	server.common.functions.Cmd_TokenizeString(s);

	// see if it is a server level command
	for (u = ucmds; u->name; u++) {
		if (!strcmp(server.common.functions.Cmd_Argv(0), u->name)) {
			u->func(cl);
			break;
		}
	}

	if (clientOK) {
		// pass unknown strings to the game
		if (!u->name && server.sv->state == SS_GAME) {
			Proxy_OldAPI_VM_Call(GAME_CLIENT_COMMAND, cl - server.svs->clients);
		}
	}
}

/*
==================
SV_WriteDownloadToClient

Check to see if the client wants a file, open it if needed and start pumping the client
Fill up msg with data
==================
*/
void (*Original_SV_WriteDownloadToClient)(client_t*, msg_t*);
void Proxy_SV_WriteDownloadToClient(client_t* cl, msg_t* msg)
{
	int curindex;
	qboolean unreferenced = qtrue;
	char errorMessage[1024];
	char pakbuf[MAX_QPATH], * pakptr;
	int numRefPaks;

	if (!*cl->downloadName)
		return;	// Nothing being downloaded

	if (!cl->download)
	{
		// Chop off filename extension.
		Com_sprintf(pakbuf, sizeof(pakbuf), "%s", cl->downloadName);
		pakptr = strrchr(pakbuf, '.');

		if (pakptr)
		{
			*pakptr = '\0';

			// Check for pk3 filename extension
			if (!Q_stricmp(pakptr + 1, "pk3"))
			{
				const char* referencedPaks = server.common.functions.FS_ReferencedPakNames();

				// Check whether the file appears in the list of referenced
				// paks to prevent downloading of arbitrary files.
				Proxy_Cmd_TokenizeStringIgnoreQuotes(referencedPaks);
				numRefPaks = *server.common.vars.cmd_argc;

				for (curindex = 0; curindex < numRefPaks; curindex++)
				{
					if (!FS_FilenameCompare(server.common.functions.Cmd_Argv(curindex), pakbuf))
					{
						unreferenced = qfalse;

						break;
					}
				}
			}
		}

		cl->download = 0;

		if (unreferenced)
		{
			server.common.functions.Com_Printf("clientDownload: %d : \"%s\" is not referenced and cannot be downloaded.\n", (int)(cl - server.svs->clients), cl->downloadName);
			Com_sprintf(errorMessage, sizeof(errorMessage), "File \"%s\" is not referenced and cannot be downloaded.", cl->downloadName);
			
			server.common.functions.MSG_WriteByte(msg, svc_download);
			server.common.functions.MSG_WriteShort(msg, 0); // client is expecting block zero
			server.common.functions.MSG_WriteLong(msg, -1); // illegal file size
			server.common.functions.MSG_WriteString(msg, errorMessage);

			*cl->downloadName = 0;

			if (cl->download)
				server.common.functions.FS_FCloseFile(cl->download);

			return;
		}
	}

	Original_SV_WriteDownloadToClient(cl, msg);
}
