#pragma once

#include "sdk/sys/sys_public.hpp"
#include "sdk/qcommon/qcommon.hpp"

extern void (*Original_SV_CalcPings)(void);
void Proxy_SV_CalcPings(void);

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