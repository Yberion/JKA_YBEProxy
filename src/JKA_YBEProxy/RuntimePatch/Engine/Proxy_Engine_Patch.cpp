#include "JKA_YBEProxy/RuntimePatch/HookUtils/HookUtils.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Patch.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Wrappers.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon/Proxy_cmd.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon/Proxy_common.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon/Proxy_files.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon/Proxy_msg.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_ccmds.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_client.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_game.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_main.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_snapshot.hpp"

#include <cstdlib>
#include <variant>

// A hook that cannot be installed leaves the server half patched: stop it, like when the original game module cannot
// be loaded
[[noreturn]] static void Proxy_FatalHookError(const char* hookName, HookUtils::Status status)
{
	proxy.trap->Print("----- Proxy: Could not hook '%s': %s\n", hookName, HookUtils::StatusString(status));

	exit(EXIT_FAILURE);
}

static std::variant<
	HookUtils::HookEntry<decltype(Proxy_Com_Printf)>,
	HookUtils::HookEntry<decltype(Proxy_MSG_ReadBits)>,
	HookUtils::HookEntry<decltype(Proxy_Cmd_TokenizeString)>, // HookEntry<decltype(&Proxy_Info_Print)>,
	HookUtils::HookEntry<decltype(Proxy_SV_SendMessageToClient)>,
	HookUtils::HookEntry<decltype(Proxy_SV_CalcPings)>, // HookEntry<decltype(&Proxy_SV_Status_f)>, HookEntry<decltype(&Proxy_Com_WriteConfig_f)>,
	HookUtils::HookEntry<decltype(Proxy_SV_SvEntityForGentity)>,
	HookUtils::HookEntry<decltype(Proxy_SV_SendClientGameState)>, // HookEntry<decltype(&Proxy_SV_UserinfoChanged)>, HookEntry<decltype(&Proxy_SV_BeginDownload_f)>, HookEntry<decltype(&Proxy_SV_DoneDownload_f)>,
	HookUtils::HookEntry<decltype(Proxy_SV_ExecuteClientMessage)>, // HookEntry<decltype(&Proxy_SV_WriteDownloadToClient)>,
	HookUtils::HookEntry<decltype(Proxy_SV_ExecuteClientCommand)>
> hookEntries[] = {
	HookUtils::HookEntry("Com_Printf", (unsigned char*)func_Com_Printf_addr[PROXY_PLATFORM], &Original_Com_Printf, &Proxy_Com_Printf),
	HookUtils::HookEntry("MSG_ReadBits", (unsigned char*)func_MSG_ReadBits_addr[PROXY_PLATFORM], &Original_MSG_ReadBits, &Proxy_MSG_ReadBits),
	HookUtils::HookEntry("Cmd_TokenizeString", (unsigned char*)func_Cmd_TokenizeString_addr[PROXY_PLATFORM], &Original_Cmd_TokenizeString, &Proxy_Cmd_TokenizeString),
	HookUtils::HookEntry("Com_WriteConfig_f", (unsigned char*)func_Com_WriteConfig_f_addr[PROXY_PLATFORM], &Original_Com_WriteConfig_f, &Proxy_Com_WriteConfig_f),
	HookUtils::HookEntry("Info_Print", (unsigned char*)func_Info_Print_addr[PROXY_PLATFORM], &Original_Info_Print, &Proxy_Info_Print),
	HookUtils::HookEntry("SV_SendMessageToClient", (unsigned char*)func_SV_SendMessageToClient_addr[PROXY_PLATFORM], &Original_SV_SendMessageToClient, &Proxy_SV_SendMessageToClient),
	HookUtils::HookEntry("SV_CalcPings", (unsigned char*)func_SV_CalcPings_addr[PROXY_PLATFORM], &Original_SV_CalcPings, &Proxy_SV_CalcPings),
	HookUtils::HookEntry("SV_SvEntityForGentity", (unsigned char*)func_SV_SvEntityForGentity_addr[PROXY_PLATFORM], &Original_SV_SvEntityForGentity, &Proxy_SV_SvEntityForGentity),
	HookUtils::HookEntry("SV_SendClientGameState", (unsigned char*)func_SV_SendClientGameState_addr[PROXY_PLATFORM], &Original_SV_SendClientGameState, &Proxy_SV_SendClientGameState),
	HookUtils::HookEntry("SV_ExecuteClientMessage", (unsigned char*)func_SV_ExecuteClientMessage_addr[PROXY_PLATFORM], &Original_SV_ExecuteClientMessage, &Proxy_SV_ExecuteClientMessage),
	HookUtils::HookEntry("SV_UserinfoChanged", (unsigned char*)func_SV_UserinfoChanged_addr[PROXY_PLATFORM], &Original_SV_UserinfoChanged, &Proxy_SV_UserinfoChanged),
	HookUtils::HookEntry("SV_Status_f", (unsigned char*)func_SV_Status_f_addr[PROXY_PLATFORM], &Original_SV_Status_f, &Proxy_SV_Status_f),
	HookUtils::HookEntry("SV_BeginDownload_f", (unsigned char*)func_SV_BeginDownload_f_addr[PROXY_PLATFORM], &Original_SV_BeginDownload_f, &Proxy_SV_BeginDownload_f),
	HookUtils::HookEntry("SV_DoneDownload_f", (unsigned char*)func_SV_DoneDownload_f_addr[PROXY_PLATFORM], &Original_SV_DoneDownload_f, &Proxy_SV_DoneDownload_f),
	HookUtils::HookEntry("SV_ExecuteClientCommand", (unsigned char*)func_SV_ExecuteClientCommand_addr[PROXY_PLATFORM], &Original_SV_ExecuteClientCommand, &Proxy_SV_ExecuteClientCommand),
	HookUtils::HookEntry("SV_WriteDownloadToClient", (unsigned char*)func_SV_WriteDownloadToClient_addr[PROXY_PLATFORM], &Original_SV_WriteDownloadToClient, &Proxy_SV_WriteDownloadToClient)
};

