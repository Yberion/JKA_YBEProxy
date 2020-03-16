#pragma once

// ==================================================
// DEFINE
// ==================================================

#if defined(_WIN32) && !defined(MINGW32)
	#define chunck_PingFix_SV_SendMessageToClient_addr 0x444E23
	#define chunck_PingFix_SV_SendMessageToClient_NextInstr_addr 0x444E29

	#define chunck_PingFix_SV_UserMove_addr 0x43C2FF
	#define chunck_PingFix_SV_UserMove_NextInstr_addr 0x43C306
#else
	#define chunck_PingFix_SV_SendMessageToClient_addr 0x8058cf1
	#define chunck_PingFix_SV_SendMessageToClient_NextInstr_addr 0x8058cf7

	#define chunck_PingFix_SV_UserMove_addr 0x804e7dd
	#define chunck_PingFix_SV_UserMove_NextInstr_addr 0x804e7e4
#endif

// ==================================================
// FUNCTION
// ==================================================

void* Proxy_EnginePatch_PingFix_SV_SendMessageToClient(void);
void* Proxy_EnginePatch_PingFix_SV_UserMove(void);