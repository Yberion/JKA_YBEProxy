#include "Proxy_EnginePatch.h"

/*
0x4580E0 = Sys_Milliseconds() : int (*Milliseconds)(void)
00444220 = SV_CalcPings() : void (*SV_CalcPings)(void)
0x40FBE0 = Com_Printf() : void (*bla)(const char* msg, ...)
*/

// ==================================================
// Proxy_EnginePatch_Attach
// --------------------------------------------------
// Attach all the engine functions, shouldn't be very
// long without these detours. This allows me to detour
// calls into engine functions so I can add additional
// options, features or fixes.
// ==================================================

void Proxy_EnginePatch_Attach(void)
{
	pSV_SendMessageToClient =	Attach((unsigned char*)0x444E23, (unsigned char*)Proxy_EnginePatch_PingFix_SV_SendMessageToClient());
	// 0x43C2FF for the assignement when using the one with the if
	pSV_UserMove =				Attach((unsigned char*)0x43C2F0, (unsigned char*)Proxy_EnginePatch_PingFix_SV_UserMove());
	// 0x4442AC = 004442AC  |> 8B91 580C0000  ||MOV EDX,DWORD PTR DS:[ECX+C58]
	pSV_CalcPings =				Attach((unsigned char*)0x4442AC, (unsigned char*)Proxy_EnginePatch_DisplaySnapShots_SV_CalcPings());
}

// ==================================================
// Proxy_EnginePatch_Detach
// --------------------------------------------------
// We are closing and we should remove the detour,
// otherwise the jump might be done to an undefined
// function and we can't restore the original bytes
// anymore.
// ==================================================

void Proxy_EnginePatch_Detach(void)
{
	Detach((unsigned char*)0x444E23, (unsigned char*)pSV_SendMessageToClient);
	Detach((unsigned char*)0x43C2F0, (unsigned char*)pSV_UserMove);
	Detach((unsigned char*)0x4442AC, (unsigned char*)pSV_CalcPings);
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

	// Normal behavior
	// client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = svs.time;
	__sh_Prologue;
	{
		__asm2__(MOV EDX, DWORD PTR DS : [0x60621C])
		__asm1__(push 0x444E29)
		__asm1__(retn)
	}
	__sh_Epilogue;
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
	/*
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
	*/

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
	
	// Normal behavior
	// cl->frames[ cl->messageAcknowledge & PACKET_MASK ].messageAcked = svs.time;
	__sh_Prologue;
	{
		__asm2__(MOV ECX, DWORD PTR DS : [0x60621C])
		__asm1__(push 0x43C2F6)
		__asm1__(retn)
	}
	__sh_Epilogue;
}

void* Proxy_EnginePatch_DisplaySnapShots_SV_CalcPings(void)
{
	char* message = "messageAcked: %d     messageSent: %d\n";

	__sh_Prologue;
	{
		__asm1__(push eax)
		__asm2__(mov edx, DWORD PTR DS : [ecx + 0xC58])
		__asm1__(push dword ptr ds : [ecx + 0xC54])
		__asm1__(push dword ptr ds : [ecx + 0xC58])
		__asm2__(lea eax, message)
		__asm1__(push eax)
		__asm2__(mov eax, 0x40FBE0)
		__asm1__(call eax)
		__asm2__(add esp, 0xC)
		__asm1__(pop eax)
		__asm1__(push 0x4442B2)
		__asm1__(retn)
	}
	__sh_Epilogue;
}