// ==================================================
// SV_PacketEvent filter stub
// --------------------------------------------------
// The engine unloads the proxy on every map change, and
// an rcon "map" runs from inside the packet code. So no
// proxy frame may be on the stack while the original
// packet code runs: SV_PacketEvent jumps to this stub,
// which calls the filter, then either returns (drop) or
// jumps to the trampoline. x86 cdecl only, checked on
// jampDed.exe and linuxjampded: [esp] return address,
// [esp+4] netadr_t (20 bytes), [esp+24] msg.
// ==================================================

// The filter gets its own copy of the arguments: under cdecl a callee may overwrite its argument slots,
// and here those slots are the engine's return address and the arguments the trampoline reads next
// The stub hardcodes 24 bytes of arguments (6 pushes, add esp 24, [esp+24]): netadr_t (20) + msg (4)
// The filter is compiled with the SDK netadr_t: if its size changes, the filter would read msg at the wrong place
static_assert(sizeof(netadr_t) == 20, "the stub copies 24 bytes of arguments");

// Size of the stub in bytes: 6 * 4 (push) + 5 (call) + 3 (add) + 2 (test) + 2 (jnz) + 1 (ret) + 5 (jmp)
// Also gives the allocation size and the offset of the jmp rel32 written after Attach
static constexpr std::size_t filterStubLength = 42;
// Target of the jmp written at the entry of SV_PacketEvent, freed in GAME_SHUTDOWN. nullptr: not attached
static unsigned char* packetEventFilterStub = nullptr;

// Not in hookEntries: its target is the stub, only known at runtime, and it needs steps the generic loop doesn't do
// (build the stub before Attach, write the jmp rel32 after, free the stub after Detach)
// If the stub can't be allocated, this hook is not attached at all, instead of a jmp to nullptr
static HookUtils::HookEntry<void(netadr_t, msg_t*)> packetEventHookEntry("SV_PacketEvent", (unsigned char*)func_SV_PacketEvent_addr[PROXY_PLATFORM], &Original_SV_PacketEvent, nullptr);

// The last rel32 (jmp trampoline) is written once Attach has created the trampoline
static unsigned char* Proxy_MakeFilterStub(void* filter)
{
	unsigned char* stub = HookUtils::AllocateMemory(filterStubLength);

	if (!stub)
	{
		return nullptr;
	}

	unsigned char* p = stub;

	for (int i = 0; i < 6; ++i)
	{
		*p++ = 0xFF; *p++ = 0x74; *p++ = 0x24; *p++ = 0x18;                               // push dword [esp+24]: copies msg, then from, last dword first
	}

	*p++ = 0xE8; *(int32_t*)p = (int32_t)((uintptr_t)filter - (uintptr_t)(p + 4)); p += 4;  // call filter
	*p++ = 0x83; *p++ = 0xC4; *p++ = 0x18;                                                // add esp, 24
	*p++ = 0x85; *p++ = 0xC0;                                                             // test eax, eax
	*p++ = 0x75; *p++ = 0x01;                                                             // jnz +1
	*p++ = 0xC3;                                                                          // ret: drop the packet
	*p++ = 0xE9; *(int32_t*)p = 0;                                                        // jmp trampoline

	return stub;
}

