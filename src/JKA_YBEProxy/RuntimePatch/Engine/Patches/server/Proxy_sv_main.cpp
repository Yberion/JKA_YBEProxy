#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Wrappers.hpp"
#include "JKA_YBEProxy/Imports/server/sv_game.hpp"
#include "JKA_YBEProxy/Proxy_Header.hpp"
#include "sdk/game/g_public.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_main.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_RateLimit.hpp"

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
			// Not "<= 0": with proxy_sv_pingFix the times come from Sys_Milliseconds, negative after about 24.8 days of uptime
			// -1: sent, not acknowledged yet. Both 0: never sent since the client slot was cleared on connect
			if (cl->frames[j].messageAcked == -1 || (cl->frames[j].messageSent == 0 && cl->frames[j].messageAcked == 0))
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
			if (proxy.originalEngineCvars.proxy_sv_pingFix.integer && cl->ping < 1)
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

/*
=================
SV_PacketEvent filter

Called from the stub at the entry of SV_PacketEvent (see Proxy_Engine_Patch.cpp), with a copy of the arguments.
Returns qfalse to drop the packet. Otherwise the original SV_PacketEvent runs with no proxy frame on the stack:
an rcon map change unloads the proxy from inside the packet code.
=================
*/
void (*Original_SV_PacketEvent)(netadr_t, msg_t*);
qboolean QDECL Proxy_SV_PacketEvent_Filter(netadr_t from, msg_t* msg)
{
	// Sequenced game packet: nothing to filter
	if (msg->cursize < 4 || *(int*)msg->data != -1)
	{
		return qtrue;
	}

	// connect is Huffman-compressed and the original decompresses it in place: leave it alone
	if (!Q_strncmp("connect", (const char*)&msg->data[4], 7))
	{
		// Per address only, as Quake3e: reads nothing but the address
		return Proxy_Engine_RateLimit_AddressAllowed(from, "connect") ? qtrue : qfalse;
	}

	// Same reading as the original SV_ConnectionlessPacket, which reads and tokenizes again afterwards
	server.common.functions.MSG_BeginReadingOOB(msg);
	server.common.functions.MSG_ReadLong(msg); // skip the -1 marker
	server.common.functions.Cmd_TokenizeString(server.common.functions.MSG_ReadStringLine(msg));

	const char* c = server.common.functions.Cmd_Argv(0);

	if (!Q_stricmp(c, "getstatus") || !Q_stricmp(c, "getinfo"))
	{
		// Fix q3infoboom
		// In Info_SetValueForKey if the infostring is too big then it will drop the server
		if (std::strlen(server.common.functions.Cmd_Argv(1)) > 128)
		{
			return qfalse;
		}

		return Proxy_Engine_RateLimit_StatusAllowed(from, c) ? qtrue : qfalse;
	}

	// Per address only, as Quake3e: a global bucket would let a getstatus flood block every connection
	if (!Q_stricmp(c, "getchallenge"))
	{
		return Proxy_Engine_RateLimit_AddressAllowed(from, c) ? qtrue : qfalse;
	}

	if (!Q_stricmp(c, "rcon"))
	{
		return Proxy_Engine_RateLimit_RconAllowed() ? qtrue : qfalse;
	}

	// The original only answers the old authorize server, but prints a line for every other sender
	if (!Q_stricmp(c, "ipAuthorize"))
	{
		return qfalse;
	}

	return qtrue;
}
