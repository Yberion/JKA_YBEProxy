#include "EnginePatch/Proxy_EnginePatch.hpp"
#include "DetourPatcher/DetourPatcher.hpp"
#include "Proxy_Shell.hpp"
#include "Proxy_Server.hpp"

// ==================================================
// Proxy_EnginePatch_Attach
// --------------------------------------------------
// Attach all the engine functions, shouldn't be very
// long without these detours. This allows me to detour
// calls into engine functions so I can add additional
// options, features or fixes.
// ==================================================

void Proxy_Patch_Attach(void)
{
	Original_SV_UserMove = (void (*)(client_t*, msg_t*, qboolean)) Attach((unsigned char*)func_SV_UserMove_addr, (unsigned char*)&Proxy_SV_UserMove);
	Original_SV_SendMessageToClient = (void (*)(msg_t*, client_t*)) Attach((unsigned char*)func_SV_SendMessageToClient_addr, (unsigned char*)&Proxy_SV_SendMessageToClient);
	Original_SV_CalcPings = (void (*)(void)) Attach((unsigned char*)func_SV_CalcPings_addr, (unsigned char*)&Proxy_SV_CalcPings);
	Original_SV_SvEntityForGentity = (svEntity_t* (*)(sharedEntity_t*)) Attach((unsigned char*)func_SV_SvEntityForGentity_addr, (unsigned char*)&Proxy_SV_SvEntityForGentity);
	Original_Common_Com_Printf = (void (QDECL *)(const char* fmt, ...)) Attach((unsigned char*)func_Com_Printf_addr, (unsigned char*)&Proxy_Common_Com_Printf);
	Original_SV_Status_f = (void (*)(void)) Attach((unsigned char*)func_SV_Status_f_addr, (unsigned char*)&Proxy_SV_Status_f);
	Original_SV_SendClientGameState = (void (*)(client_t*)) Attach((unsigned char*)func_SV_SendClientGameState_addr, (unsigned char*)&Proxy_SV_SendClientGameState);
}

// ==================================================
// Proxy_EnginePatch_Detach
// --------------------------------------------------
// We are closing and we should remove the detour,
// otherwise the jump might be done to an undefined
// function and we can't restore the original bytes
// anymore.
// ==================================================

void Proxy_Patch_Detach(void)
{
	Detach((unsigned char*)func_SV_UserMove_addr, (unsigned char*)Original_SV_UserMove);
	Detach((unsigned char*)func_SV_SendMessageToClient_addr, (unsigned char*)Original_SV_SendMessageToClient);
	Detach((unsigned char*)func_SV_CalcPings_addr, (unsigned char*)Original_SV_CalcPings);
	Detach((unsigned char*)func_SV_SvEntityForGentity_addr, (unsigned char*)Original_SV_SvEntityForGentity);
	Detach((unsigned char*)func_Com_Printf_addr, (unsigned char*)Original_Common_Com_Printf);
	Detach((unsigned char*)func_SV_Status_f_addr, (unsigned char*)Original_SV_Status_f);
	Detach((unsigned char*)func_SV_SendClientGameState_addr, (unsigned char*)Original_SV_SendClientGameState);
}
