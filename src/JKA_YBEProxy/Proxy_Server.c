#include "Proxy_Server.h"

void Proxy_Server_Initialize_MemoryAddress(void)
{
	proxy.server.svs = (serverStatic_t*)var_svs_addr;
}