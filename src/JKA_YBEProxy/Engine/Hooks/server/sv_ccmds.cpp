#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Engine/EngineState.hpp"
#include "sdk/qcommon/game_version.hpp"
#include "JKA_YBEProxy/Ports/server/sv_game.hpp"
#include "JKA_YBEProxy/Engine/Hooks/qcommon/common.hpp"
#include "JKA_YBEProxy/Engine/Hooks/qcommon/files.hpp"
#include "JKA_YBEProxy/Engine/Hooks/server/sv_ccmds.hpp"

namespace ybeproxy::engine::hooks
{

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
	if (!server.common.cvars.com_sv_running->integer)
	{
		server.common.functions.Com_Printf("Server is not running.\n");

		return;
	}

	humans = bots = 0;

	for (i = 0; i < server.cvars.sv_maxclients->integer; i++)
	{
		if (server.svs->clients[i].state >= CS_CONNECTED)
		{
			if (server.svs->clients[i].netchan.remoteAddress.type != NA_BOT)
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

	Q_strncpyz(hostname, server.cvars.sv_hostname->string, sizeof(hostname));
	Q_StripColor(hostname);

	server.common.functions.Com_Printf("hostname: %s^7\n", hostname);
	server.common.functions.Com_Printf("server  : %s:%i, %s, %s\n", server.common.functions.Cvar_VariableString("net_ip"), proxy.trap->Cvar_VariableIntegerValue("net_port"), STATUS_OS, ded_table[server.common.cvars.com_dedicated->integer]);
	server.common.functions.Com_Printf("game    : %s %i, %s\n", VERSION_STRING_DOTTED, PROTOCOL_VERSION, Proxy_FS_GetCurrentGameDir());
	// The game clamps g_gametype at GAME_INIT, so it should always be in range, but the check costs nothing
	const int gametype = server.cvars.sv_gametype->integer;
	const char* gametypeName = (gametype >= 0 && gametype < (int)ARRAY_LEN(gametypeNames)) ? gametypeNames[gametype] : "Unknown";

	server.common.functions.Com_Printf("map     : ^7%s^7, %s(%i)\n", server.cvars.sv_mapname->string, gametypeName, gametype);
	server.common.functions.Com_Printf("players : %i %s, %i %s(%i max)\n", humans, (humans == 1 ? "human" : "humans"), bots, (bots == 1 ? "bot" : "bots"), server.cvars.sv_maxclients->integer - server.cvars.sv_privateClients->integer);

	server.common.functions.Com_Printf("score ping rate   address                id name \n");
	server.common.functions.Com_Printf("----- ---- ------ ---------------------- -- ---------------\n");

	for (i = 0, cl = server.svs->clients; i < server.cvars.sv_maxclients->integer; i++, cl++)
	{
		if (!cl->state)
			continue;

		if (cl->state == CS_CONNECTED)
			Q_strncpyz(state, "CON ", sizeof(state));
		else if (cl->state == CS_ZOMBIE)
			Q_strncpyz(state, "ZMB ", sizeof(state));
		else if (cl->netchan.remoteAddress.type == NA_BOT)
			Q_strncpyz(state, "BOT ", sizeof(state));
		else
		{
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Com_sprintf(state, sizeof(state), "%4i", ping);
		}

		ps = ports::GetPlayerState(i);
		s = server.common.functions.NET_AdrToString(cl->netchan.remoteAddress);

		//No need for truncation "feature" if we move name to end
		server.common.functions.Com_Printf("%5i %4s %6i %22s %2i %s^7\n", ps->persistant[PERS_SCORE], state, cl->rate, s, i, cl->name);
	}

	server.common.functions.Com_Printf("\n");
}

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

const char* Proxy_SV_GetStringEdString(const char* /*refSection*/, const char* refName)
{
	//Well, it would've been lovely doing it the above way, but it would mean mixing
	//languages for the client depending on what the server is. So we'll mark this as
	//a stringed reference with @@@ and send the refname to the client, and when it goes
	//to print it will get scanned for the stringed reference indication and dealt with
	//properly.
	static char text[1024] = { 0 };
	Com_sprintf(text, sizeof(text), "@@@%s", refName);
	return text;
}

/*
================
SV_DumpUser_f

The original only finds a player by name. A client number now works too, as in ioq3, OpenJK and Quake3e
(SV_GetPlayerByHandle). Anything else, a name or a bad number, goes to the original command
================
*/

void (*Original_SV_DumpUser_f)(void);
void Proxy_SV_DumpUser_f(void)
{
	if (server.common.cvars.com_sv_running->integer && *server.common.vars.cmd_argc == 2)
	{
		const char* arg = server.common.functions.Cmd_Argv(1);
		const char* c = arg;

		while (*c >= '0' && *c <= '9')
		{
			++c;
		}

		// An empty argument is not a number
		if (c != arg && !*c)
		{
			const int clientNum = atoi(arg);

			// A number too long for an int is not a client number: the original then looks for a player of that name
			if (c - arg <= 4 && clientNum < server.cvars.sv_maxclients->integer && server.svs->clients[clientNum].state)
			{
				server.common.functions.Com_Printf("userinfo\n");
				server.common.functions.Com_Printf("--------\n");
				Proxy_Info_Print(server.svs->clients[clientNum].userinfo);

				return;
			}
		}
	}

	Original_SV_DumpUser_f();
}

} // namespace ybeproxy::engine::hooks
