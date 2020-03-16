#include "Proxy_Server.h"
#include "Proxy_Header.h"

void Proxy_Server_Initialize_MemoryAddress(void)
{
	proxy.server.svs = (serverStatic_t*)var_svs_addr;

	proxy.server.functions.SV_Netchan_Transmit = (void (*)(client_t*, msg_t*))func_SV_Netchan_Transmit_addr;
	proxy.server.functions.SV_RateMsec = (int (*)(client_t*, int))func_SV_RateMsec_addr;
	proxy.server.functions.Netchan_TransmitNextFragment = (void	(*)(netchan_t *))func_Netchan_TransmitNextFragment_addr;
	proxy.server.functions.Sys_IsLANAddress = (qboolean (*)(netadr_t))func_Sys_IsLANAddress_addr;
}