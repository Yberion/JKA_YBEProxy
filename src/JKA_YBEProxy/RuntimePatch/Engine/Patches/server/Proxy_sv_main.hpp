#pragma once

#include "sdk/sys/sys_public.hpp"
#include "sdk/qcommon/qcommon.hpp"

extern void (*Original_SV_CalcPings)(void);
void Proxy_SV_CalcPings(void);

extern void (*Original_SV_PacketEvent)(netadr_t, msg_t*);
qboolean QDECL Proxy_SV_PacketEvent_Filter(netadr_t from, msg_t* msg);
