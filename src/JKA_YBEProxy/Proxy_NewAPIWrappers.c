#include "Proxy_Header.h"

void Proxy_NewAPI_InitLayerExportTable(void)
{
	proxy.copyNewAPIGameExportTable->ShutdownGame = Proxy_NewAPI_ShutdownGame;
	proxy.copyNewAPIGameExportTable->ClientConnect = Proxy_NewAPI_ClientConnect;
	proxy.copyNewAPIGameExportTable->ClientBegin = Proxy_NewAPI_ClientBegin;
	proxy.copyNewAPIGameExportTable->ClientCommand = Proxy_NewAPI_ClientCommand;
	proxy.copyNewAPIGameExportTable->ClientUserinfoChanged = Proxy_NewAPI_ClientUserinfoChanged;
	proxy.copyNewAPIGameExportTable->RunFrame = Proxy_NewAPI_RunFrame;
}

void Proxy_NewAPI_InitLayerImportTable(void)
{
	proxy.copyNewAPIGameImportTable->LocateGameData = Proxy_NewAPI_LocateGameData;
	proxy.copyNewAPIGameImportTable->GetUsercmd = Proxy_NewAPI_GetUsercmd;
}

// ==================================================
// IMPORT TABLE
// ==================================================

void Proxy_NewAPI_LocateGameData(sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient)
{
	Proxy_Shared_LocateGameData(gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGameClient);

	proxy.originalNewAPIGameImportTable->LocateGameData(gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGameClient);
}

void Proxy_NewAPI_GetUsercmd(int clientNum, usercmd_t* cmd)
{
	Proxy_Shared_GetUsercmd(clientNum, cmd);

	proxy.originalNewAPIGameImportTable->GetUsercmd(clientNum, cmd);
}

// ==================================================
// EXPORT TABLE
// ==================================================

void Proxy_NewAPI_ShutdownGame(int restart)
{
	if (proxy.jampgameHandle)
	{
		proxy.originalNewAPIGameExportTable->ShutdownGame(restart);

		// We can close our proxy library
		YBEProxy_CloseLibrary(proxy.jampgameHandle);
	}
}

char* Proxy_NewAPI_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot)
{
	Proxy_Shared_ClientConnect(clientNum, firstTime, isBot);

	return proxy.originalNewAPIGameExportTable->ClientConnect(clientNum, firstTime, isBot);
}

void Proxy_NewAPI_ClientBegin(int clientNum, qboolean allowTeamReset)
{
	Proxy_Shared_ClientBegin(clientNum, allowTeamReset);

	proxy.originalNewAPIGameExportTable->ClientBegin(clientNum, allowTeamReset);
}

void Proxy_NewAPI_ClientCommand(int clientNum)
{
	if (!Proxy_Shared_ClientCommand(clientNum))
	{
		return;
	}

	proxy.originalNewAPIGameExportTable->ClientCommand(clientNum);
}

qboolean Proxy_NewAPI_ClientUserinfoChanged(int clientNum)
{
	Proxy_Shared_ClientUserinfoChanged(clientNum);

	return proxy.originalNewAPIGameExportTable->ClientUserinfoChanged(clientNum);
}

void Proxy_NewAPI_RunFrame(int levelTime)
{
	proxy.originalNewAPIGameExportTable->RunFrame(levelTime);
}