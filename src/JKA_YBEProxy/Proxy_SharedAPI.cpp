#include "JKA_YBEProxy/Proxy_Header.hpp"
#include "sdk/server/server.hpp"
#include "JKA_YBEProxy/Proxy_ClientCommand.hpp"
#include "sdk/server/sv_client.hpp"
#include "JKA_YBEProxy/Proxy_SharedAPI.hpp"
#include "JKA_YBEProxy/Proxy_Bans.hpp"
#include "JKA_YBEProxy/Proxy_CVars.hpp"
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

	// A signed char reaches -128 but only +127: a negative move would be one unit longer than the longest positive one
	// (ioq3 bug 5273, OpenJK, Quake3e and TaystJK do it when they read the usercmd)
	if (ucmd->forwardmove == -128)
	{
		ucmd->forwardmove = -127;
	}

	if (ucmd->rightmove == -128)
	{
		ucmd->rightmove = -127;
	}

	if (ucmd->upmove == -128)
	{
		ucmd->upmove = -127;
	}
}

// ==================================================
// EXPORT TABLE
// ==================================================

// The userinfo of a bot comes from the bot files, and has no forcepowers key
static bool Proxy_IsBot(int clientNum)
{
	if (!proxy.locatedGameData.g_entities)
	{
		return false;
	}

	const sharedEntity_t* ent = (const sharedEntity_t*)((const unsigned char*)proxy.locatedGameData.g_entities + (std::size_t)proxy.locatedGameData.g_entitySize * (std::size_t)clientNum);

	return (ent->r.svFlags & SVF_BOT) != 0;
}

// "1.2.3.4:29070" -> "1.2.3.4"
static void Proxy_GetBaseAddress(const char* ip, char* out, std::size_t outSize)
{
	const char* port = std::strrchr(ip, ':');
	const std::size_t len = port ? (std::size_t)(port - ip) : std::strlen(ip);

	Q_strncpyz(out, ip, (outSize < len + 1) ? outSize : len + 1);
}

const char* Proxy_SharedAPI_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot)
{
	// The game reads the userinfo here, before SV_UserinfoChanged and GAME_CLIENT_USERINFO_CHANGED
	// Bots are skipped: their userinfo comes from the bot files
	if (!isBot)
	{
		// SV_DirectConnect adds the "ip" key that the game's ban check reads. When the userinfo was too long to receive it,
		// the key is missing, and a banned address would pass
		char userinfo[MAX_INFO_STRING];

		proxy.trap->GetUserinfo(clientNum, userinfo, sizeof(userinfo));

		if (!Info_ValueForKey(userinfo, "ip")[0])
		{
			// Same message as ioq3, JK2MV and OpenJK give when the connection is refused for that reason
			return "Userinfo string length exceeded.  Try removing setu cvars from your config.";
		}

		// The ban list is the proxy's, the game's own is empty (see Proxy_Bans.cpp). Same reason as the game's
		if (Proxy_Bans_IsBanned(Info_ValueForKey(userinfo, "ip")))
		{
			return "Banned.";
		}

		// Limit of clients per IP address, from Quake3e's sv_maxclientsPerIP (SV_DirectConnect): a new connection is refused
		// when the other clients of the address already reach the limit
		// Several players can share one address (LAN, carrier NAT): the limit is a setting. Only for a new connection: at
		// each map change the engine connects again the clients that are already in
		if (firstTime && proxy.cvars.proxy_sv_maxClientsPerIP.integer > 0)
		{
			char address[sizeof(proxy.clientData[0].address)];
			int count = 0;

			Proxy_GetBaseAddress(Info_ValueForKey(userinfo, "ip"), address, sizeof(address));

			for (int i = 0; i < MAX_CLIENTS; ++i)
			{
				// The address of a slot is empty for a bot, and until the game accepted its client
				if (i != clientNum && !std::strcmp(proxy.clientData[i].address, address) && ++count >= proxy.cvars.proxy_sv_maxClientsPerIP.integer)
				{
					return "Too many connections from your IP address.";
				}
			}
		}

		const char* reason = Proxy_SharedAPI_SanitizeUserinfo(clientNum, true, nullptr);

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

void Proxy_SharedAPI_ClientConnected(int clientNum, qboolean isBot)
{
	if (isBot)
	{
		return;
	}

	char userinfo[MAX_INFO_STRING];

	proxy.trap->GetUserinfo(clientNum, userinfo, sizeof(userinfo));
	Proxy_GetBaseAddress(Info_ValueForKey(userinfo, "ip"), proxy.clientData[clientNum].address, sizeof(proxy.clientData[clientNum].address));
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

			if (!proxy.cvars.proxy_sv_disableWelcomeMessage.integer)
			{
				proxy.trap->SendServerCommand(clientNum, va("print \"^5%s (^7%s^5) %s^7\n\"", YBEPROXY_NAME, YBEPROXY_VERSION, YBEPROXY_BY_AUTHOR));
			}
		}
	}
}

