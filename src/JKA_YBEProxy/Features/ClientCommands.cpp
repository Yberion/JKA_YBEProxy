#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Features/ClientCommands.hpp"
#include "JKA_YBEProxy/Features/Cvars.hpp"
#include "JKA_YBEProxy/Ports/game/g_cmds.hpp"
#include "JKA_YBEProxy/Ports/server/sv_game.hpp"

namespace ybeproxy::features::client_commands
{

// Longest command line (name, space and arguments) a client may send to the game. The game logs some command lines
// whole in a buffer of 1024 bytes; no command of the game needs more than this
static constexpr std::size_t maxCommandLength = 512;

// Number of entries of gc_orders in the game (g_cmds.c)
static constexpr int gcOrdersCount = 7;

// Arguments given to the game instead of the client's, for the command it runs now, by argument number (0 unused). An
// empty one is not replaced
static char gameArgv[3][16];

// The slot of the client with this name, compared without colors and case as OpenJK does, or -1. The game only looked in
// the first numConnectedClients slots, and kept the name of a client who left: every slot with a CS_PLAYERS is looked at
// here, the game empties it when a client leaves
static int ClientNumberFromName(const char* name)
{
	char wanted[MAX_NETNAME];

	Q_strncpyz(wanted, name, sizeof(wanted));
	Q_StripColor(wanted);

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		char info[MAX_INFO_STRING];

		proxy.trap->GetConfigstring(CS_PLAYERS + i, info, sizeof(info));

		if (!info[0])
		{
			continue;
		}

		char netname[MAX_NETNAME];

		Q_strncpyz(netname, Info_ValueForKey(info, "n"), sizeof(netname));
		Q_StripColor(netname);

		if (!Q_stricmp(netname, wanted))
		{
			return i;
		}
	}

	return -1;
}

/*
==================
MyRatio
==================
*/
static void ratioString(int kill, int death, char* ratioString, int sizeRatioString)
{
	if (kill - death >= 0)
	{
		Com_sprintf(ratioString, sizeRatioString, "(" S_COLOR_GREEN "+%i" S_COLOR_WHITE ")", kill - death);
	}
	else
	{
		Com_sprintf(ratioString, sizeRatioString, "(" S_COLOR_RED "%i" S_COLOR_WHITE ")", kill - death);
	}
}

static float calcRatio(int kill, int death)
{
	if (kill == 0 && death == 0)
	{
		return 1.00;
	}
	else if (kill < 1 && death >= 1)
	{
		return 0.00;
	}
	else if (kill >= 1 && death <= 1)
	{
		return (float)kill;
	}
	else
	{
		return (float)kill / (float)death;
	}
}

void MyRatio(int clientNum)
{
	ClientData *currentClientData = &proxy.clientData[clientNum];

	if (currentClientData->lastTimeMyratioCheck + 1000 > proxy.levelTime)
	{
		return;
	}

	currentClientData->lastTimeMyratioCheck = proxy.levelTime;

	char ratioStringBuffer[16];
	playerState_t* ps;

	ps = ports::GetPlayerState(clientNum);

	float ratio = calcRatio(ps->persistant[PERS_SCORE], ps->persistant[PERS_KILLED]);

	ratioString(ps->persistant[PERS_SCORE], ps->persistant[PERS_KILLED], ratioStringBuffer, sizeof(ratioStringBuffer));

	if (proxy.cvars.sv_gametype.integer != GT_DUEL && proxy.cvars.sv_gametype.integer != GT_TEAM && proxy.cvars.sv_gametype.integer != GT_FFA)
	{
		proxy.trap->SendServerCommand(clientNum, "print \"Command not supported for this gametype\n\"");

		return;
	}

	proxy.trap->SendServerCommand(clientNum, va("print \"Kills" S_COLOR_BLUE ": " S_COLOR_WHITE "%i " S_COLOR_BLUE "| "
		S_COLOR_WHITE "Deaths" S_COLOR_BLUE ": " S_COLOR_WHITE "%i " S_COLOR_BLUE "(" S_COLOR_WHITE "Suicides" S_COLOR_BLUE ": " S_COLOR_WHITE "%i" S_COLOR_BLUE ") " S_COLOR_BLUE "| "
		S_COLOR_WHITE "Ratio" S_COLOR_BLUE ": " S_COLOR_WHITE "%.2f %s\n\"",
		ps->persistant[PERS_SCORE], ps->persistant[PERS_KILLED],
		ps->fd.suicides,
		(double)ratio, ratioStringBuffer));
}

