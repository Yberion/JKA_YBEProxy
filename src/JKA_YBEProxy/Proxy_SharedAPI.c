#include "Proxy_Header.h"

// ==================================================
// IMPORT TABLE
// ==================================================

void Proxy_Shared_LocateGameData(sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient)
{
	proxy.locatedGameData.g_entities = gEnts;
	proxy.locatedGameData.num_entities = numGEntities;
	proxy.locatedGameData.g_entitySize = sizeofGEntity_t;

	proxy.locatedGameData.g_clients = clients;
	proxy.locatedGameData.g_clientSize = sizeofGameClient;
}

void Proxy_Shared_GetUsercmd(int clientNum, usercmd_t* cmd)
{
	cmd->forcesel = 0xFFu;
	cmd->angles[ROLL] = 0;
}

// ==================================================
// EXPORT TABLE
// ==================================================

void Proxy_Shared_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot)
{
	// Doesn't work on the new API
	if (firstTime && !isBot)
	{
		proxy.trap->SendServerCommand(clientNum, va("print \"^5%s (^7%s^5) %s^7\n\"", YBEPROXY_NAME, YBEPROXY_VERSION, YBEPROXY_BY_AUTHOR));
	}
}

void Proxy_Shared_ClientBegin(int clientNum, qboolean allowTeamReset)
{
	if (clientNum >= 0 && clientNum < MAX_CLIENTS)
	{
		proxy.proxyClientData[clientNum].isConnected = qtrue;
	}
}

qboolean Proxy_Shared_ClientCommand(int clientNum)
{
	if (!proxy.proxyClientData[clientNum].isConnected)
	{
		return qfalse;
	}

	char cmd[MAX_TOKEN_CHARS] = { 0 };
	qboolean sayCmd = qfalse;

	proxy.trap->Argv(0, cmd, sizeof(cmd));

	if (!Q_stricmpn(&cmd[0], "jkaDST_", 7))
	{
		proxy.trap->DropClient(clientNum, "(Anti-Cheat system) you got kicked cause of cheating");
		proxy.trap->SendServerCommand(-1, "chat \"^3(Anti-Cheat system) ^7X^3 got kicked cause of cheating^7\"");

		return qfalse;
	}

	char* argsConcat = ConcatArgs(1);

	if (!Q_stricmpn(&cmd[0], "say", 3) || !Q_stricmpn(&cmd[0], "say_team", 8) || !Q_stricmpn(&cmd[0], "tell", 4))
	{
		sayCmd = qtrue;

		// 256 because we don't need more, the chat can handle 150 max char
		// and allowing 256 prevent a message to not be sent instead of being truncated
		// if this is a bit more than 150
		if (strlen(argsConcat) > 256)
		{
			return qfalse;
		}
	}

	char cmd_arg1[MAX_TOKEN_CHARS] = { 0 };
	char cmd_arg2[MAX_TOKEN_CHARS] = { 0 };

	proxy.trap->Argv(1, cmd_arg1, sizeof(cmd_arg1));
	proxy.trap->Argv(2, cmd_arg2, sizeof(cmd_arg2));

	// Fix: crach gc
	if (!Q_stricmpn(&cmd[0], "gc", 2) && atoi(cmd_arg1) >= proxy.trap->Cvar_VariableIntegerValue("sv_maxclients"))
	{
		return qfalse;
	}

	// Fix: crash npc spawn
	if (!Q_stricmpn(&cmd[0], "npc", 3) && !Q_stricmpn(&cmd_arg1[0], "spawn", 5) && (!Q_stricmpn(&cmd_arg2[0], "ragnos", 6) || !Q_stricmpn(&cmd_arg2[0], "saber_droid", 6)))
	{
		return qfalse;
	}

	// Fix: team crash
	if (!Q_stricmpn(&cmd[0], "team", 4) && (!Q_stricmpn(&cmd_arg1[0], "follow1", 7) || !Q_stricmpn(&cmd_arg1[0], "follow2", 7)))
	{
		return qfalse;
	}

	// Disable: callteamvote, useless in basejka and can lead to a bugged UI on custom client
	if (!Q_stricmpn(&cmd[0], "callteamvote", 12))
	{
		return qfalse;
	}

	// Fix: callvote fraglimit/timelimit with negative value
	if (!Q_stricmpn(&cmd[0], "callvote", 8) && (!Q_stricmpn(&cmd_arg1[0], "fraglimit", 9) || !Q_stricmpn(&cmd_arg1[0], "timelimit", 9)) && atoi(cmd_arg2) < 0)
	{
		return qfalse;
	}

	if (Q_strchrs(argsConcat, "\r\n"))
	{
		return qfalse;
	}

	if (!sayCmd && Q_strchrs(argsConcat, ";"))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean Proxy_Shared_ClientUserinfoChanged(int clientNum)
{
	// WIP
	// Fix bugged model 
	// Clean name

	char userinfo[MAX_INFO_STRING] = { 0 };

	proxy.trap->Argv(1, userinfo, sizeof(userinfo));

	proxy.trap->Print("AAA : %s \n", Info_ValueForKey(userinfo, "name"));

	return qtrue;
}