// "forceteam -1 <team>": the game only takes a client number or a name, "-1" is looked up as a name and fails
// Every client in the game gets the game's own "forceteam <number> <team>", so SetTeam runs with its usual checks
// The commands are appended, not executed now: the game must not run console commands from inside one
static void Proxy_ForceTeamEveryone(const char* team)
{
	// The team goes into command text: it must not be able to end the command and start another
	if (!team[0] || std::strpbrk(team, ";\"\n\r"))
	{
		proxy.trap->Print("Usage: forceteam -1 <team>\n");

		return;
	}

	const int maxClients = proxy.trap->Cvar_VariableIntegerValue("sv_maxclients");

	for (int i = 0; i < MAX_CLIENTS && i < maxClients; ++i)
	{
		if (proxy.clientData[i].isConnected)
		{
			proxy.trap->SendConsoleCommand(EXEC_APPEND, va("forceteam %i \"%s\"\n", i, team));
		}
	}
}

int Proxy_SharedAPI_ConsoleClientNum(const char* cmd)
{
	char arg[MAX_TOKEN_CHARS];

	proxy.trap->Argv(1, arg, sizeof(arg));

	// Digits only: atoi would read "1abc" as 1, and the empty argument as 0
	bool isNumber = arg[0] != '\0';

	for (const char* c = arg; *c; ++c)
	{
		isNumber = isNumber && *c >= '0' && *c <= '9';
	}

	const int clientNum = isNumber ? atoi(arg) : -1;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum >= proxy.trap->Cvar_VariableIntegerValue("sv_maxclients") || !proxy.clientData[clientNum].isConnected)
	{
		proxy.trap->Print("Usage: %s <client number>, of a player in the game (see status)\n", cmd);

		return -1;
	}

	return clientNum;
}

// "mute <client number>" and "unmute <client number>", from the server console or rcon
// The game has no such command, so GAME_CONSOLE_COMMAND reaches the proxy for it
qboolean Proxy_SharedAPI_ConsoleCommand(void)
{
	char cmd[MAX_TOKEN_CHARS];

	proxy.trap->Argv(0, cmd, sizeof(cmd));

	if (Proxy_Bans_ConsoleCommand(cmd))
	{
		return qtrue;
	}

	if (!Q_stricmp(cmd, "proxyconfig"))
	{
		Proxy_CVars_PrintConfig();

		return qtrue;
	}

	if (!Q_stricmp(cmd, "forceteam"))
	{
		char arg1[MAX_TOKEN_CHARS];
		char arg2[MAX_TOKEN_CHARS];

		proxy.trap->Argv(1, arg1, sizeof(arg1));
		proxy.trap->Argv(2, arg2, sizeof(arg2));

		if (std::strcmp(arg1, "-1"))
		{
			return qfalse;
		}

		Proxy_ForceTeamEveryone(arg2);

		return qtrue;
	}

	const bool mute = !Q_stricmp(cmd, "mute");

	if (!mute && Q_stricmp(cmd, "unmute"))
	{
		return qfalse;
	}

	const int clientNum = Proxy_SharedAPI_ConsoleClientNum(cmd);

	if (clientNum < 0)
	{
		return qtrue;
	}

	ClientData_t* clientData = &proxy.clientData[clientNum];

	clientData->muted = mute;

	proxy.trap->Print("%s ^7%s\n", clientData->cleanName, mute ? "is muted" : "is no longer muted");
	proxy.trap->SendServerCommand(clientNum, mute ? "print \"You have been muted.\n\"" : "print \"You are no longer muted.\n\"");

	return qtrue;
}

// Below the team gametypes the game turns say_team into say, so a spectator could not talk to the other spectators
// only. While the game runs the say_team of a spectator there, its chat only goes to the spectators, as a team chat.
// The say_team of a player stays public
static bool spectatorChat = false;

