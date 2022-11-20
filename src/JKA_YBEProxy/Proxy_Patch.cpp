#include "EnginePatch/Proxy_EnginePatch.hpp"
#include "DetourPatcher/DetourPatcher.hpp"
#include "Proxy_Shell.hpp"
#include "Proxy_Server.hpp"

#include <variant>

static std::variant<
	HookUtils::HookEntry<decltype(Proxy_Com_Printf)>,
	HookUtils::HookEntry<decltype(Proxy_Cmd_TokenizeString)>,
	HookUtils::HookEntry<decltype(Proxy_SV_SendMessageToClient)>,
	HookUtils::HookEntry<decltype(Proxy_SV_CalcPings)>, // HookEntry<decltype(&Proxy_SV_Status_f)>,
	HookUtils::HookEntry<decltype(Proxy_SV_SvEntityForGentity)>,
	HookUtils::HookEntry<decltype(Proxy_SV_SendClientGameState)>, // HookEntry<decltype(&Proxy_SV_UserinfoChanged)>, HookEntry<decltype(&Proxy_SV_BeginDownload_f)>, HookEntry<decltype(&Proxy_SV_DoneDownload_f)>,
	HookUtils::HookEntry<decltype(Proxy_SV_ExecuteClientMessage)>,
	HookUtils::HookEntry<decltype(Proxy_SVC_Status)>, // HookEntry<decltype(&Proxy_SVC_Info)>,
	HookUtils::HookEntry<decltype(Proxy_SV_PacketEvent)>, // HookEntry<decltype(&Proxy_SV_ConnectionlessPacket)>, HookEntry<decltype(&Proxy_SVC_RemoteCommand)>, HookEntry<decltype(&Proxy_SV_WriteDownloadToClient)>,
	HookUtils::HookEntry<decltype(Proxy_SV_ExecuteClientCommand)>
> hookEntries[] = {
	HookUtils::HookEntry("Com_Printf", (unsigned char*)func_Com_Printf_addr, &Original_Com_Printf, &Proxy_Com_Printf, 0),
	HookUtils::HookEntry("Cmd_TokenizeString", (unsigned char*)func_Cmd_TokenizeString_addr, &Original_Cmd_TokenizeString, &Proxy_Cmd_TokenizeString, 0),
	HookUtils::HookEntry("SV_SendMessageToClient", (unsigned char*)func_SV_SendMessageToClient_addr, &Original_SV_SendMessageToClient, &Proxy_SV_SendMessageToClient, 0),
	HookUtils::HookEntry("SV_CalcPings", (unsigned char*)func_SV_CalcPings_addr, &Original_SV_CalcPings, &Proxy_SV_CalcPings, 0),
	HookUtils::HookEntry("SV_SvEntityForGentity", (unsigned char*)func_SV_SvEntityForGentity_addr, &Original_SV_SvEntityForGentity, &Proxy_SV_SvEntityForGentity, 0),
	HookUtils::HookEntry("SV_SendClientGameState", (unsigned char*)func_SV_SendClientGameState_addr, &Original_SV_SendClientGameState, &Proxy_SV_SendClientGameState, 0),
	HookUtils::HookEntry("SV_ExecuteClientMessage", (unsigned char*)func_SV_ExecuteClientMessage_addr, &Original_SV_ExecuteClientMessage, &Proxy_SV_ExecuteClientMessage, 0),
	HookUtils::HookEntry("SV_UserinfoChanged", (unsigned char*)func_SV_UserinfoChanged_addr, &Original_SV_UserinfoChanged, &Proxy_SV_UserinfoChanged, 0),
	HookUtils::HookEntry("SV_Status_f", (unsigned char*)func_SV_Status_f_addr, &Original_SV_Status_f, &Proxy_SV_Status_f, 0),
	HookUtils::HookEntry("SVC_Status", (unsigned char*)func_SVC_Status_addr, &Original_SVC_Status, &Proxy_SVC_Status, 0),
	HookUtils::HookEntry("SVC_Info", (unsigned char*)func_SVC_Info_addr, &Original_SVC_Info, &Proxy_SVC_Info, 0),
	HookUtils::HookEntry("SV_PacketEvent", (unsigned char*)func_SV_PacketEvent_addr, &Original_SV_PacketEvent, &Proxy_SV_PacketEvent, 0),
	HookUtils::HookEntry("SV_ConnectionlessPacket", (unsigned char*)func_SV_ConnectionlessPacket_addr, &Original_SV_ConnectionlessPacket, &Proxy_SV_ConnectionlessPacket, 0),
	HookUtils::HookEntry("SVC_RemoteCommand", (unsigned char*)func_SVC_RemoteCommand_addr, &Original_SVC_RemoteCommand, &Proxy_SVC_RemoteCommand, 0),
	HookUtils::HookEntry("SV_BeginDownload_f", (unsigned char*)func_SV_BeginDownload_f_addr, &Original_SV_BeginDownload_f, &Proxy_SV_BeginDownload_f, 0),
	HookUtils::HookEntry("SV_DoneDownload_f", (unsigned char*)func_SV_DoneDownload_f_addr, &Original_SV_DoneDownload_f, &Proxy_SV_DoneDownload_f, 0),
	HookUtils::HookEntry("SV_ExecuteClientCommand", (unsigned char*)func_SV_ExecuteClientCommand_addr, &Original_SV_ExecuteClientCommand, &Proxy_SV_ExecuteClientCommand, 0),
	HookUtils::HookEntry("SV_WriteDownloadToClient", (unsigned char*)func_SV_WriteDownloadToClient_addr, &Original_SV_WriteDownloadToClient, &Proxy_SV_WriteDownloadToClient, 0)
};

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
	for (auto& hookEntry : hookEntries) {
		std::visit([](auto& hookEntryVisitor) {
			if (!HookUtils::Attach(hookEntryVisitor))
			{
				proxy.trap->Print("----- Proxy: Could not allocate memory for the trampoline '%s'", hookEntryVisitor.hookName);
			}
		}, hookEntry);
	}
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
	for (auto& hookEntry : hookEntries) {
		std::visit([](auto& hookEntryVisitor) {
			if (!HookUtils::Detach(hookEntryVisitor))
			{
				proxy.trap->Print("----- Proxy: Could not release memory for the trampoline '%s'", hookEntryVisitor.hookName);
			}
		}, hookEntry);
	}
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
	HookUtils::Patch_NOP_Bytes((unsigned char*)func_SVC_RemoteCommand_timer_start_block_addr, func_SVC_RemoteCommand_timer_NOP_amount);
	// When calling Com_BeginRedirect in SVC_RemoteCommand, change the length of the buffer from 49136 to 1008
	// It fix truncated text from SVC_RemoteCommand with the new Com_Printf
	HookUtils::Patch((unsigned char*)func_SVC_RemoteCommand_Com_BeginRedirect_SV_OUTPUTBUF_LENGTH_addr, 0x03);
}