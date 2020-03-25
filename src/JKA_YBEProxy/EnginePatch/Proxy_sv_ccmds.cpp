#include "Proxy_EnginePatch.hpp"
#include "qcommon/game_version.hpp"

/*
================
SV_Status_f
================
*/

void (*Original_SV_Status_f)(void);
void Proxy_SV_Status_f(void)
{
	int				i;
	int				humans;
	int				bots;
	client_t*		cl;
	playerState_t*	ps;
	const char*		s;
	int				ping;
	char			state[32];

	// make sure server is running
	if (!proxy.server.common.com_sv_running->integer)
	{
		Proxy_Common_Com_Printf("Server is not running.\n");

		return;
	}

	humans = bots = 0;

	for (i = 0; i < proxy.server.cvars.sv_maxclients->integer; i++)
	{
		if (proxy.server.svs->clients[i].state >= CS_CONNECTED)
		{
			if (proxy.server.svs->clients[i].netchan.remoteAddress.type != NA_BOT)
			{
				humans++;
			}
			else
			{
				bots++;
			}
		}
	}

#if defined(_WIN32)
	#define STATUS_OS "Windows"
#elif defined(__linux__)
	#define STATUS_OS "Linux"
#elif defined(MACOS_X)
	#define STATUS_OS "OSX"
#else
	#define STATUS_OS "Unknown"
#endif

	const char* ded_table[] = { "Listen", "LAN dedicated", "Public dedicated" };
	const char* gametypeNames[] = { "FFA", "Holocron", "Jedimaster", "Duel", "PowerDuel", "Single", "TFFA", "Siege", "CTF", "CTY" };
	char hostname[MAX_HOSTNAMELENGTH] = { 0 };

	Q_strncpyz(hostname, proxy.server.cvars.sv_hostname->string, sizeof(hostname));
	Q_StripColor(hostname);

	Proxy_Common_Com_Printf("hostname: %s^7\n", hostname);
	Proxy_Common_Com_Printf("server  : %s:%i, %s, %s\n", proxy.server.common.Cvar_VariableString("net_ip"), proxy.trap->Cvar_VariableIntegerValue("net_port"), STATUS_OS, ded_table[proxy.server.common.com_dedicated->integer]);
	Proxy_Common_Com_Printf("game    : %s %i, %s\n", VERSION_STRING_DOTTED, PROTOCOL_VERSION, FS_GetCurrentGameDir());
	Proxy_Common_Com_Printf("map     : ^7%s^7, %s(%i)\n", proxy.server.cvars.sv_mapname->string, gametypeNames[proxy.server.cvars.sv_gametype->integer], proxy.server.cvars.sv_gametype->integer);//Do we need to validate sv_gametype is 0-9? don't think so
	Proxy_Common_Com_Printf("players : %i %s, %i %s(%i max)\n", humans, (humans == 1 ? "human" : "humans"), bots, (bots == 1 ? "bot" : "bots"), proxy.server.cvars.sv_maxclients->integer - proxy.server.cvars.sv_privateClients->integer);

	Proxy_Common_Com_Printf("score ping rate   address                id name \n");
	Proxy_Common_Com_Printf("----- ---- ------ ---------------------- -- ---------------\n");

	for (i = 0, cl = proxy.server.svs->clients; i < proxy.server.cvars.sv_maxclients->integer; i++, cl++)
	{
		if (!cl->state)
			continue;

		if (cl->state == CS_CONNECTED)
			Q_strncpyz(state, "CON ", sizeof(state));
		else if (cl->state == CS_ZOMBIE)
			Q_strncpyz(state, "ZMB ", sizeof(state));
		else
		{
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Com_sprintf(state, sizeof(state), "%4i", ping);
		}

		ps = Proxy_GetPlayerStateByClientNum(i);
		s = proxy.server.common.NET_AdrToString(cl->netchan.remoteAddress);

		//No need for truncation "feature" if we move name to end
		Proxy_Common_Com_Printf("%5i %4s %6i %22s %2i %s^7\n", ps->persistant[PERS_SCORE], state, cl->rate, s, i, cl->name);
	}

	Proxy_Common_Com_Printf("\n");
}