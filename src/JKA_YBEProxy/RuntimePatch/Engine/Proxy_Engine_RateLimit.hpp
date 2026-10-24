#pragma once

#include "sdk/sys/sys_public.hpp"

// Out-of-band rate limiting (F6), called from the SV_PacketEvent filter. Both return false to drop the packet
// command: only used in the message printed with developer 1
bool Proxy_Engine_RateLimit_AddressAllowed(const netadr_t& from, const char* command);
bool Proxy_Engine_RateLimit_StatusAllowed(const netadr_t& from, const char* command);
bool Proxy_Engine_RateLimit_RconAllowed(void);