// The team of the client in its configstring (the game's sessionTeam). The playerState of a spectator who follows a
// player is a copy of that player's, team included
static int Proxy_ClientTeam(int clientNum)
{
	char info[MAX_INFO_STRING];

	proxy.trap->GetConfigstring(CS_PLAYERS + clientNum, info, sizeof(info));

	return atoi(Info_ValueForKey(info, "t"));
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

	// The muted client gets told, so it does not think the server lost its message
	if (currentClientData->muted && (!Q_stricmp(cmd, "say") || !Q_stricmp(cmd, "say_team") || !Q_stricmp(cmd, "tell") || !Q_stricmp(cmd, "voice_cmd")))
	{
		proxy.trap->SendServerCommand(clientNum, "print \"You are muted.\n\"");

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

		if (proxy.originalEngineCvars.proxy_sv_enableNetStatus.integer && !Q_stricmp(cmd, "explainNetStatus"))
		{
			Proxy_Engine_ClientCommand_ExplainNetStatus(clientNum);

			return qfalse;
		}
	}

	if (!Q_stricmp(cmd, "say_team") && proxy.cvars.sv_gametype.integer < GT_TEAM && Proxy_ClientTeam(clientNum) == TEAM_SPECTATOR)
	{
		spectatorChat = true;
	}

	return qtrue;
}

void Proxy_SharedAPI_ClientCommandDone(void)
{
	spectatorChat = false;
}

const char* Proxy_SharedAPI_TeamChat(int clientNum, const char* text)
{
	// A broadcast cannot be split by team
	if (!spectatorChat || clientNum < 0 || std::strncmp(text, "chat \"", 6))
	{
		return text;
	}

	if (Proxy_ClientTeam(clientNum) != TEAM_SPECTATOR)
	{
		return nullptr;
	}

	// The game's say: "chat \"<name>^7" EC ": ^2<message>\"". A name cannot hold EC, so the first one ends it. Another
	// format (a mod) only gets the filter above
	const char* const name = text + 6;
	const char* const separator = std::strchr(name, EC[0]);

	if (!separator || separator - name < 2 || std::strncmp(separator - 2, "^7" EC ": ^2", 7))
	{
		return text;
	}

	// The game's say_team of the team gametypes: "tchat \"" EC "(<name>^7" EC ")" EC ": ^5<message>\""
	static char teamText[MAX_STRING_CHARS];

	Com_sprintf(teamText, sizeof(teamText), "tchat \"" EC "(%.*s^7" EC ")" EC ": " S_COLOR_CYAN "%s", (int)(separator - 2 - name), name,
		separator + 5);

	return teamText;
}

/*
// If we want to do something with the usercmd then call manually GetUsercmd
void Proxy_SharedAPI_ClientThink(int clientNum, usercmd_t* ucmd)
{}
*/

qboolean Proxy_SharedAPI_ClientUserinfoChanged(int clientNum)
{
	bool isAntiCheat = false;
	const char* reason = Proxy_SharedAPI_SanitizeUserinfo(clientNum, !Proxy_IsBot(clientNum), &isAntiCheat);

	if (reason)
	{
		if (isAntiCheat)
		{
			proxy.trap->SendServerCommand(-1, va("chat \"^3(Anti-Cheat system) ^7%s^3 got kicked cause of cheating^7\"", proxy.clientData[clientNum].cleanName));
		}

		proxy.trap->DropClient(clientNum, reason);

		return qfalse;
	}

	return qtrue;
}

// ==================================================
// Userinfo validation
// --------------------------------------------------
// Port of OpenJK's G_ValidateUserinfo (codemp/game/g_client.c), with its default settings (g_userinfoValidate): size,
// slashes, control characters, and the number of each known key counted without case. Extended ASCII is not checked.
// The info functions of the game read a key without case, but remove only a key with the same case: a userinfo with a
// key twice, or with another case, would let a bad value through the checks below
// The retail client sends every key that is required here, with its default settings
// ==================================================

struct UserinfoField_t
{
	const char*		name;
	unsigned int	minCount;
	unsigned int	maxCount;
};

static const UserinfoField_t userinfoFields[] =
{
	{ "cl_guid",		0, 0 }, // not allowed, q3fill protection
	{ "cl_punkbuster",	0, 0 }, // not allowed, q3fill protection
	{ "ip",				0, 1 }, // the engine adds this
	{ "name",			1, 1 },
	{ "rate",			1, 1 },
	{ "snaps",			1, 1 },
	{ "model",			1, 1 },
	{ "forcepowers",	1, 1 },
	{ "color1",			1, 1 },
	{ "color2",			1, 1 },
	{ "handicap",		1, 1 },
	{ "sex",			0, 1 },
	{ "cg_predictItems",1, 1 },
	{ "saber1",			1, 1 },
	{ "saber2",			1, 1 },
	{ "char_color_red",	1, 1 },
	{ "char_color_green",1, 1 },
	{ "char_color_blue",1, 1 },
	{ "teamtask",		0, 1 }, // optional
	{ "password",		0, 1 }, // optional
	{ "teamoverlay",	0, 1 }, // only registered in cgame, not sent when connecting
};