static void Proxy_Attach_PacketEventFilter(void)
{
	packetEventFilterStub = Proxy_MakeFilterStub((void*)&Proxy_SV_PacketEvent_Filter);

	if (!packetEventFilterStub)
	{
		Proxy_FatalHookError(packetEventHookEntry.hookName, HookUtils::Status::AllocationFailed);
	}

	packetEventHookEntry.proxyFunctionPtr = (decltype(packetEventHookEntry.proxyFunctionPtr))packetEventFilterStub;

	const HookUtils::Status status = HookUtils::Attach(packetEventHookEntry);

	if (status != HookUtils::Status::Ok)
	{
		Proxy_FatalHookError(packetEventHookEntry.hookName, status);
	}

	// The jmp rel32 is the last 4 bytes of the stub. It can only be written now that Attach created the trampoline
	// The server is single-threaded: no packet can reach the stub before this write
	unsigned char* jmpRel32 = packetEventFilterStub + filterStubLength - 4;

	*(int32_t*)jmpRel32 = (int32_t)((uintptr_t)Original_SV_PacketEvent - (uintptr_t)(jmpRel32 + 4));
}

// Safe during GAME_SHUTDOWN: neither the stub nor the trampoline leaves a return address on the stack
static void Proxy_Detach_PacketEventFilter(void)
{
	if (!packetEventFilterStub)
	{
		return;
	}

	const HookUtils::Status status = HookUtils::Detach(packetEventHookEntry);

	if (status != HookUtils::Status::Ok)
	{
		proxy.trap->Print("----- Proxy: Could not unhook '%s': %s\n", packetEventHookEntry.hookName, HookUtils::StatusString(status));
	}

	HookUtils::ReleaseMemory(packetEventFilterStub, filterStubLength);
	packetEventFilterStub = nullptr;
}

// ==================================================
// Proxy_Patch_Attach
// --------------------------------------------------
// Attach all the engine functions, shouldn't be very
// long without these detours. This allows me to detour
// calls into engine functions so I can add additional
// options, features or fixes.
// ==================================================

void Proxy_Engine_Attach_Patches(void)
{
	for (auto& hookEntry : hookEntries) {
		std::visit([](auto& hookEntryVisitor) {
			const HookUtils::Status status = HookUtils::Attach(hookEntryVisitor);

			if (status != HookUtils::Status::Ok)
			{
				Proxy_FatalHookError(hookEntryVisitor.hookName, status);
			}
		}, hookEntry);
	}

	Proxy_Attach_PacketEventFilter();
}

// ==================================================
// Proxy_Patch_Detach
// --------------------------------------------------
// We are closing and we should remove the detour,
// otherwise the jump might be done to an undefined
// function and we can't restore the original bytes
// anymore.
// ==================================================

void Proxy_Engine_Detach_Patches(void)
{
	for (auto& hookEntry : hookEntries) {
		std::visit([](auto& hookEntryVisitor) {
			const HookUtils::Status status = HookUtils::Detach(hookEntryVisitor);

			if (status != HookUtils::Status::Ok)
			{
				proxy.trap->Print("----- Proxy: Could not unhook '%s': %s\n", hookEntryVisitor.hookName, HookUtils::StatusString(status));
			}
		}, hookEntry);
	}

	Proxy_Detach_PacketEventFilter();
}

// ==================================================
// Proxy_Inline_Patch
// --------------------------------------------------
// Patches that doesn't require to be unpatched.
// It is not a problem to re-execute those patches.
// ==================================================

void Proxy_Engine_Inline_Patches(void)
{
	// Remove all the timer block in SVC_RemoteCommand, fix rcon disabler
	if (!HookUtils::Patch_NOP_Bytes((unsigned char*)func_SVC_RemoteCommand_timer_start_block_addr[PROXY_PLATFORM], func_SVC_RemoteCommand_timer_NOP_amount[PROXY_PLATFORM]))
	{
		Proxy_FatalHookError("SVC_RemoteCommand timer", HookUtils::Status::ProtectionFailed);
	}
	// When calling Com_BeginRedirect in SVC_RemoteCommand, change the length of the buffer from 49136 to 1008
	// It fix truncated text from SVC_RemoteCommand with the new Com_Printf
	if (!HookUtils::Patch((unsigned char*)func_SVC_RemoteCommand_Com_BeginRedirect_SV_OUTPUTBUF_LENGTH_addr[PROXY_PLATFORM], 0x03))
	{
		Proxy_FatalHookError("SVC_RemoteCommand Com_BeginRedirect", HookUtils::Status::ProtectionFailed);
	}
	// "jbe" to "jb": a userinfo that reaches exactly 1024 characters (or a big info string, 8192) must be refused, not
	// written with its NUL one byte past the buffer. The connect packet gets there through SV_DirectConnect's "ip" key
	if (!HookUtils::Patch((unsigned char*)func_Info_SetValueForKey_length_jbe_addr[PROXY_PLATFORM], 0x72))
	{
		Proxy_FatalHookError("Info_SetValueForKey length", HookUtils::Status::ProtectionFailed);
	}

	if (!HookUtils::Patch((unsigned char*)func_Info_SetValueForKey_Big_length_jbe_addr[PROXY_PLATFORM], 0x72))
	{
		Proxy_FatalHookError("Info_SetValueForKey_Big length", HookUtils::Status::ProtectionFailed);
	}
}