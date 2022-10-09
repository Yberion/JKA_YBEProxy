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
	Original_SV_SendMessageToClient = (void (*)(msg_t*, client_t*)) Attach((unsigned char*)func_SV_SendMessageToClient_addr, (unsigned char*)&Proxy_SV_SendMessageToClient);
	Original_SV_CalcPings = (void (*)(void)) Attach((unsigned char*)func_SV_CalcPings_addr, (unsigned char*)&Proxy_SV_CalcPings);
	Original_SV_SvEntityForGentity = (svEntity_t* (*)(sharedEntity_t*)) Attach((unsigned char*)func_SV_SvEntityForGentity_addr, (unsigned char*)&Proxy_SV_SvEntityForGentity);
	Original_SV_Status_f = (void (*)(void)) Attach((unsigned char*)func_SV_Status_f_addr, (unsigned char*)&Proxy_SV_Status_f);
	Original_SV_SendClientGameState = (void (*)(client_t*)) Attach((unsigned char*)func_SV_SendClientGameState_addr, (unsigned char*)&Proxy_SV_SendClientGameState);
	Original_SV_UserinfoChanged = (void (*)(client_t*)) Attach((unsigned char*)func_SV_UserinfoChanged_addr, (unsigned char*)&Proxy_SV_UserinfoChanged);
	Original_SV_ExecuteClientMessage = (void (*)(client_t*, msg_t*)) Attach((unsigned char*)func_SV_ExecuteClientMessage_addr, (unsigned char*)&Proxy_SV_ExecuteClientMessage);
	Original_SVC_Status = (void (*)(netadr_t)) Attach((unsigned char*)func_SVC_Status_addr, (unsigned char*)&Proxy_SVC_Status);
	Original_SVC_Info = (void (*)(netadr_t)) Attach((unsigned char*)func_SVC_Info_addr, (unsigned char*)&Proxy_SVC_Info);
	Original_SVC_RemoteCommand = (void (*)(netadr_t, msg_t*)) Attach((unsigned char*)func_SVC_RemoteCommand_addr, (unsigned char*)&Proxy_SVC_RemoteCommand);
	Original_Common_Com_Printf = (void (QDECL *)(const char* fmt, ...)) Attach((unsigned char*)func_Com_Printf_addr, (unsigned char*)&Proxy_Common_Com_Printf);
	Original_Cmd_TokenizeString = (void (*)(const char*)) Attach((unsigned char*)func_Cmd_TokenizeString_addr, (unsigned char*)&Proxy_Cmd_TokenizeString);
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
	Detach((unsigned char*)func_SV_SendMessageToClient_addr, (unsigned char*)Original_SV_SendMessageToClient);
	Detach((unsigned char*)func_SV_CalcPings_addr, (unsigned char*)Original_SV_CalcPings);
	Detach((unsigned char*)func_SV_SvEntityForGentity_addr, (unsigned char*)Original_SV_SvEntityForGentity);
	Detach((unsigned char*)func_SV_Status_f_addr, (unsigned char*)Original_SV_Status_f);
	Detach((unsigned char*)func_SV_SendClientGameState_addr, (unsigned char*)Original_SV_SendClientGameState);
	Detach((unsigned char*)func_SV_ExecuteClientMessage_addr, (unsigned char*)Original_SV_ExecuteClientMessage);
	Detach((unsigned char*)func_SV_UserinfoChanged_addr, (unsigned char*)Original_SV_UserinfoChanged);
	Detach((unsigned char*)func_SVC_Status_addr, (unsigned char*)Original_SVC_Status);
	Detach((unsigned char*)func_SVC_Info_addr, (unsigned char*)Original_SVC_Info);
	Detach((unsigned char*)func_SVC_RemoteCommand_addr, (unsigned char*)Original_SVC_RemoteCommand);
	Detach((unsigned char*)func_Com_Printf_addr, (unsigned char*)Original_Common_Com_Printf);
	Detach((unsigned char*)func_Cmd_TokenizeString_addr, (unsigned char*)Original_Cmd_TokenizeString);
}
