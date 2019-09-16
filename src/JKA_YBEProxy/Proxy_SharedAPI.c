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

	proxy.trap->Argv(0, cmd, sizeof(cmd));

	if (!Q_stricmpn(&cmd[0], "jkaDST_", 7))
	{
		proxy.trap->DropClient(clientNum, "^3(Anticheat system) you got kicked cause of cheating^7");
		proxy.trap->SendServerCommand(-1, "chat \"^3(Anticheat system) ^7X^3 got kicked cause of cheating^7\"");

		return qfalse;
	}

	char cmd_arg1[MAX_TOKEN_CHARS] = { 0 };
	char cmd_arg2[MAX_TOKEN_CHARS] = { 0 };
	//char* message;

	//message = ConcatArgs(1);
	//proxy.trap->Print("cmd: %s, message: %s\n", cmd, message);

	return qtrue;
}