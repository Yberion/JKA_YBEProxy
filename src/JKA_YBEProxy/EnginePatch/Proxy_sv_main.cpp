#include "Proxy_EnginePatch.hpp"

/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/

void (*Original_SV_CalcPings)(void);
void Proxy_SV_CalcPings(void)
{
	int			i, j;
	client_t* cl;
	int			total, count;
	int			delta;
	playerState_t* ps;

	for (i = 0; i < server.cvars.sv_maxclients->integer; i++)
	{
		cl = &server.svs->clients[i];
		if (cl->state != CS_ACTIVE)
		{
			cl->ping = 999;
			continue;
		}
		if (!cl->gentity)
		{
			cl->ping = 999;
			continue;
		}
		if (cl->gentity->r.svFlags & SVF_BOT)
		{
			cl->ping = 0;
			continue;
		}

		total = 0;
		count = 0;

		for (j = 0; j < PACKET_BACKUP; j++)
		{
			// Proxy -------------->
			// if (cl->frames[j].messageAcked <= 0)
			if (cl->frames[j].messageAcked == -1)
			// Proxy <--------------
			{
				continue;
			}
			delta = cl->frames[j].messageAcked - cl->frames[j].messageSent;
			count++;
			total += delta;
		}
		if (!count)
		{
			cl->ping = 999;
		}
		else
		{
			cl->ping = total / count;
			
			if (cl->ping > 999)
			{
				cl->ping = 999;
			}

			// Proxy -------------->
			if (proxy.cvarsOldAPI.proxy_sv_pingFix.integer && cl->ping < 1)
			{
				cl->ping = 1;
			}
			// Proxy <--------------
		}

		// let the game dll know about the ping
		// Proxy -------------->
		// ps = SV_GameClientNum( i );
		ps = Proxy_GetPlayerStateByClientNum(i);
		// Proxy <--------------
		ps->ping = cl->ping;
	}
}

void (*Original_SVC_Status)(netadr_t);
void Proxy_SVC_Status(netadr_t from) {
	// Proxy -------------->
	// Fix q3infoboom
	// In Info_SetValueForKey if the infostring is too big then it will drop the server
	if (strlen(server.common.functions.Cmd_Argv(1)) > 128)
	{
		return;
	}
	// Proxy <--------------

	Original_SVC_Status(from);
}

void (*Original_SVC_Info)(netadr_t);
void Proxy_SVC_Info(netadr_t from) {
	// Proxy -------------->
	// Fix q3infoboom
	// In Info_SetValueForKey if the infostring is too big then it will drop the server
	if (strlen(server.common.functions.Cmd_Argv(1)) > 128)
	{
		return;
	}
	// Proxy <--------------

	Original_SVC_Info(from);
}

