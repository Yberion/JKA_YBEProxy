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

extern svEntity_t* (*Original_SV_SvEntityForGentity)(sharedEntity_t*);
svEntity_t* Proxy_SV_SvEntityForGentity(sharedEntity_t* gEnt);

extern void (*Original_SV_ExecuteClientMessage)(client_t*, msg_t*);
void Proxy_SV_ExecuteClientMessage(client_t* cl, msg_t* msg);

extern void (QDECL* Original_Com_Printf)(const char*, ...);
void QDECL Proxy_Com_Printf(const char* fmt, ...);

extern void (*Original_SV_Status_f)(void);
void Proxy_SV_Status_f(void);

extern void (*Original_SV_SendClientGameState)(client_t*);
void Proxy_SV_SendClientGameState(client_t* client);

extern void (*Original_SVC_Status)(netadr_t);
void Proxy_SVC_Status(netadr_t from);

extern void (*Original_SVC_Info)(netadr_t);
void Proxy_SVC_Info(netadr_t from);

extern void (*Original_SVC_RemoteCommand)(netadr_t, msg_t*);
void Proxy_SVC_RemoteCommand(netadr_t from, msg_t* msg);

extern void (*Original_SV_ConnectionlessPacket)(netadr_t, msg_t*);
void Proxy_SV_ConnectionlessPacket(netadr_t from, msg_t* msg);

extern void (*Original_SV_PacketEvent)(netadr_t, msg_t*);
void Proxy_SV_PacketEvent(netadr_t from, msg_t* msg);

extern void (*Original_Cmd_TokenizeString)(const char*);
void Proxy_Cmd_TokenizeString(const char* text_in);

extern void (*Original_SV_UserinfoChanged)(client_t*);
void Proxy_SV_UserinfoChanged(client_t* cl);

extern void (*Original_SV_BeginDownload_f)(client_t*);
void Proxy_SV_BeginDownload_f(client_t* cl);

extern void (*Original_SV_DoneDownload_f)(client_t*);
void Proxy_SV_DoneDownload_f(client_t* cl);

extern void (*Original_SV_ExecuteClientCommand)(client_t*, const char*, qboolean);
void Proxy_SV_ExecuteClientCommand(client_t* cl, const char* s, qboolean clientOK);

extern void (*Original_SV_WriteDownloadToClient)(client_t*, msg_t*);
void Proxy_SV_WriteDownloadToClient(client_t* cl, msg_t* msg);

// ------------- common

const char* Proxy_FS_GetCurrentGameDir(bool emptybase = false);
void Proxy_Cmd_TokenizeStringIgnoreQuotes(const char*);
char* Proxy_Cmd_ArgsFrom(int arg);
char* Proxy_Cmd_Cmd(void);
void Proxy_Windows_Com_EndRedirect(void);
