#pragma once

// ==================================================
// INCLUDE
// ==================================================

#include "JKA_YBEProxy/Proxy_Header.h"
#include "server/server.h"

// ==================================================
// DEFINE
// ==================================================

#define func_SV_CalcPings_addr (( isWindows ) ? 0x444220 : 0x8057204)

#define var_svs_addr (( isWindows ) ? 0x606218 : 0x83121e0)

// ==================================================
// STRUCTS
// ==================================================

typedef struct ProxyServer_s
{
	serverStatic_t* svs;
} ProxyServer_t;

// ==================================================
// EXTERN VARIABLE
// ==================================================

extern ProxyServer_t proxyServer;
extern int isWindows;

// ==================================================
// FUNCTION
// ==================================================

void Proxy_Initialize_Server_MemoryAddress(void);

void (*Original_SV_CalcPings)(void);
void Proxy_SV_CalcPings(void);

void (*Original_SV_SendMessageToClient)(msg_t*, client_t*);
void Proxy_SV_SendMessageToClient(msg_t* msg, client_t* client);