#include "EnginePatch/Proxy_EnginePatch.hpp"
#include "DetourPatcher/DetourPatcher.hpp"
#include "Proxy_Shell.hpp"
#include "Proxy_Server.hpp"

// ==================================================
// Proxy_Patch_Attach
// --------------------------------------------------
// Attach all the engine functions, shouldn't be very
// long without these detours. This allows me to detour
// calls into engine functions so I can add additional
// options, features or fixes.
// ==================================================

void Proxy_Patch_Attach(void)
{
	Original_Common_Com_Printf = (void (QDECL *)(const char*, ...)) Attach((unsigned char*)func_Com_Printf_addr, (unsigned char*)&Proxy_Common_Com_Printf);
	Original_Cmd_TokenizeString = (void (*)(const char*)) Attach((unsigned char*)func_Cmd_TokenizeString_addr, (unsigned char*)&Proxy_Cmd_TokenizeString);
	Original_SV_SendMessageToClient = (void (*)(msg_t*, client_t*)) Attach((unsigned char*)func_SV_SendMessageToClient_addr, (unsigned char*)&Proxy_SV_SendMessageToClient);
	Original_SV_CalcPings = (void (*)(void)) Attach((unsigned char*)func_SV_CalcPings_addr, (unsigned char*)&Proxy_SV_CalcPings);
	Original_SV_SvEntityForGentity = (svEntity_t* (*)(sharedEntity_t*)) Attach((unsigned char*)func_SV_SvEntityForGentity_addr, (unsigned char*)&Proxy_SV_SvEntityForGentity);
	Original_SV_Status_f = (void (*)(void)) Attach((unsigned char*)func_SV_Status_f_addr, (unsigned char*)&Proxy_SV_Status_f);
	Original_SV_SendClientGameState = (void (*)(client_t*)) Attach((unsigned char*)func_SV_SendClientGameState_addr, (unsigned char*)&Proxy_SV_SendClientGameState);
	Original_SV_UserinfoChanged = (void (*)(client_t*)) Attach((unsigned char*)func_SV_UserinfoChanged_addr, (unsigned char*)&Proxy_SV_UserinfoChanged);
	Original_SV_ExecuteClientMessage = (void (*)(client_t*, msg_t*)) Attach((unsigned char*)func_SV_ExecuteClientMessage_addr, (unsigned char*)&Proxy_SV_ExecuteClientMessage);
	Original_SVC_Status = (void (*)(netadr_t)) Attach((unsigned char*)func_SVC_Status_addr, (unsigned char*)&Proxy_SVC_Status);
	Original_SVC_Info = (void (*)(netadr_t)) Attach((unsigned char*)func_SVC_Info_addr, (unsigned char*)&Proxy_SVC_Info);
	Original_SV_PacketEvent = (void (*)(netadr_t, msg_t *)) Attach((unsigned char*)func_SV_PacketEvent_addr, (unsigned char*)&Proxy_SV_PacketEvent);
	Original_SV_ConnectionlessPacket = (void (*)(netadr_t, msg_t*)) Attach((unsigned char*)func_SV_ConnectionlessPacket_addr, (unsigned char*)&Proxy_SV_ConnectionlessPacket);
	Original_SVC_RemoteCommand = (void (*)(netadr_t, msg_t *)) Attach((unsigned char*)func_SVC_RemoteCommand_addr, (unsigned char*)&Proxy_SVC_RemoteCommand);
	Original_SV_BeginDownload_f = (void (*)(client_t*)) Attach((unsigned char*)func_SV_BeginDownload_f_addr, (unsigned char*)&Proxy_SV_BeginDownload_f);
	Original_SV_DoneDownload_f = (void (*)(client_t*)) Attach((unsigned char*)func_SV_DoneDownload_f_addr, (unsigned char*)&Proxy_SV_DoneDownload_f);
	Original_SV_ExecuteClientCommand = (void (*)(client_t*, const char*, qboolean)) Attach((unsigned char*)func_SV_ExecuteClientCommand_addr, (unsigned char*)&Proxy_SV_ExecuteClientCommand);
	Original_SV_WriteDownloadToClient = (void (*)(client_t*, msg_t*)) Attach((unsigned char*)func_SV_WriteDownloadToClient_addr, (unsigned char*)&Proxy_SV_WriteDownloadToClient);
}

