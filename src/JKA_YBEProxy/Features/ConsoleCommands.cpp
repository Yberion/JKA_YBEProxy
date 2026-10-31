#include "JKA_YBEProxy/Features/ConsoleCommands.hpp"
#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Features/Bans.hpp"
#include "JKA_YBEProxy/Features/Cvars.hpp"

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
qboolean Proxy_ConsoleCommands_Run(void)
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
