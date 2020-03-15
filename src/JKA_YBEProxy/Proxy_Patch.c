#include "EnginePatch/Proxy_Server.h"
#include "DetourPatcher/DetourPatcher.h"
#include "Proxy_Shell.h"
#include "Proxy_Patch.h"

/*
0x4580E0 = Sys_Milliseconds() : int (*Sys_Milliseconds_ptr)(void)
0x444220 = SV_CalcPings() : void (*SV_CalcPings_ptr)(void)
0x40FBE0 = Com_Printf() : void (*Com_Printf_ptr)(const char* msg, ...)
*/

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
	// Normal: 0x444E23 ||| 0x444E32
	//pSV_SendMessageToClient =	Attach((unsigned char*)0x444E23, (unsigned char*)Proxy_EnginePatch_PingFix_SV_SendMessageToClient());
	// Normal: 0x43C2F0 ||| 0x43C2FF for the assignement when using the one with the if
	//pSV_UserMove =				Attach((unsigned char*)0x43C2FF, (unsigned char*)Proxy_EnginePatch_PingFix_SV_UserMove());

	Original_SV_CalcPings = (void (*)(void))	Attach((unsigned char*)0x444220, (unsigned char*)&Proxy_SV_CalcPings);
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
	//Detach((unsigned char*)0x444E23, (unsigned char*)pSV_SendMessageToClient);
	//Detach((unsigned char*)0x43C2FF, (unsigned char*)pSV_UserMove);
	Detach((unsigned char*)0x444220, (unsigned char*)Original_SV_CalcPings);
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
	/*
	// client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = Sys_Milliseconds();
	// Modify assignment
	__sh_Prologue;
	{
		__asm1__(push eax)
		__asm1__(push 0x0)
		__asm2__(mov eax, 0x4580E0)
		__asm1__(call eax)
		__asm2__(add esp, 0x4)
		__asm2__(mov edx, eax)
		__asm1__(pop eax)
		__asm2__(MOV DWORD PTR DS : [ECX + EBX + 21570] , EDX)
		__asm1__(push 0x444E39)
		__asm1__(retn)
	}
	__sh_Epilogue;
	*/

	/*
	// client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = Sys_Milliseconds();
	__sh_Prologue;
	{
		__asm1__(push eax)
		__asm1__(push 0x0)
		__asm2__(mov eax, 0x4580E0)
		__asm1__(call eax)
		__asm2__(add esp, 0x4)
		__asm2__(mov edx, eax)
		__asm1__(pop eax)
		__asm1__(push 0x444E29)
		__asm1__(retn)
	}
	__sh_Epilogue;
	*/

	// TESTING
	// client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = 12;
	__sh_Prologue;
	{
		__asm1__(push eax)
		__asm2__(MOV EDX, 0xC)
		__asm1__(pop eax)
		__asm1__(push 0x444E29)
		__asm1__(retn)
	}
	__sh_Epilogue;
	
	/*
	// Normal behavior
	// client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = svs.time;
	__sh_Prologue;
	{
		__asm2__(MOV EDX,DWORD PTR DS:[60621C])
		__asm1__(push 0x444E29)
		__asm1__(retn)
	}
	__sh_Epilogue;
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
	__sh_Prologue;
	{
		__asm2__(cmp dword ptr ds : [eax + ebx + 0x21574] , -1)
		__asm1__(jne exitUserMoveHook)

		__asm1__(push eax)
		__asm1__(push 0x0)
		__asm2__(mov eax, 0x4580E0)
		__asm1__(call eax)
		__asm2__(add esp, 0x4)
		__asm2__(mov ecx, eax)
		__asm1__(pop eax)
		__asm2__(mov dword ptr ds : [eax + ebx + 0x21574] , ecx)

		__asm1__(exitUserMoveHook:)
		__asm1__(push 0x43C306)
		__asm1__(retn)
	}
	__sh_Epilogue;
	

	/*
	// cl->frames[ cl->messageAcknowledge & PACKET_MASK ].messageAcked = Sys_Milliseconds();
	__sh_Prologue;
	{
		__asm1__(push eax)
		__asm1__(push 0x0)
		__asm2__(mov eax, 0x4580E0)
		__asm1__(call eax)
		__asm2__(add esp, 0x4)
		__asm2__(mov ecx, eax)
		__asm1__(pop eax)
		__asm1__(push 0x43C2F6)
		__asm1__(retn)
	}
	__sh_Epilogue;
	*/

	/*
	// Normal behavior
	// cl->frames[ cl->messageAcknowledge & PACKET_MASK ].messageAcked = svs.time;
	__sh_Prologue;
	{
		__asm2__(MOV ECX, DWORD PTR DS : [0x60621C])
		__asm1__(push 0x43C2F6)
		__asm1__(retn)
	}
	__sh_Epilogue;
	*/
}