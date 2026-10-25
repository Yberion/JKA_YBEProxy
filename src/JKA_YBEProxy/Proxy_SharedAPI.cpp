#include "JKA_YBEProxy/Proxy_Header.hpp"
#include "sdk/server/server.hpp"
#include "JKA_YBEProxy/Proxy_ClientCommand.hpp"
#include "sdk/server/sv_client.hpp"
#include "JKA_YBEProxy/Proxy_SharedAPI.hpp"
#include "JKA_YBEProxy/Imports/game/g_cmds.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_ClientCommand.hpp"

// ==================================================
// IMPORT TABLE
// ==================================================

void Proxy_SharedAPI_LocateGameData(sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient)
{
	proxy.locatedGameData.g_entities = gEnts;
	proxy.locatedGameData.num_entities = numGEntities;
	proxy.locatedGameData.g_entitySize = sizeofGEntity_t;

	proxy.locatedGameData.g_clients = clients;
	proxy.locatedGameData.g_clientSize = sizeofGameClient;
}

void Proxy_SharedAPI_GetUsercmd(int /*clientNum*/, usercmd_t* ucmd)
{
	if (ucmd->forcesel == FP_LEVITATION || ucmd->forcesel >= NUM_FORCE_POWERS)
	{
		ucmd->forcesel = 0xFFu;
	}

	ucmd->angles[ROLL] = 0;
}

// ==================================================
// EXPORT TABLE
// ==================================================

const char* Proxy_SharedAPI_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot)
{
	// The game reads the userinfo here, before SV_UserinfoChanged and GAME_CLIENT_USERINFO_CHANGED
	// Bots are skipped: their userinfo comes from the bot files
	if (!isBot)
	{
		const char* reason = Proxy_SharedAPI_SanitizeUserinfo(clientNum);

		// Returning the reason refuses the client, never DropClient here (the slot is still CS_FREE)
		if (reason)
		{
			return reason;
		}
	}

	// Sent in ClientBegin: OpenJK drops server commands to a client below CS_PRIMED, and the slot is still CS_FREE here
	proxy.clientData[clientNum].sendWelcomeMessage = firstTime && !isBot;

	return nullptr;
}

void Proxy_SharedAPI_ClientDisconnect(int clientNum)
{
	// No isConnected test: it is only set in ClientBegin, so a client that leaves before entering the game
	// would leave its data (cleanName, sendWelcomeMessage) to the next client of the slot
	if (clientNum >= 0 && clientNum < MAX_CLIENTS) {
		// Reset client's data on disconnect
		std::memset(&proxy.clientData[clientNum], 0, sizeof(*proxy.clientData));
	}
}

void Proxy_SharedAPI_ClientBegin(int clientNum, qboolean /*allowTeamReset*/)
{
	if (clientNum >= 0 && clientNum < MAX_CLIENTS)
	{
		ClientData_t* currentClientData = &proxy.clientData[clientNum];

		currentClientData->isConnected = true;

		if (currentClientData->sendWelcomeMessage)
		{
			currentClientData->sendWelcomeMessage = false;

			proxy.trap->SendServerCommand(clientNum, va("print \"^5%s (^7%s^5) %s^7\n\"", YBEPROXY_NAME, YBEPROXY_VERSION, YBEPROXY_BY_AUTHOR));
		}
	}
}

