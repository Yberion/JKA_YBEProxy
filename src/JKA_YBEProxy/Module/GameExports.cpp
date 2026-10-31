#include "JKA_YBEProxy/Module/GameExports.hpp"
#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Features/Chat.hpp"
#include "JKA_YBEProxy/Features/ClientCommands.hpp"
#include "JKA_YBEProxy/Features/Connection.hpp"
#include "JKA_YBEProxy/Features/ConsoleCommands.hpp"
#include "JKA_YBEProxy/Features/Userinfo.hpp"
#include "JKA_YBEProxy/Engine/NetStatusCommands.hpp"
#include "JKA_YBEProxy/Features/Bans.hpp"
#include "JKA_YBEProxy/Features/Cvars.hpp"
#include "JKA_YBEProxy/Module/GameLibrary.hpp"
#include "JKA_YBEProxy/Engine/EngineState.hpp"
#include "JKA_YBEProxy/Engine/NetStatus.hpp"
#include "JKA_YBEProxy/Engine/Patches.hpp"
#include "JKA_YBEProxy/Engine/Restart.hpp"

void Proxy_SharedAPI_Init(void)
{
	proxy.trap->Print("================================================================\n");
	proxy.trap->Print("----- Proxy: %s %s %s\n", YBEPROXY_NAME, YBEPROXY_VERSION, YBEPROXY_BY_AUTHOR);
	proxy.trap->Print("================================================================\n");

	Proxy_GameLibrary_Load();

	proxy.trap->Print("----- Proxy: Initializing Proxy CVars\n");

	Proxy_CVars_Registration();

	proxy.trap->Print("----- Proxy: Proxy CVars properly initialized\n");

	char version[MAX_STRING_CHARS];

	proxy.trap->Cvar_VariableStringBuffer("version", version, sizeof(version));

	if (!Q_stricmpn(version, ORIGINAL_ENGINE_VERSION, sizeof(ORIGINAL_ENGINE_VERSION) - 1))
	{
		proxy.isOriginalEngine = true;
	}

	if (proxy.isOriginalEngine)
	{
		proxy.trap->Print("----- Proxy: Original engine detected\n");

		Proxy_Engine_Initialize_MemoryLayer();
		Proxy_OriginalEngine_CVars_Registration();

		proxy.trap->Print("----- Proxy: Patching engine\n");

		Proxy_Engine_Inline_Patches();
		Proxy_Engine_Attach_Patches();

		proxy.trap->Print("----- Proxy: Engine properly patched\n");
	}

	// Before the game loads its ban list from g_banIPs
	Proxy_Bans_BeforeGameInit();

	// Before the game loads the navigation of the map (trap_Nav_Load, or a node per waypoint). The game never frees
	// it (NAV_Shutdown is never called), and the retail CNavigator::Load adds to the nodes of the previous map: a
	// leak at each map change and map_restart, and NPCs that can pick a node of the previous map. OpenJK frees in
	// CNavigator::Load
	proxy.trap->Nav_Free();
}

void Proxy_SharedAPI_InitDone(void)
{
	if (proxy.isOriginalEngine)
	{
		// The game loads the sub-BSPs of the map (misc_bsp, model "#name") during its init. CM_LoadMap_Actual reads each
		// file into a TAG_BSP_DISKIMAGE buffer that it never frees: it only frees the buffer of the main map, at the end
		// of its load on a dedicated server. So the buffers left with that tag are the lost ones, and none is in use
		// outside CM_LoadMap_Actual. OpenJK frees the buffer of a sub-BSP at the end of its load
		server.common.functions.Z_TagFree(TAG_BSP_DISKIMAGE);
	}
}

void Proxy_SharedAPI_Shutdown(void)
{
	if (proxy.isOriginalEngine)
	{
		// On "rcon map XXX" or "rcon map_restart 0" it directly goes there from SVC_RemoteCommand,
		// which only calls Com_EndRedirect() once Cmd_ExecuteString() returns, after the whole map load
		// While the redirect is active, Com_Printf skips the console and the log file: end it here,
		// so the rest of the map load is printed there. The later Com_EndRedirect() then does nothing
		server.common.functions.Com_EndRedirect();

		proxy.trap->Print("----- Proxy: Unpatching engine\n");

		Proxy_Engine_Detach_Patches();

		proxy.trap->Print("----- Proxy: Engine properly unpatched\n");
	}
}

// The server is going to active its new frame, which often is around 20 times per second. Active some of our own
// active functions
void Proxy_SharedAPI_RunFrame(int levelTime)
{
	Proxy_UpdateAllCvars(levelTime);

	if (proxy.isOriginalEngine && proxy.originalEngineCvars.proxy_sv_enableNetStatus.integer)
	{
		Proxy_Engine_NetStatus_FrameClock();
	}

	if (proxy.isOriginalEngine)
	{
		Proxy_Engine_Restart_BeforeWrap();
	}

	proxy.proxyData.svsTime = levelTime;
}

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