bool Allowed(int clientNum, const char* cmd)
{
	ClientData *currentClientData = &proxy.clientData[clientNum];
	qboolean sayCmd = qfalse;

	// The commands and arguments below are compared whole (Q_stricmp), as the game does: a prefix also matched
	// other commands ("team" matched "teamvote") and other arguments ("saber_droid" only compared "saber_")
	// jkaDST_ stays a prefix: it catches every command of the cheat
	if (!Q_stricmpn(cmd, "jkaDST_", 7))
	{
		proxy.trap->SendServerCommand(-1, va("chat \"^3(Anti-Cheat system) ^7%s^3 got kicked cause of cheating^7\"", currentClientData->cleanName));
		proxy.trap->DropClient(clientNum, "(Anti-Cheat system) you got kicked cause of cheating");

		return false;
	}

	// The muted client gets told, so it does not think the server lost its message
	if (currentClientData->muted && (!Q_stricmp(cmd, "say") || !Q_stricmp(cmd, "say_team") || !Q_stricmp(cmd, "tell") || !Q_stricmp(cmd, "voice_cmd")))
	{
		proxy.trap->SendServerCommand(clientNum, "print \"You are muted.\n\"");

		return false;
	}

	// Todo (not sure): Check in the entier command + args?
	const char* argsConcat = ports::ConcatArgs(1);

	// Fix: stack overflow in the game's G_LogPrintf (vsprintf in a buffer of 1024 bytes)
	// At intermission the game turns every command it does not know into a public chat message of the whole command
	// line, and logs it before cutting it: the limit on "say", "say_team" and "tell" below does not cover that
	if (std::strlen(cmd) + 1 + std::strlen(argsConcat) > maxCommandLength)
	{
		return false;
	}

	if (!Q_stricmp(cmd, "say") || !Q_stricmp(cmd, "say_team") || !Q_stricmp(cmd, "tell"))
	{
		sayCmd = qtrue;

		// 256 because we don't need more, the chat can handle 150 max char
		// and allowing 256 prevent a message to not be sent instead of being truncated
		// if this is a bit more than 150
		if (std::strlen(argsConcat) > 256)
		{
			return false;
		}
	}

	char cmd_arg1[MAX_TOKEN_CHARS] = { 0 };
	char cmd_arg2[MAX_TOKEN_CHARS] = { 0 };

	proxy.trap->Argv(1, cmd_arg1, sizeof(cmd_arg1));
	proxy.trap->Argv(2, cmd_arg2, sizeof(cmd_arg2));

	// Fix: crach gc
	// The order is an index of the game's gc_orders: the game also accepts gcOrdersCount, one past the end, and sends
	// what it reads there as a text (crash). It already refuses a negative player or order
	if (!Q_stricmp(cmd, "gc") &&
		(atoi(cmd_arg1) >= proxy.trap->Cvar_VariableIntegerValue("sv_maxclients") || atoi(cmd_arg2) >= gcOrdersCount))
	{
		return false;
	}

	// Fix: crash npc spawn
	if (!Q_stricmp(cmd, "npc") && !Q_stricmp(cmd_arg1, "spawn") && (!Q_stricmp(cmd_arg2, "ragnos") || !Q_stricmp(cmd_arg2, "saber_droid")))
	{
		return false;
	}

	// Fix: team crash
	if (!Q_stricmp(cmd, "team") && (!Q_stricmp(cmd_arg1, "follow1") || !Q_stricmp(cmd_arg1, "follow2")))
	{
		return false;
	}

	// Disable: callteamvote, useless in basejka and can lead to a bugged UI on custom client
	if (!Q_stricmp(cmd, "callteamvote"))
	{
		return false;
	}

	if (proxy.cvars.proxy_sv_disableKillCmd.integer && !Q_stricmp(cmd, "kill"))
	{
		return false;
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
			return false;
		}

		// Fix: callvote kick <name>, the proxy looks the name up (see ClientNumberFromName), and the game gets the vote as
		// clientkick <slot> (ReplaceArguments). Same message as the game when nobody has this name
		if (!Q_stricmp(cmd_arg1, "kick") && ClientNumberFromName(cmd_arg2) < 0)
		{
			proxy.trap->SendServerCommand(clientNum, va("print \"there is no client named '%s' currently on the server.\n\"", cmd_arg2));

			return false;
		}

		// Fix: callvote capturelimit with wrong values
		if (!Q_stricmp(cmd_arg1, "capturelimit"))
		{
			minArg2Value = 0;
			maxArg2Value = 0x7FFFFFFF;
			checkNeeded = qtrue;
		}

		// Fix: callvote clientkick crash
		// The game only compares the number with MAX_CLIENTS, then reads g_entities[n].client, which is NULL from
		// sv_maxclients up to MAX_CLIENTS - 1
		if (!Q_stricmp(cmd_arg1, "clientkick"))
		{
			minArg2Value = 0;
			maxArg2Value = proxy.trap->Cvar_VariableIntegerValue("sv_maxclients") - 1;
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
			return false;
		}
	}

	if (Q_strchrs(argsConcat, "\r\n"))
	{
		return false;
	}

	if (!sayCmd && Q_strchrs(argsConcat, ";"))
	{
		return false;
	}

	return true;
}