// Returns nullptr, or why the userinfo is not valid. The text of the reason is only valid until the next call
static const char* Proxy_ValidateUserinfo(const char* userinfo)
{
	static char reason[64];
	const std::size_t length = std::strlen(userinfo);

	if (length < 1)
	{
		return "Userinfo too short";
	}

	if (length >= MAX_INFO_STRING)
	{
		return "Userinfo too long";
	}

	// there must be a leading slash
	if (userinfo[0] != '\\')
	{
		return "Missing leading slash";
	}

	// no trailing slash allowed, the engine adds ip\ip:port in front
	if (userinfo[length - 1] == '\\')
	{
		return "Trailing slash";
	}

	// the format is \key\value, so there must be an even number of slashes
	std::size_t slashes = 0;

	for (std::size_t i = 0; i < length; ++i)
	{
		if (userinfo[i] == '\\')
		{
			++slashes;
		}
	}

	if (slashes & 1)
	{
		return "Bad number of slashes";
	}

	// disallow \n \r ; and \"
	if (Q_strchrs(userinfo, "\n\r;\""))
	{
		return "Invalid characters found";
	}

	unsigned int fieldCount[ARRAY_LEN(userinfoFields)] = {};
	char key[MAX_INFO_KEY];
	char value[MAX_INFO_VALUE];
	const char* s = userinfo;

	while (s)
	{
		Info_NextPair(&s, key, value);

		if (!key[0])
		{
			break;
		}

		for (std::size_t i = 0; i < ARRAY_LEN(userinfoFields); ++i)
		{
			if (!Q_stricmp(key, userinfoFields[i].name))
			{
				++fieldCount[i];
			}
		}
	}

	for (std::size_t i = 0; i < ARRAY_LEN(userinfoFields); ++i)
	{
		if (userinfoFields[i].minCount && !fieldCount[i])
		{
			Com_sprintf(reason, sizeof(reason), "%s field not found", userinfoFields[i].name);

			return reason;
		}

		if (fieldCount[i] > userinfoFields[i].maxCount)
		{
			Com_sprintf(reason, sizeof(reason), "Too many %s fields (%u/%u)", userinfoFields[i].name, fieldCount[i], userinfoFields[i].maxCount);

			return reason;
		}
	}

	return nullptr;
}

const char* Proxy_SharedAPI_SanitizeUserinfo(int clientNum, bool validate, bool* isAntiCheat)
{
	static char reason[96];
	char userinfo[MAX_INFO_STRING];

	proxy.trap->GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	if (isAntiCheat)
	{
		*isAntiCheat = false;
	}

	if (validate)
	{
		const char* invalid = Proxy_ValidateUserinfo(userinfo);

		if (invalid)
		{
			Com_sprintf(reason, sizeof(reason), "Failed userinfo validation: %s", invalid);

			return reason;
		}
	}

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

	// Info_SetValueForKey adds nothing when the userinfo has no room left: the old value would stay, and the game would read it
	if (!Info_SetValueForKey(userinfo, "name", currentClientData->cleanName))
	{
		return "Userinfo string length exceeded.";
	}

	val = Info_ValueForKey(userinfo, "model");

	// Fix bugged models
	// Fix model length crash on some custom clients
	// There's also a check done in SV_UserinfoChanged
	if (val)
	{
		len = std::strlen(val);

		if (!Q_stricmpn(val, "darksidetools", 13))
		{
			if (isAntiCheat)
			{
				*isAntiCheat = true;
			}

			return "(Anti-Cheat system) you got kicked cause of cheating";
		}

		if (!val[0] ||
			!Q_stricmp(val, "jedi_") || !Q_stricmpn(val, "jedi_/", 6) ||
			!Q_stricmpn(val, "rancor", 6) ||
			!Q_stricmpn(val, "wampa", 5) ||
			!Q_IsValidAsciiStr(val) ||
			len >= /*MAX_QPATH*/ (std::size_t)proxy.cvars.proxy_sv_modelPathLength.integer)
		{
			if (!Info_SetValueForKey(userinfo, "model", "kyle"))
			{
				return "Userinfo string length exceeded.";
			}
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

	if (!Info_SetValueForKey(userinfo, "forcepowers", forcePowers))
	{
		return "Userinfo string length exceeded.";
	}

	proxy.trap->SetUserinfo(clientNum, userinfo);

	return nullptr;
}