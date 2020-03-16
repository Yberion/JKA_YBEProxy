#include "Proxy_Server.h"

#if defined(_WIN32) && !defined(MINGW32)
	int isWindows = 1;
#else
	int isWindows = 0;
#endif

ProxyServer_t proxyServer = { 0 };

void Proxy_Initialize_Server_MemoryAddress(void)
{
	proxyServer.svs = (serverStatic_t*)var_svs_addr;
}