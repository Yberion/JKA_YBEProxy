#include "Proxy_Header.h"

void Proxy_NewAPI_InitLayerExportTable(void)
{
	proxy.copyNewAPIGameExportTable->ClientConnect = Proxy_NewAPI_ClientConnect;
	proxy.copyNewAPIGameExportTable->ClientBegin = Proxy_NewAPI_ClientBegin;
	proxy.copyNewAPIGameExportTable->ClientCommand = Proxy_NewAPI_ClientCommand;
	proxy.copyNewAPIGameExportTable->ClientThink = Proxy_NewAPI_ClientThink;
	proxy.copyNewAPIGameExportTable->ClientUserinfoChanged = Proxy_NewAPI_ClientUserinfoChanged;
	proxy.copyNewAPIGameExportTable->RunFrame = Proxy_NewAPI_RunFrame;
	proxy.copyNewAPIGameExportTable->ShutdownGame = Proxy_NewAPI_ShutdownGame;
}

void Proxy_NewAPI_InitLayerImportTable(void)
{
	proxy.copyNewAPIGameImportTable->GetUsercmd = Proxy_NewAPI_GetUsercmd;
	proxy.copyNewAPIGameImportTable->LocateGameData = Proxy_NewAPI_LocateGameData;
}

// ==================================================
// IMPORT TABLE
// ==================================================

void Proxy_NewAPI_GetUsercmd(int clientNum, usercmd_t* cmd)
{
	Proxy_SharedAPI_GetUsercmd(clientNum, cmd);

	proxy.originalNewAPIGameImportTable->GetUsercmd(clientNum, cmd);
}

void Proxy_NewAPI_LocateGameData(sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient)
{
	Proxy_SharedAPI_LocateGameData(gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGameClient);

	proxy.originalNewAPIGameImportTable->LocateGameData(gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGameClient);
}

// ==================================================
// EXPORT TABLE
// ==================================================

char* Proxy_NewAPI_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot)
{
	Proxy_SharedAPI_ClientConnect(clientNum, firstTime, isBot);

	return proxy.originalNewAPIGameExportTable->ClientConnect(clientNum, firstTime, isBot);
}

void Proxy_NewAPI_ClientBegin(int clientNum, qboolean allowTeamReset)
{
	Proxy_SharedAPI_ClientBegin(clientNum, allowTeamReset);

	proxy.originalNewAPIGameExportTable->ClientBegin(clientNum, allowTeamReset);
}

void Proxy_NewAPI_ClientCommand(int clientNum)
{
	if (!Proxy_SharedAPI_ClientCommand(clientNum))
	{
		return;
	}

	proxy.originalNewAPIGameExportTable->ClientCommand(clientNum);
}

void Proxy_NewAPI_ClientThink(int clientNum, usercmd_t* ucmd)
{
	Proxy_SharedAPI_ClientThink(clientNum);

	proxy.originalNewAPIGameExportTable->ClientThink(clientNum, ucmd);
}

qboolean Proxy_NewAPI_ClientUserinfoChanged(int clientNum)
{
	Proxy_SharedAPI_ClientUserinfoChanged(clientNum);

	return proxy.originalNewAPIGameExportTable->ClientUserinfoChanged(clientNum);
}

void Proxy_NewAPI_RunFrame(int levelTime)
{
	proxy.originalNewAPIGameExportTable->RunFrame(levelTime);
}

void Proxy_NewAPI_ShutdownGame(int restart)
{
	if (proxy.jampgameHandle)
	{
		proxy.originalNewAPIGameExportTable->ShutdownGame(restart);

		// We can close our proxy library
		YBEProxy_CloseLibrary(proxy.jampgameHandle);
	}
}