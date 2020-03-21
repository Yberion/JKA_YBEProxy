#include "Proxy_Header.hpp"
#include "server/server.hpp"

#include <algorithm>
#include <iostream>

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

/*
==================
Proxy_ClientCommand_MyRatio
==================
*/
static void ratioString(int kill, int death, int suicides, char* ratioString, int sizeRatioString)
{
	if (kill - death >= 0)
	{
		Com_sprintf(ratioString, sizeRatioString, "(" S_COLOR_GREEN "+%i" S_COLOR_WHITE ")", kill - death);
	}
	else
	{
		Com_sprintf(ratioString, sizeRatioString, "(" S_COLOR_RED "%i" S_COLOR_WHITE ")", kill - (death - suicides));
	}
}

static float calcRatio(int kill, int death)
{
	if (kill == 0 && death == 0)
	{
		return 1.00;
	}
	else if (kill < 1 && death >= 1)
	{
		return 0.00;
	}
	else if (kill >= 1 && death <= 1)
	{
		return (float)kill;
	}
	else
	{
		return (float)kill / (float)death;
	}
}

void Proxy_ClientCommand_MyRatio(int clientNum)
{
	if (proxy.clientData[clientNum].lastTimeMyratioCheck + 1000 > proxy.server.svs->time)
	{
		return;
	}

	proxy.clientData[clientNum].lastTimeMyratioCheck = proxy.server.svs->time;

	char ratioStringBuffer[16];
	playerState_t* ps;

	ps = Proxy_GetPlayerStateByClientNum(clientNum);

	float ratio = calcRatio(ps->persistant[PERS_SCORE], ps->persistant[PERS_KILLED]);

	ratioString(ps->persistant[PERS_SCORE], ps->persistant[PERS_KILLED], ps->fd.suicides, ratioStringBuffer, sizeof(ratioStringBuffer));

	if (proxy.server.cvars.sv_gametype->integer != GT_DUEL && proxy.server.cvars.sv_gametype->integer != GT_TEAM && proxy.server.cvars.sv_gametype->integer != GT_FFA)
	{
		proxy.trap->SendServerCommand(clientNum, "print \"Command not supported for this gametype\n\"");

		return;
	}

	proxy.trap->SendServerCommand(clientNum, va("print \"Kills" S_COLOR_BLUE ": " S_COLOR_WHITE "%i " S_COLOR_BLUE "| "
		S_COLOR_WHITE "Deaths" S_COLOR_BLUE ": " S_COLOR_WHITE "%i " S_COLOR_BLUE "(" S_COLOR_WHITE "Suicides" S_COLOR_BLUE ": " S_COLOR_WHITE "%i" S_COLOR_BLUE ") " S_COLOR_BLUE "| "
		S_COLOR_WHITE "Ratio" S_COLOR_BLUE ": " S_COLOR_WHITE "%.2f %s\n\"",
		ps->persistant[PERS_SCORE], ps->persistant[PERS_KILLED],
		ps->fd.suicides,
		ratio, ratioStringBuffer));
}