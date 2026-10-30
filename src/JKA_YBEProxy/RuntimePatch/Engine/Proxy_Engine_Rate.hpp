#pragma once

#include "sdk/server/server.hpp"

// sv_minRate, sv_minSnaps and sv_maxSnaps, as in ioq3, Quake3e, OpenJK and JK2MV (see Proxy_Engine_Rate.cpp)
void Proxy_Engine_Rate_Apply(client_t* cl);
// Applies a change of sv_minRate, sv_minSnaps, sv_maxSnaps or sv_fps to the clients already connected
void Proxy_Engine_Rate_SettingsChanged(void);
// The rate the engine applies to the client: cl->rate, capped by sv_maxRate
int Proxy_Engine_Rate_Applied(const client_t* cl);