qboolean Proxy_SharedAPI_ClientCommand(int clientNum)
{
	ClientData_t *currentClientData = &proxy.clientData[clientNum];

	if (!currentClientData->isConnected)
	{
		return qfalse;
	}

	char cmd[MAX_TOKEN_CHARS] = { 0 };
	qboolean sayCmd = qfalse;

	proxy.trap->Argv(0, cmd, sizeof(cmd));

	// The commands and arguments below are compared whole (Q_stricmp), as the game does: a prefix also matched
	// other commands ("team" matched "teamvote") and other arguments ("saber_droid" only compared "saber_")
	// jkaDST_ stays a prefix: it catches every command of the cheat
	if (!Q_stricmpn(cmd, "jkaDST_", 7))
	{
		proxy.trap->SendServerCommand(-1, va("chat \"^3(Anti-Cheat system) ^7%s^3 got kicked cause of cheating^7\"", currentClientData->cleanName));
		proxy.trap->DropClient(clientNum, "(Anti-Cheat system) you got kicked cause of cheating");

		return qfalse;
	}

	// Todo (not sure): Check in the entier command + args?
	const char* argsConcat = ConcatArgs(1);

	if (!Q_stricmp(cmd, "say") || !Q_stricmp(cmd, "say_team") || !Q_stricmp(cmd, "tell"))
	{
		sayCmd = qtrue;

		// 256 because we don't need more, the chat can handle 150 max char
		// and allowing 256 prevent a message to not be sent instead of being truncated
		// if this is a bit more than 150
		if (std::strlen(argsConcat) > 256)
		{
			return qfalse;
		}
	}

	char cmd_arg1[MAX_TOKEN_CHARS] = { 0 };
	char cmd_arg2[MAX_TOKEN_CHARS] = { 0 };

	proxy.trap->Argv(1, cmd_arg1, sizeof(cmd_arg1));
	proxy.trap->Argv(2, cmd_arg2, sizeof(cmd_arg2));

	// Fix: crach gc
	if (!Q_stricmp(cmd, "gc") && atoi(cmd_arg1) >= proxy.trap->Cvar_VariableIntegerValue("sv_maxclients"))
	{
		return qfalse;
	}

	// Fix: crash npc spawn
	if (!Q_stricmp(cmd, "npc") && !Q_stricmp(cmd_arg1, "spawn") && (!Q_stricmp(cmd_arg2, "ragnos") || !Q_stricmp(cmd_arg2, "saber_droid")))
	{
		return qfalse;
	}

	// Fix: team crash
	if (!Q_stricmp(cmd, "team") && (!Q_stricmp(cmd_arg1, "follow1") || !Q_stricmp(cmd_arg1, "follow2")))
	{
		return qfalse;
	}

	// Disable: callteamvote, useless in basejka and can lead to a bugged UI on custom client
	if (!Q_stricmp(cmd, "callteamvote"))
	{
		return qfalse;
	}

	if (proxy.cvars.proxy_sv_disableKillCmd.integer && !Q_stricmp(cmd, "kill"))
	{
		return qfalse;
	}

	if (!Q_stricmp(cmd, "callvote"))
	{
		const int cmdArg2NumberValue = atoi(cmd_arg2);

		int minArg2Value = 0;
		int maxArg2Value = 1;
		qboolean checkNeeded = qfalse;

		// Fix: callvote long string length crash
		if (std::strlen(cmd_arg2) >= MAX_CVAR_VALUE_STRING)
		{
			return qfalse;
		}

		// Fix: callvote capturelimit with wrong values
		if (!Q_stricmp(cmd_arg1, "capturelimit"))
		{
			minArg2Value = 0;
			maxArg2Value = 0x7FFFFFFF;
			checkNeeded = qtrue;
		}

		// Fix: callvote fraglimit with wrong values
		if (!Q_stricmp(cmd_arg1, "fraglimit"))
		{
			minArg2Value = 0;
			maxArg2Value = 0x7FFFFFFF;
			checkNeeded = qtrue;
		}

		// Fix: callvote g_doWarmup with wrong values
		if (!Q_stricmp(cmd_arg1, "g_doWarmup"))
		{
			minArg2Value = 0;
			maxArg2Value = 1;
			checkNeeded = qtrue;
		}

		// Fix: callvote map_restart with wrong values
		if (!Q_stricmp(cmd_arg1, "map_restart"))
		{
			minArg2Value = 0;
			maxArg2Value = proxy.cvars.proxy_sv_maxCallVoteMapRestartValue.integer;
			checkNeeded = qtrue;
		}

		// Fix: callvote timelimit with wrong values
		if (!Q_stricmp(cmd_arg1, "timelimit"))
		{
			minArg2Value = 0;
			maxArg2Value = 35790;
			checkNeeded = qtrue;
		}

		if (checkNeeded && (cmdArg2NumberValue < minArg2Value || cmdArg2NumberValue > maxArg2Value))
		{
			return qfalse;
		}
	}

	if (Q_strchrs(argsConcat, "\r\n"))
	{
		return qfalse;
	}

	if (!sayCmd && Q_strchrs(argsConcat, ";"))
	{
		return qfalse;
	}

	if (!Q_stricmp(cmd, "myratio"))
	{
		Proxy_ClientCommand_MyRatio(clientNum);

		return qfalse;
	}

	// Only work on default engine since it require memory hook
	if (proxy.isOriginalEngine)
	{
		if (proxy.originalEngineCvars.proxy_sv_enableNetStatus.integer && (!Q_stricmp(cmd, "netStatus") || !Q_stricmp(cmd, "showNet")))
		{
			Proxy_Engine_ClientCommand_NetStatus(clientNum);

			return qfalse;
		}
	}

	return qtrue;
}

