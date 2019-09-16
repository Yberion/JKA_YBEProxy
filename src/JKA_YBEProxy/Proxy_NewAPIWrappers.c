#include "Proxy_Header.h"

void Proxy_NewAPI_InitLayerExportTable(void)
{
	proxy.copyNewAPIGameExportTable->ShutdownGame = Proxy_NewAPI_ShutdownGame;
	proxy.copyNewAPIGameExportTable->ClientConnect = Proxy_NewAPI_ClientConnect;
	proxy.copyNewAPIGameExportTable->ClientBegin = Proxy_NewAPI_ClientBegin;
	proxy.copyNewAPIGameExportTable->ClientCommand = Proxy_NewAPI_ClientCommand;
	proxy.copyNewAPIGameExportTable->RunFrame = Proxy_NewAPI_RunFrame;
}

void Proxy_NewAPI_InitLayerImportTable(void)
{
	proxy.copyNewAPIGameImportTable->LocateGameData = Proxy_NewAPI_LocateGameData;
}

// ==================================================
// IMPORT TABLE
// ==================================================

void Proxy_NewAPI_LocateGameData(sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient)
{
	Proxy_Shared_LocateGameData(gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGameClient);

	proxy.originalNewAPIGameImportTable->LocateGameData(gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGameClient);
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

void Proxy_NewAPI_RunFrame(int levelTime)
{
	proxy.originalNewAPIGameExportTable->RunFrame(levelTime);
}