#pragma once

#include "sdk/qcommon/qcommon.hpp"
#include "sdk/server/server.hpp"

extern void (*Original_SV_SendMessageToClient)(msg_t*, client_t*);
void Proxy_SV_SendMessageToClient(msg_t* msg, client_t* client);