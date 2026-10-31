#include "JKA_YBEProxy/Module/GameExports.hpp"
#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Features/Chat.hpp"
#include "JKA_YBEProxy/Features/ClientCommands.hpp"
#include "JKA_YBEProxy/Features/Connection.hpp"
#include "JKA_YBEProxy/Features/ConsoleCommands.hpp"
#include "JKA_YBEProxy/Features/Userinfo.hpp"
#include "JKA_YBEProxy/Engine/NetStatusCommands.hpp"

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

const char* Proxy_SharedAPI_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot)
{
	// The game reads the userinfo here, before SV_UserinfoChanged and GAME_CLIENT_USERINFO_CHANGED
	// Bots are skipped: their userinfo comes from the bot files
	if (!isBot)
	{
		const char* reason = Proxy_Connection_Check(clientNum, firstTime);

		if (!reason)
		{
			reason = Proxy_SharedAPI_SanitizeUserinfo(clientNum, true, nullptr);
		}

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
	Proxy_Connection_Connected(clientNum, isBot);
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
		Proxy_Connection_Begin(clientNum);
	}
}

qboolean Proxy_SharedAPI_ConsoleCommand(void)
{
	return Proxy_ConsoleCommands_Run();
}

qboolean Proxy_SharedAPI_ClientCommand(int clientNum)
{
	if (!proxy.clientData[clientNum].isConnected)
	{
		return qfalse;
	}

	char cmd[MAX_TOKEN_CHARS] = { 0 };

	proxy.trap->Argv(0, cmd, sizeof(cmd));

	if (!Proxy_ClientCommands_Allowed(clientNum, cmd) || Proxy_ClientCommands_Run(clientNum, cmd))
	{
		return qfalse;
	}

	// Only work on default engine since it require memory hook
	if (proxy.isOriginalEngine && Proxy_Engine_NetStatusCommands_Run(clientNum, cmd))
	{
		return qfalse;
	}

	Proxy_Chat_ClientCommand(clientNum, cmd);

	return qtrue;
}

void Proxy_SharedAPI_ClientCommandDone(void)
{
	Proxy_Chat_ClientCommandDone();
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