// ==================================================
// Proxy_Patch_Detach
// --------------------------------------------------
// We are closing and we should remove the detour,
// otherwise the jump might be done to an undefined
// function and we can't restore the original bytes
// anymore.
// ==================================================

void Proxy_Patch_Detach(void)
{
	Detach((unsigned char*)func_Com_Printf_addr, (unsigned char*)Original_Common_Com_Printf);
	Detach((unsigned char*)func_Cmd_TokenizeString_addr, (unsigned char*)Original_Cmd_TokenizeString);
	Detach((unsigned char*)func_SV_SendMessageToClient_addr, (unsigned char*)Original_SV_SendMessageToClient);
	Detach((unsigned char*)func_SV_CalcPings_addr, (unsigned char*)Original_SV_CalcPings);
	Detach((unsigned char*)func_SV_SvEntityForGentity_addr, (unsigned char*)Original_SV_SvEntityForGentity);
	Detach((unsigned char*)func_SV_Status_f_addr, (unsigned char*)Original_SV_Status_f);
	Detach((unsigned char*)func_SV_SendClientGameState_addr, (unsigned char*)Original_SV_SendClientGameState);
	Detach((unsigned char*)func_SV_ExecuteClientMessage_addr, (unsigned char*)Original_SV_ExecuteClientMessage);
	Detach((unsigned char*)func_SV_UserinfoChanged_addr, (unsigned char*)Original_SV_UserinfoChanged);
	Detach((unsigned char*)func_SVC_Status_addr, (unsigned char*)Original_SVC_Status);
	Detach((unsigned char*)func_SVC_Info_addr, (unsigned char*)Original_SVC_Info);
	Detach((unsigned char*)func_SV_PacketEvent_addr, (unsigned char*)Original_SV_PacketEvent);
	Detach((unsigned char*)func_SV_ConnectionlessPacket_addr, (unsigned char*)Original_SV_ConnectionlessPacket);
	Detach((unsigned char*)func_SVC_RemoteCommand_addr, (unsigned char*)Original_SVC_RemoteCommand);
	Detach((unsigned char*)func_SV_BeginDownload_f_addr, (unsigned char*)Original_SV_BeginDownload_f);
	Detach((unsigned char*)func_SV_DoneDownload_f_addr, (unsigned char*)Original_SV_DoneDownload_f);
	Detach((unsigned char*)func_SV_ExecuteClientCommand_addr, (unsigned char*)Original_SV_ExecuteClientCommand);
	Detach((unsigned char*)func_SV_WriteDownloadToClient_addr, (unsigned char*)Original_SV_WriteDownloadToClient);
}

// ==================================================
// Proxy_Inline_Patch
// --------------------------------------------------
// Patches that doesn't require to be unpatched.
// It is not a problem to re-execute those patches.
// ==================================================

void Proxy_Inline_Patch(void)
{
	// Remove all the timer block in SVC_RemoteCommand, fix rcon disabler
	Patch_NOP_Bytes((unsigned char*)func_SVC_RemoteCommand_timer_start_block_addr, func_SVC_RemoteCommand_timer_NOP_amount);
	// When calling Com_BeginRedirect in SVC_RemoteCommand, change the length of the buffer from 49136 to 1008
	// It fix truncated text from SVC_RemoteCommand with the new Com_Printf
	Patch((unsigned char*)func_SVC_RemoteCommand_Com_BeginRedirect_SV_OUTPUTBUF_LENGTH_addr, 0x03);
}