#pragma once

#include "JKA_YBEProxy/Proxy_Header.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Wrappers.hpp"
#include "sdk/server/server.hpp"

#include <cstddef>

inline std::size_t getClientNumFromAddr(client_t* client)
{
	return client - (*(client_t**)var_svsClients_addr[PROXY_PLATFORM]);
}