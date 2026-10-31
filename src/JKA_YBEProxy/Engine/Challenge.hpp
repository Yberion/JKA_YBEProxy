#pragma once

#include "sdk/sys/sys_public.hpp"

namespace ybeproxy::engine::challenge
{

// Called from the SV_PacketEvent filter before the original answers a getchallenge (see Challenge.cpp)
void Renew(const netadr_t& from);

} // namespace ybeproxy::engine::challenge
