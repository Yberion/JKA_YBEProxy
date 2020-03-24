#pragma once

// ==================================================
// INCLUDE
// ==================================================

#include "JKA_YBEProxy/Proxy_Header.hpp"
#include "JKA_YBEProxy/Proxy_Shell.hpp"

// ==================================================
// DEFINE
// ==================================================

// ==================================================
// STRUCTS
// ==================================================



// ==================================================
// EXTERN VARIABLE
// ==================================================

// ==================================================
// FUNCTION
// ==================================================

extern void (*Original_SV_CalcPings)(void);
void Proxy_SV_CalcPings(void);

extern void (*Original_SV_SendMessageToClient)(msg_t*, client_t*);
void Proxy_SV_SendMessageToClient(msg_t* msg, client_t* client);

extern void (*Original_SV_UserMove)(client_t*, msg_t*, qboolean);
void Proxy_SV_UserMove(client_t* cl, msg_t* msg, qboolean delta);

extern svEntity_t* (*Original_SV_SvEntityForGentity)(sharedEntity_t*);
svEntity_t* Proxy_SV_SvEntityForGentity(sharedEntity_t* gEnt);

extern void (QDECL* Original_Common_Com_Printf)(const char*, ...);
void QDECL Proxy_Common_Com_Printf(const char* fmt, ...);