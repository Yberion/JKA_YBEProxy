#include "Proxy_Header.h"

void Proxy_NewAPI_InitLayerExportTable(void)
{
	proxy.copyNewAPIGameExportTable->ClientConnect = Proxy_NewAPI_ClientConnect;
	proxy.copyNewAPIGameExportTable->ShutdownGame = Proxy_NewAPI_ShutdownGame;
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
	if (firstTime && !isBot)
	{
		proxy.originalNewAPIGameImportTable->SendServerCommand(-1, va("print \"^5%s (^7%s^5) %s^7\n\"", YBEPROXY_NAME, YBEPROXY_VERSION, YBEPROXY_BY_AUTHOR));
		proxy.originalNewAPIGameImportTable->SendServerCommand(-1, "print \"FDSOIFDSOF\n\"");
	}

	return proxy.originalNewAPIGameExportTable->ClientConnect(clientNum, firstTime, isBot);
}

void Proxy_NewAPI_ShutdownGame(int restart)
{
	if (proxy.jampgameHandle)
	{
		// We can close our proxy library
		YBEProxy_CloseLibrary(proxy.jampgameHandle);

		proxy.originalNewAPIGameExportTable->ShutdownGame(restart);
	}
}