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
	serverStatic_t* svs = (serverStatic_t*)0x606218;
	svs->clients[0].ping = 222;
	ps->ping = svs->clients[0].ping;

	//Original_SV_CalcPings();
}