#pragma once

#include "sdk/server/server.hpp"

// sv_minRate, a floor to the rate of the clients, as in ioq3, Quake3e, OpenJK and JK2MV (see Proxy_Engine_Rate.cpp)
void Proxy_Engine_Rate_ApplyMinRate(client_t* cl);
// Applies a change of sv_minRate to the clients already connected
void Proxy_Engine_Rate_MinRateChanged(void);
// The rate the engine applies to the client: cl->rate, capped by sv_maxRate
int Proxy_Engine_Rate_Applied(const client_t* cl);
