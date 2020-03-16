#include "EnginePatch/Proxy_EnginePatch.h"
#include "DetourPatcher/DetourPatcher.h"
#include "Proxy_Shell.h"
#include "Proxy_Patch.h"
#include "Proxy_Server.h"

// WIP
unsigned char* pSV_SendMessageToClient;
unsigned char* pSV_UserMove;
unsigned char* pSV_CalcPings;

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
	pSV_SendMessageToClient =	Attach((unsigned char*)chunck_PingFix_SV_SendMessageToClient_addr, (unsigned char*)Proxy_EnginePatch_PingFix_SV_SendMessageToClient());
	// Windows ONLY for now, DON'T test this one on Linux
	pSV_UserMove =				Attach((unsigned char*)chunck_PingFix_SV_UserMove_addr, (unsigned char*)Proxy_EnginePatch_PingFix_SV_UserMove());

	//Original_SV_CalcPings = (void (*)(void))	Attach((unsigned char*)func_SV_CalcPings_addr, (unsigned char*)&Proxy_SV_CalcPings);
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
	Detach((unsigned char*)chunck_PingFix_SV_SendMessageToClient_addr, (unsigned char*)pSV_SendMessageToClient);
	Detach((unsigned char*)chunck_PingFix_SV_UserMove_addr, (unsigned char*)pSV_UserMove);
	//Detach((unsigned char*)func_SV_CalcPings_addr, (unsigned char*)Original_SV_CalcPings);
}

// ==================================================
// Proxy_EnginePatch_PingFix_SV_SendMessageToClient
// --------------------------------------------------
// Patch:
//
// client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = svs.time;
//
// TO
//
// client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = Sys_Milliseconds();
//
// ==================================================

void* Proxy_EnginePatch_PingFix_SV_SendMessageToClient(void)
{
	// client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = Sys_Milliseconds();
	__StartHook(Proxy_EnginePatch_PingFix_SV_SendMessageToClient);
	{
		__asm1__(push ecx)
		__asm1__(push eax)
		__asm1__(push 0x0)
		__asm2__(mov eax, func_Sys_Milliseconds_addr)
		__asm1__(call eax)
		__asm2__(add esp, 0x4)
		__asm2__(mov edx, eax)
		__asm1__(pop eax)
		__asm1__(pop ecx)
		__asm1__(push chunck_PingFix_SV_SendMessageToClient_NextInstr_addr)
		__asm1__(ret)
	}
	__EndHook(Proxy_EnginePatch_PingFix_SV_SendMessageToClient);

	// TESTING
	// client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = 12;
	// On linux: __asm2__(MOV EDX, 0xC) -> __asm2__(MOV EAX, 0xC)
	/*
	__StartHook(Proxy_EnginePatch_PingFix_SV_SendMessageToClient);
	{
		__asm2__(MOV EDX, 0xC)
		__asm1__(push chunck_PingFix_SV_SendMessageToClient_NextInstr_addr)
		__asm1__(ret)
	}
	__EndHook(Proxy_EnginePatch_PingFix_SV_SendMessageToClient);
	*/

	/*
	// Normal behavior
	// client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = svs.time;
	// On linux: __asm2__(MOV EDX, DWORD PTR DS:[60621C]) -> __asm2__(MOV EAX, DWORD PTR DS:[EDI + 0x39458])
	__StartHook(Proxy_EnginePatch_PingFix_SV_SendMessageToClient);
	{
		__asm2__(MOV EDX, DWORD PTR DS:[60621C])
		__asm1__(push chunck_PingFix_SV_SendMessageToClient_NextInstr_addr)
		__asm1__(ret)
	}
	__EndHook(Proxy_EnginePatch_PingFix_SV_SendMessageToClient);
	*/
}

// ==================================================
// Proxy_EnginePatch_PingFix_SV_UserMove
// --------------------------------------------------
// Patch:
//
// cl->frames[ cl->messageAcknowledge & PACKET_MASK ].messageAcked = svs.time;
//
// TO
//
// if (cl->frames[cl->messageAcknowledge & PACKET_MASK].messageAcked == -1)
//		cl->frames[ cl->messageAcknowledge & PACKET_MASK ].messageAcked = Sys_Milliseconds();
//
// ==================================================

void* Proxy_EnginePatch_PingFix_SV_UserMove(void)
{
	//	if (cl->frames[cl->messageAcknowledge & PACKET_MASK].messageAcked == -1)
	//		cl->frames[ cl->messageAcknowledge & PACKET_MASK ].messageAcked = Sys_Milliseconds();
	// Windows ONLY for now, DON'T test this one on Linux
	__StartHook(Proxy_EnginePatch_PingFix_SV_UserMove);
	{
		__asm2__(cmp dword ptr ds : [eax + ebx + 0x21574] , -1)
		__asm1__(jne exitUserMoveHook)

		__asm1__(push eax)
		__asm1__(push 0x0)
		__asm2__(mov eax, func_Sys_Milliseconds_addr)
		__asm1__(call eax)
		__asm2__(add esp, 0x4)
		__asm2__(mov ecx, eax)
		__asm1__(pop eax)
		__asm2__(mov dword ptr ds : [eax + ebx + 0x21574] , ecx)

		__asm1__(exitUserMoveHook:)
		__asm1__(push chunck_PingFix_SV_UserMove_NextInstr_addr)
		__asm1__(ret)
	}
	__EndHook(Proxy_EnginePatch_PingFix_SV_UserMove);
}
