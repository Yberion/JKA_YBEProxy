#include "Proxy_Header.h"
#include "server/server.h"

/*
==================
Proxy_ClientCommand_NetStatus

Display net settings of all players
==================
*/
void Proxy_ClientCommand_NetStatus(int clientNum)
{
	if (proxy.clientData[clientNum].lastTimeNetStatus + 500 > proxy.server.svs->time)
	{
		return;
	}

	char			status[4096];
	int				i;
	client_t* cl;
	playerState_t* ps;
	int				ping;
	char			state[32];

	status[0] = 0;

	//Q_strcat(status, sizeof(status), "cl score ping rate  fps packets timeNudge timeNudge2 name \n");
	Q_strcat(status, sizeof(status), "score ping rate   fps packets timeNudge snaps id name \n");
	Q_strcat(status, sizeof(status), "----- ---- ------ --- ------- --------- ----- -- ---------------\n");

	for (i = 0, cl = proxy.server.svs->clients; i < proxy.server.cvars.sv_maxclients->integer; i++, cl++)
	{
		int			fps = 0;
		int			packets = 0;
		int			snaps = 0;

		if (!cl->state)
			continue;

		if (cl->state == CS_CONNECTED)
			Q_strncpyz(state, "CON ", sizeof(state));
		else if (cl->state == CS_ZOMBIE)
			Q_strncpyz(state, "ZMB ", sizeof(state));
		else
		{
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Com_sprintf(state, sizeof(state), "%4i", ping);
		}

		ps = Proxy_GetPlayerStateByClientNum(i);

		// If not a bot
		if (cl->ping >= 1)
		{
			Proxy_Server_CalcPacketsAndFPS(getClientNumFromAddr(cl), &packets, &fps);
		}

		if (1000 / cl->snapshotMsec > proxy.server.cvars.sv_fps->integer)
		{
			snaps = proxy.server.cvars.sv_fps->integer;
		}
		else
		{
			snaps = 1000 / cl->snapshotMsec;
		}

		// No need for truncation "feature" if we move name to end
		Q_strcat(status, sizeof(status), va("%5i %s %6i %3i %7i %9i %5i %2i %s^7\n", ps->persistant[PERS_SCORE], state, cl->rate, fps, packets, proxy.clientData[getClientNumFromAddr(cl)].timenudge, snaps, i, cl->name));
	}

	proxy.clientData[clientNum].lastTimeNetStatus = proxy.server.svs->time;

	char buffer[1012] = { 0 };
	int statusLength = strlen(status);
	int currentProgress = 0;

	while (currentProgress < statusLength)
	{
		Q_strncpyz(buffer, &status[currentProgress], sizeof(buffer));

		if (strlen(buffer) < 1012)
		{
			Q_strcat(buffer, sizeof(buffer), "\n");
		}

		proxy.trap->SendServerCommand(clientNum, va("print \"%s", buffer));
		currentProgress += strlen(buffer);
	}
}