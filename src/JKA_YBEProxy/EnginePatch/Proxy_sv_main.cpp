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
			if (proxy.cvarsOldAPI.sv_pingFix.integer && cl->ping < 1)
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
	qboolean	valid;
	char		remaining[1024];
	// TTimo - scaled down to accumulate, but not overflow anything network wise, print wise etc.
	// (OOB messages are the bottleneck here)
#define	SV_OUTPUTBUF_LENGTH	(1024 - 16)
	char		sv_outputbuf[SV_OUTPUTBUF_LENGTH];
	char* cmd_aux;

	if (!strlen(server.cvars.sv_rconPassword->string) ||
		strcmp(server.common.functions.Cmd_Argv(1), server.cvars.sv_rconPassword->string)) {
		valid = qfalse;
		Proxy_Common_Com_Printf("Bad rcon from %s: %s\n", server.common.functions.NET_AdrToString(from), Cmd_ArgsFrom(2));
	}
	else {
		valid = qtrue;
		Proxy_Common_Com_Printf("Rcon from %s: %s\n", server.common.functions.NET_AdrToString(from), Cmd_ArgsFrom(2));
	}

	// start redirecting all print outputs to the packet
	server.svs->redirectAddress = from;
	server.common.functions.Com_BeginRedirect(sv_outputbuf, SV_OUTPUTBUF_LENGTH, server.functions.SV_FlushRedirect);

	if (!strlen(server.cvars.sv_rconPassword->string)) {
		Proxy_Common_Com_Printf("No rconpassword set.\n");
	}
	else if (!valid) {
		Proxy_Common_Com_Printf("Bad rconpassword.\n");
	}
	else {
		remaining[0] = 0;

		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
		// get the command directly, "rcon <pass> <command>" to avoid quoting issues
		// extract the command by walking
		// since the cmd formatting can fuckup (amount of spaces), using a dumb step by step parsing
		cmd_aux = Cmd_Cmd();
		cmd_aux += 4;
		while (cmd_aux[0] == ' ')
			cmd_aux++;
		while (cmd_aux[0] && cmd_aux[0] != ' ') // password
			cmd_aux++;
		while (cmd_aux[0] == ' ')
			cmd_aux++;

		Q_strcat(remaining, sizeof(remaining), cmd_aux);

		server.common.functions.Cmd_ExecuteString(remaining);
	}
	server.common.functions.Com_EndRedirect();
}
