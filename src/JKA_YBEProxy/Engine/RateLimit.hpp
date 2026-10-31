#pragma once

#include "sdk/sys/sys_public.hpp"

namespace ybeproxy::engine::rate_limit
{

// Out-of-band rate limiting, called from the SV_PacketEvent filter. All return false to drop the packet
// command: only used in the message printed with developer 1
bool AddressAllowed(const netadr_t& from, const char* command);
bool StatusAllowed(const netadr_t& from, const char* command);
bool RconAllowed(void);

} // namespace ybeproxy::engine::rate_limit