/*
// If we want to do something with the usercmd then call manually GetUsercmd
void Proxy_SharedAPI_ClientThink(int clientNum, usercmd_t* ucmd)
{}
*/

qboolean Proxy_SharedAPI_ClientUserinfoChanged(int clientNum)
{
	const char* reason = Proxy_SharedAPI_SanitizeUserinfo(clientNum);

	if (reason)
	{
		proxy.trap->SendServerCommand(-1, va("chat \"^3(Anti-Cheat system) ^7%s^3 got kicked cause of cheating^7\"", proxy.clientData[clientNum].cleanName));
		proxy.trap->DropClient(clientNum, reason);

		return qfalse;
	}

	return qtrue;
}

const char* Proxy_SharedAPI_SanitizeUserinfo(int clientNum)
{
	char userinfo[MAX_INFO_STRING];

	proxy.trap->GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	if (std::strlen(userinfo) == 0)
	{
		return nullptr;
	}

	std::size_t len = 0;
	const char* val = NULL;

	val = Info_ValueForKey(userinfo, "name");

	ClientData_t *currentClientData = &proxy.clientData[clientNum];

	// Fix bad names
	Proxy_ClientCleanName(val, currentClientData->cleanName, sizeof(currentClientData->cleanName));
	Info_SetValueForKey(userinfo, "name", currentClientData->cleanName);

	val = Info_ValueForKey(userinfo, "model");

	// Fix bugged models
	// Fix model length crash on some custom clients
	// There's also a check done in SV_UserinfoChanged
	if (val)
	{
		len = std::strlen(val);

		if (!Q_stricmpn(val, "darksidetools", 13))
		{
			return "(Anti-Cheat system) you got kicked cause of cheating";
		}

		if (!val[0] ||
			!Q_stricmp(val, "jedi_") || !Q_stricmpn(val, "jedi_/", 6) ||
			!Q_stricmpn(val, "rancor", 6) ||
			!Q_stricmpn(val, "wampa", 5) ||
			!Q_IsValidAsciiStr(val) ||
			len >= /*MAX_QPATH*/ (std::size_t)proxy.cvars.proxy_sv_modelPathLength.integer)
		{
			Info_SetValueForKey(userinfo, "model", "kyle");
		}
	}

	//Fix forcepowers crash
	char forcePowers[30];
	
	Q_strncpyz(forcePowers, Info_ValueForKey(userinfo, "forcepowers"), sizeof(forcePowers));

	len = std::strlen(forcePowers);

	qboolean badForce = qfalse;

	if (len >= 22 && len <= 24)
	{
		byte seps = 0;

		for (std::size_t i = 0; i < len; i++)
		{
			if (forcePowers[i] != '-' && (forcePowers[i] < '0' || forcePowers[i] > '9'))
			{
				badForce = qtrue;
				break;
			}

			if ((i < 1 || i > 5) && forcePowers[i] == '-')
			{
				badForce = qtrue;
				break;
			}

			if (i && forcePowers[i - 1] == '-' && forcePowers[i] == '-')
			{
				badForce = qtrue;
				break;
			}

			if (forcePowers[i] == '-')
			{
				seps++;
			}
		}

		if (seps != 2)
		{
			badForce = qtrue;
		}
	}
	else
	{
		badForce = qtrue;
	}

	if (badForce)
	{
		Q_strncpyz(forcePowers, "7-1-030000000000003332", sizeof(forcePowers));
	}

	Info_SetValueForKey(userinfo, "forcepowers", forcePowers);

	proxy.trap->SetUserinfo(clientNum, userinfo);

	return nullptr;
}