bool Run(int clientNum, const char* cmd)
{
	if (!Q_stricmp(cmd, "myratio"))
	{
		MyRatio(clientNum);

		return true;
	}

	if (!Q_stricmp(cmd, "proxyconfig"))
	{
		ClientData* currentClientData = &proxy.clientData[clientNum];

		// Several prints: once per second at most, so a client cannot fill its reliable commands
		if (currentClientData->lastTimeProxyConfig + 1000 <= proxy.levelTime)
		{
			currentClientData->lastTimeProxyConfig = proxy.levelTime;
			features::cvars::PrintConfig(clientNum);
		}

		return true;
	}

	return false;
}

void ReplaceArguments(const char* cmd)
{
	ClientCommandDone();

	char arg1[MAX_TOKEN_CHARS];

	proxy.trap->Argv(1, arg1, sizeof(arg1));

	// The game tests msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1', a typo of Quake 3: "vote Y" and "vote 1" counted as
	// no. Same rule as ioq3 and OpenJK, a missing answer is no
	if (!Q_stricmp(cmd, "vote"))
	{
		Q_strncpyz(gameArgv[1], (arg1[0] == 'y' || arg1[0] == 'Y' || arg1[0] == '1') ? "yes" : "no", sizeof(gameArgv[1]));
	}
	// The game's kick branch builds the same vote ("clientkick <slot>", shown as "kick <name>") as its clientkick branch.
	// Allowed already dropped the vote when nobody has this name
	else if (!Q_stricmp(cmd, "callvote") && !Q_stricmp(arg1, "kick"))
	{
		char name[MAX_TOKEN_CHARS];

		proxy.trap->Argv(2, name, sizeof(name));

		const int target = ClientNumberFromName(name);

		if (target >= 0)
		{
			Q_strncpyz(gameArgv[1], "clientkick", sizeof(gameArgv[1]));
			Com_sprintf(gameArgv[2], sizeof(gameArgv[2]), "%i", target);
		}
	}
}

void ClientCommandDone(void)
{
	std::memset(gameArgv, 0, sizeof(gameArgv));
}

bool GameArgv(int n, char* buffer, int bufferLength)
{
	if (n < 1 || n >= static_cast<int>(ARRAY_LEN(gameArgv)) || !gameArgv[n][0])
	{
		return false;
	}

	Q_strncpyz(buffer, gameArgv[n], bufferLength);

	return true;
}

} // namespace ybeproxy::features::client_commands
