#pragma once

#include "sdk/server/server.hpp"
#include "sdk/qcommon/qcommon.hpp"
#include "sdk/qcommon/q_shared.hpp"

extern void (*Original_SV_ExecuteClientMessage)(client_t*, msg_t*);
void Proxy_SV_ExecuteClientMessage(client_t* cl, msg_t* msg);

extern void (*Original_SV_SendClientGameState)(client_t*);
void Proxy_SV_SendClientGameState(client_t* client);

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