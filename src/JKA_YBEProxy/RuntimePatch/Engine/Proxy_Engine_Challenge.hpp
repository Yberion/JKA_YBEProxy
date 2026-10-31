#pragma once

#include "sdk/sys/sys_public.hpp"

// Called from the SV_PacketEvent filter before the original answers a getchallenge (see Proxy_Engine_Challenge.cpp)
void Proxy_Engine_Challenge_Renew(const netadr_t& from);
