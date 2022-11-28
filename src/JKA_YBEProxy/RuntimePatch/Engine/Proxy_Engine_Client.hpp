#pragma once

#include "sdk/qcommon/q_shared.hpp"
#include "sdk/server/server.hpp"

#include <cstddef>

void Proxy_Engine_Client_UpdateUcmdStats(std::size_t clientNum, usercmd_t* cmd, std::size_t packetIndex);
void Proxy_Engine_Client_CalcPacketsAndFPS(int clientNum, int* packets, int* fps);
void Proxy_Engine_Client_UpdateTimenudge(client_t* client, usercmd_t* cmd, int _Milliseconds);