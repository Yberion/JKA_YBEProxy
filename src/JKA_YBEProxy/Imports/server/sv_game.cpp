#include "JKA_YBEProxy/Proxy_Header.hpp"
#include "sv_game.hpp"

// playerState_t *SV_GameClientNum( int num )
playerState_t* Proxy_GetPlayerStateByClientNum(int clientNum)
{
	return (playerState_t*)((byte*)proxy.locatedGameData.g_clients + proxy.locatedGameData.g_clientSize * (clientNum));
}