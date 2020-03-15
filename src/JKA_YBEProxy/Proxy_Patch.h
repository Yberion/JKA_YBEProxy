#pragma once

// WIP
unsigned char* pSV_SendMessageToClient;
unsigned char* pSV_UserMove;
unsigned char* pSV_CalcPings;

// ==================================================
// FUNCTION
// ==================================================

void* Proxy_EnginePatch_PingFix_SV_SendMessageToClient(void);
void* Proxy_EnginePatch_PingFix_SV_UserMove(void);