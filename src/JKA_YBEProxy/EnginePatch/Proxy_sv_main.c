#include "Proxy_EnginePatch.h"

/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
void Proxy_SV_CalcPings(void)
{

	int			i, j;
	client_t* cl;
	int			total, count;
	int			delta;
	playerState_t* ps;

	for (i = 0; i < proxy.server.cvars.sv_maxclients->integer; i++)
	{
		cl = &proxy.server.svs->clients[i];
		if (cl->state != CS_ACTIVE)
		{
			cl->ping = 999;
			continue;
		}
		if (!cl->gentity)
		{
			cl->ping = 999;
			continue;
		}
		if (cl->gentity->r.svFlags & SVF_BOT)
		{
			cl->ping = 0;
			continue;
		}

		total = 0;
		count = 0;

		for (j = 0; j < PACKET_BACKUP; j++)
		{
			// Proxy -------------->
			// if (cl->frames[j].messageAcked <= 0)
			if (cl->frames[j].messageAcked == -1)
			// Proxy <--------------
			{
				continue;
			}
			delta = cl->frames[j].messageAcked - cl->frames[j].messageSent;
			count++;
			total += delta;
		}
		if (!count)
		{
			cl->ping = 999;
		}
		else
		{
			cl->ping = total / count;
			
			if (cl->ping > 999)
			{
				cl->ping = 999;
			}

			// Proxy -------------->
			if (cl->ping < 1)
			{
				cl->ping = 1;
			}
			// Proxy <--------------
		}

		// let the game dll know about the ping
		// Proxy -------------->
		// ps = SV_GameClientNum( i );
		ps = Proxy_GetPlayerStateByClientNum(i);
		// Proxy <--------------
		ps->ping = cl->ping;
	}
}