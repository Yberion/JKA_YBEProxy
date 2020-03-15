#pragma once

#include "JKA_YBEProxy/Proxy_Header.h"
#include "server/server.h"

// ==================================================
// sv_main
// ==================================================

void (*Original_SV_CalcPings)(void);
void Proxy_SV_CalcPings(void);