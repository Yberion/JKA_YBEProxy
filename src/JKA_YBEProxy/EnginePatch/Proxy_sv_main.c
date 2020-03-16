#include "Proxy_Server.h"

/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
void Proxy_SV_CalcPings(void)
{
	playerState_t* ps = Proxy_GetPlayerStateByClientNum(0);
	proxyServer.svs->clients[0].ping = 222;
	ps->ping = proxyServer.svs->clients[0].ping;

	//Original_SV_CalcPings();
}