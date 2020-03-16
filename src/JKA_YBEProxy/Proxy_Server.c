#include "Proxy_Server.h"
#include "Proxy_Header.h"

void Proxy_Server_Initialize_MemoryAddress(void)
{
	proxy.server.svs = (serverStatic_t*)var_svs_addr;
	proxy.server.sv = (server_t*)var_sv_addr;

	proxy.server.cvar.sv_pure = *(cvar_t**)cvar_sv_pure_addr;

	proxy.server.functions.SV_ClientEnterWorld = (void (*)(client_t*, usercmd_t*))func_SV_ClientEnterWorld_addr;
	proxy.server.functions.SV_ClientThink = (void (*)(client_t*, usercmd_t*))func_SV_ClientThink_addr;
	proxy.server.functions.SV_DropClient = (void (*)(client_t*, const char*))func_SV_DropClient_addr;
	proxy.server.functions.SV_Netchan_Transmit = (void (*)(client_t*, msg_t*))func_SV_Netchan_Transmit_addr;
	proxy.server.functions.SV_RateMsec = (int (*)(client_t*, int))func_SV_RateMsec_addr;
	proxy.server.functions.Com_HashKey = (int (*)(char*, int))func_Com_HashKey_addr;
	proxy.server.functions.Com_Printf = (void (*)(const char*, ...))func_Com_Printf_addr;
	proxy.server.functions.Netchan_TransmitNextFragment = (void (*)(netchan_t *))func_Netchan_TransmitNextFragment_addr;
	proxy.server.functions.MSG_ReadByte = (int (*)(msg_t*))func_MSG_ReadByte_addr;
	proxy.server.functions.MSG_ReadDeltaUsercmdKey = (void (*)(msg_t*, int, usercmd_t*, usercmd_t*))func_MSG_ReadDeltaUsercmdKey_addr;
	proxy.server.functions.Sys_IsLANAddress = (qboolean (*)(netadr_t))func_Sys_IsLANAddress_addr;
}