/*
===============
SVC_RemoteCommand

An rcon packet arrived from the network.
Shift down the remaining args
Redirect all printfs
===============
*/
void (*Original_SVC_RemoteCommand)(netadr_t, msg_t*);
void Proxy_SVC_RemoteCommand(netadr_t from, msg_t* msg) {
	// Only allow writeconfig on cfg files
	if (!Q_stricmpn(server.common.functions.Cmd_Argv(2), "writeconfig", 11)) {
		const char* arg3 = server.common.functions.Cmd_Argv(3);
		const size_t arg3Len = strlen(arg3);
		
		if (arg3Len >= 5 && arg3[arg3Len - 4] == '.' && Q_stricmpn(&arg3[arg3Len - 4], ".cfg", 4)) {
			return;
		}
	}

	Original_SVC_RemoteCommand(from, msg);
}

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void (*Original_SV_ConnectionlessPacket)(netadr_t, msg_t*);
void Proxy_SV_ConnectionlessPacket(netadr_t from, msg_t* msg) {
	char* s;
	char* c;

	server.common.functions.MSG_BeginReadingOOB(msg);
	server.common.functions.MSG_ReadLong(msg);		// skip the -1 marker

	if (!Q_strncmp("connect", (const char*)&msg->data[4], 7)) {
		server.common.functions.Huff_Decompress(msg, 12);
	}

	s = server.common.functions.MSG_ReadStringLine(msg);
	server.common.functions.Cmd_TokenizeString(s);

	c = server.common.functions.Cmd_Argv(0);

	if (server.common.cvars.com_developer->integer) {
		server.common.functions.Com_Printf("SV packet %s : %s\n", server.common.functions.NET_AdrToString(from), c);
	}

	if (!Q_stricmp(c, "getstatus")) {
		server.functions.SVC_Status(from);
	}
	else if (!Q_stricmp(c, "getinfo")) {
		server.functions.SVC_Info(from);
	}
	else if (!Q_stricmp(c, "getchallenge")) {
		server.functions.SV_GetChallenge(from);
	}
	else if (!Q_stricmp(c, "connect")) {
		server.functions.SV_DirectConnect(from);
	}
	else if (!Q_stricmp(c, "ipAuthorize")) {
		//SV_AuthorizeIpPacket(from);
	}
	else if (!Q_stricmp(c, "rcon")) {
		server.functions.SVC_RemoteCommand(from, msg);
	}
	else if (!Q_stricmp(c, "disconnect")) {
		// if a client starts up a local server, we may see some spurious
		// server disconnect messages when their new server sees our final
		// sequenced messages to the old client
	}
	else {
		if (server.common.cvars.com_developer->integer) {
			server.common.functions.Com_Printf("bad connectionless packet from %s:\n%s\n",
				server.common.functions.NET_AdrToString(from), s);
		}
	}
}

/*
=================
SV_ReadPackets
=================
*/
void (*Original_SV_PacketEvent)(netadr_t, msg_t*);
void Proxy_SV_PacketEvent(netadr_t from, msg_t* msg) {
	int			i;
	client_t* cl;
	int			qport;

	// check for connectionless packet (0xffffffff) first
	if (msg->cursize >= 4 && *(int*)msg->data == -1) {
		server.functions.SV_ConnectionlessPacket(from, msg);
		return;
	}

	// read the qport out of the message so we can fix up
	// stupid address translating routers
	server.common.functions.MSG_BeginReadingOOB(msg);
	server.common.functions.MSG_ReadLong(msg);				// sequence number
	qport = server.common.functions.MSG_ReadShort(msg) & 0xffff;

	// find which client the message is from
	for (i = 0, cl = server.svs->clients; i < server.cvars.sv_maxclients->integer; i++, cl++) {
		if (cl->state == CS_FREE) {
			continue;
		}
		if (!server.common.functions.NET_CompareBaseAdr(from, cl->netchan.remoteAddress)) {
			continue;
		}
		// it is possible to have multiple clients from a single IP
		// address, so they are differentiated by the qport variable
		if (cl->netchan.qport != qport) {
			continue;
		}

		// the IP port can't be used to differentiate them, because
		// some address translating routers periodically change UDP
		// port assignments
		if (cl->netchan.remoteAddress.port != from.port) {
			server.common.functions.Com_Printf("SV_ReadPackets: fixing up a translated port\n");
			cl->netchan.remoteAddress.port = from.port;
		}

		// make sure it is a valid, in sequence packet
		if (server.functions.SV_Netchan_Process(cl, msg)) {
			// zombie clients still need to do the Netchan_Process
			// to make sure they don't need to retransmit the final
			// reliable message, but they don't do any other processing
			if (cl->state != CS_ZOMBIE) {
				cl->lastPacketTime = server.svs->time;	// don't timeout
				server.functions.SV_ExecuteClientMessage(cl, msg);
			}
		}
		return;
	}

	// if we received a sequenced packet from an address we don't recognize,
	// send an out of band disconnect packet to it
	server.common.functions.NET_OutOfBandPrint(NS_SERVER, from, "disconnect");
}
