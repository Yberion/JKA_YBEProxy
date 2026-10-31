#pragma once

#include "sdk/server/server.hpp"

namespace ybeproxy::engine::rate
{

// sv_minRate, sv_minSnaps and sv_maxSnaps, as in ioq3, Quake3e, OpenJK and JK2MV (see Rate.cpp)
void Apply(client_t* cl);
// Applies a change of sv_minRate, sv_minSnaps, sv_maxSnaps or sv_fps to the clients already connected
void SettingsChanged(void);
// The rate the engine applies to the client: cl->rate, capped by sv_maxRate
int Applied(const client_t* cl);

} // namespace ybeproxy::engine::rate
