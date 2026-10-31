#pragma once

#include "sdk/sys/sys_public.hpp"

namespace ybeproxy::engine::challenge
{

// Called from the SV_PacketEvent filter instead of the original SV_GetChallenge (see Challenge.cpp)
void Answer(const netadr_t& from);
// Called from the SV_DirectConnect hook before the original looks the challenge up in svs.challenges
void Admit(const netadr_t& from);

} // namespace ybeproxy::engine::challenge
