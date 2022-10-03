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

extern void (*Original_SV_Status_f)(void);
void Proxy_SV_Status_f(void);

extern void (*Original_SV_SendClientGameState)(client_t*);
void Proxy_SV_SendClientGameState(client_t* client);

extern void (*Original_SVC_Status)(netadr_t);
void Proxy_SVC_Status(netadr_t from);

extern void (*Original_SVC_Info)(netadr_t);
void Proxy_SVC_Info(netadr_t from);

extern void (*Original_Cmd_TokenizeString)(const char* text_in);
void Proxy_Cmd_TokenizeString(const char* text_in);

extern void (*Original_SVC_RemoteCommand)(netadr_t from, msg_t* msg);
void Proxy_SVC_RemoteCommand(netadr_t from, msg_t* msg);

// ------------- common

const char* FS_GetCurrentGameDir(bool emptybase = false);
char* Cmd_ArgsFrom(int arg);
char* Cmd_Cmd(void);
void Windows_Com_EndRedirect(void);