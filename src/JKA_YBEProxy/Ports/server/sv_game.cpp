#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Ports/server/sv_game.hpp"

// playerState_t *SV_GameClientNum( int num )
playerState_t* Proxy_GetPlayerStateByClientNum(int clientNum)
{
	return (playerState_t*)((byte*)proxy.locatedGameData.g_clients + proxy.locatedGameData.g_clientSize * (clientNum));
}