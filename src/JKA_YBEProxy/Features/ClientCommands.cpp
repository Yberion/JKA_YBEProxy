#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Features/ClientCommands.hpp"
#include "JKA_YBEProxy/Ports/game/g_cmds.hpp"
#include "JKA_YBEProxy/Ports/server/sv_game.hpp"
#include "JKA_YBEProxy/Engine/EngineState.hpp"

/*
==================
Proxy_ClientCommand_MyRatio
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

void Proxy_ClientCommand_MyRatio(int clientNum)
{
	ClientData_t *currentClientData = &proxy.clientData[clientNum];

	if (currentClientData->lastTimeMyratioCheck + 1000 > proxy.proxyData.svsTime)
	{
		return;
	}

	currentClientData->lastTimeMyratioCheck = proxy.proxyData.svsTime;

	char ratioStringBuffer[16];
	playerState_t* ps;

	ps = Proxy_GetPlayerStateByClientNum(clientNum);

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

bool Proxy_ClientCommands_Allowed(int clientNum, const char* cmd)
{
	ClientData_t *currentClientData = &proxy.clientData[clientNum];
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
	const char* argsConcat = ConcatArgs(1);

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
	if (!Q_stricmp(cmd, "gc") && atoi(cmd_arg1) >= proxy.trap->Cvar_VariableIntegerValue("sv_maxclients"))
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

bool Proxy_ClientCommands_Run(int clientNum, const char* cmd)
{
	if (!Q_stricmp(cmd, "myratio"))
	{
		Proxy_ClientCommand_MyRatio(clientNum);

		return true;
	}

	return false;
}
