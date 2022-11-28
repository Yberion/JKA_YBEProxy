#include "JKA_YBEProxy/Proxy_Header.hpp"
#include "Proxy_OpenJKAPI_Wrappers.hpp"
#include "JKA_YBEProxy/Proxy_SharedAPI.hpp"

// ==================================================
// IMPORT TABLE
// ==================================================

static void Proxy_OpenJKAPI_GetUsercmd(int clientNum, usercmd_t* cmd)
{
	Proxy_SharedAPI_GetUsercmd(clientNum, cmd);

	proxy.originalOpenJKAPIGameImportTable->GetUsercmd(clientNum, cmd);
}

static void Proxy_OpenJKAPI_LocateGameData(sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient)
{
	Proxy_SharedAPI_LocateGameData(gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGameClient);

	proxy.originalOpenJKAPIGameImportTable->LocateGameData(gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGameClient);
}

// ==================================================
// EXPORT TABLE
// ==================================================

static void Proxy_OpenJKAPI_ClientBegin(int clientNum, qboolean allowTeamReset)
{
	Proxy_SharedAPI_ClientBegin(clientNum, allowTeamReset);

	proxy.originalOpenJKAPIGameExportTable->ClientBegin(clientNum, allowTeamReset);
}

static void Proxy_OpenJKAPI_ClientCommand(int clientNum)
{
	if (!Proxy_SharedAPI_ClientCommand(clientNum))
	{
		return;
	}

	proxy.originalOpenJKAPIGameExportTable->ClientCommand(clientNum);
}

static char* Proxy_OpenJKAPI_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot)
{
	Proxy_SharedAPI_ClientConnect(clientNum, firstTime, isBot);

	return proxy.originalOpenJKAPIGameExportTable->ClientConnect(clientNum, firstTime, isBot);
}

static void Proxy_OpenJKAPI_ClientDisconnect(int clientNum)
{
	Proxy_SharedAPI_ClientDisconnect(clientNum);

	proxy.originalOpenJKAPIGameExportTable->ClientDisconnect(clientNum);
}

/*
static void Proxy_OpenJKAPI_ClientThink(int clientNum, usercmd_t* ucmd)
{
	Proxy_SharedAPI_ClientThink(clientNum, ucmd);

	proxy.originalOpenJKAPIGameExportTable->ClientThink(clientNum, ucmd);
}
*/

static qboolean Proxy_OpenJKAPI_ClientUserinfoChanged(int clientNum)
{
	Proxy_SharedAPI_ClientUserinfoChanged(clientNum);

	return proxy.originalOpenJKAPIGameExportTable->ClientUserinfoChanged(clientNum);
}

static void Proxy_OpenJKAPI_RunFrame(int levelTime)
{
	proxy.originalOpenJKAPIGameExportTable->RunFrame(levelTime);
}

static void Proxy_OpenJKAPI_ShutdownGame(int restart)
{
	if (proxy.jampgameHandle)
	{
		proxy.originalOpenJKAPIGameExportTable->ShutdownGame(restart);

		// We can close our proxy library
		YBEProxy_CloseLibrary(proxy.jampgameHandle);
	}
}

// ==================================================
// INITIALIZE LAYER
// ==================================================

void Proxy_OpenJKAPI_InitLayerImportTable(void)
{
	proxy.copyOpenJKAPIGameImportTable->GetUsercmd = Proxy_OpenJKAPI_GetUsercmd;
	proxy.copyOpenJKAPIGameImportTable->LocateGameData = Proxy_OpenJKAPI_LocateGameData;
}

void Proxy_OpenJKAPI_InitLayerExportTable(void)
{
	proxy.copyOpenJKAPIGameExportTable->ClientBegin = Proxy_OpenJKAPI_ClientBegin;
	proxy.copyOpenJKAPIGameExportTable->ClientCommand = Proxy_OpenJKAPI_ClientCommand;
	proxy.copyOpenJKAPIGameExportTable->ClientConnect = Proxy_OpenJKAPI_ClientConnect;
	proxy.copyOpenJKAPIGameExportTable->ClientDisconnect = Proxy_OpenJKAPI_ClientDisconnect;
	//proxy.copyOpenJKAPIGameExportTable->ClientThink = Proxy_OpenJKAPI_ClientThink;
	proxy.copyOpenJKAPIGameExportTable->ClientUserinfoChanged = Proxy_OpenJKAPI_ClientUserinfoChanged;
	proxy.copyOpenJKAPIGameExportTable->RunFrame = Proxy_OpenJKAPI_RunFrame;
	proxy.copyOpenJKAPIGameExportTable->ShutdownGame = Proxy_OpenJKAPI_ShutdownGame;
}