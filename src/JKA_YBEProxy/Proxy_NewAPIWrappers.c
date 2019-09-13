#include "Proxy_Header.h"

void Proxy_NewAPI_InitLayerExportTable(void)
{
	proxy.copyNewAPIGameExportTable->ClientConnect = Proxy_NewAPI_ClientConnect;
	proxy.copyNewAPIGameExportTable->ShutdownGame = Proxy_NewAPI_ShutdownGame;
	proxy.copyNewAPIGameExportTable->RunFrame = Proxy_NewAPI_RunFrame;
}

void Proxy_NewAPI_InitLayerImportTable(void)
{

}

// ==================================================
// IMPORT TABLE
// ==================================================

// ==================================================
// EXPORT TABLE
// ==================================================

char* Proxy_NewAPI_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot)
{
	/*
	// Doesn't work on the new API
	if (firstTime && !isBot)
	{
		proxy.trap->SendServerCommand(-1, va("print \"^5%s (^7%s^5) %s^7\n\"", YBEPROXY_NAME, YBEPROXY_VERSION, YBEPROXY_BY_AUTHOR));
	}
	*/
	return proxy.originalNewAPIGameExportTable->ClientConnect(clientNum, firstTime, isBot);
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

void Proxy_NewAPI_RunFrame(int levelTime)
{
	proxy.originalNewAPIGameExportTable->RunFrame(levelTime);
}