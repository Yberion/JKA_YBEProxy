#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_ClientCommand.hpp"
#include "JKA_YBEProxy/Proxy_Header.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Wrappers.hpp"
#include "JKA_YBEProxy/Imports/server/sv_game.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_NetStatus.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Rate.hpp"

/*
==================
Proxy_ClientCommand_NetStatus

Display net settings of all players
==================
*/
void Proxy_Engine_ClientCommand_NetStatus(int clientNum)
{
	ClientData_t *currentClientData = &proxy.clientData[clientNum];

	if (currentClientData->lastTimeNetStatus + 500 > server.svs->time)
	{
		return;
	}

	char			status[8192];
	int				i;
	client_t* 		cl;
	playerState_t* 	ps;
	char			state[32];

	status[0] = 0;

	Q_strcat(status, sizeof(status), "score gamePing netPing userRate   rate trueRate  fps packets timeNudge snaps trueSnaps id name \n");
	Q_strcat(status, sizeof(status), "----- -------- ------- -------- ------ -------- ---- ------- --------- ----- --------- -- ---------------\n");

	for (i = 0, cl = server.svs->clients; i < server.cvars.sv_maxclients->integer; i++, cl++)
	{
		NetRates_t	rates = NetRates_t();

		if (!cl->state)
			continue;

		if (cl->state == CS_CONNECTED)
			Q_strncpyz(state, "     CON", sizeof(state));
		else if (cl->state == CS_ZOMBIE)
			Q_strncpyz(state, "     ZMB", sizeof(state));
		else if (cl->netchan.remoteAddress.type == NA_BOT)
			Q_strncpyz(state, "     BOT", sizeof(state));
		else
			Com_sprintf(state, sizeof(state), "%8i", cl->ping < 9999 ? cl->ping : 9999);

		ps = Proxy_GetPlayerStateByClientNum(i);

		// A bot's snapshotMsec can be 0, which is bad for "1000 / snapshotMsec"
		// Read-only: writing cl->snapshotMsec changed the real snapshot rate of a human with a ping of 0
		const int snapshotMsec = cl->snapshotMsec > 0 ? cl->snapshotMsec : 1;

		// The snaps the client asked for, limited by sv_fps. "trueSnaps" below is what the server sent in the last second:
		// the server sends on its frames, so a request between two frame rates gets the lower one, and a client that
		// is short of rate gets less
		const int snaps = (1000 / snapshotMsec > server.cvars.sv_fps->integer) ? server.cvars.sv_fps->integer : 1000 / snapshotMsec;

		// The numbers of a bot stay 0: it has no packets, and its snapshots go nowhere
		// A ping of 0 is not a bot: a human whose round trip is shorter than one server frame also shows 0
		char netPing[16] = "BOT";

		if (cl->netchan.remoteAddress.type != NA_BOT)
		{
			Proxy_Engine_NetStatus_GetRates(cl, &rates);
			Com_sprintf(netPing, sizeof(netPing), "%i", proxy.clientData[i].netPing);
		}

		// "-" until measured, and for a client with too few snapshots to tell
		char timeNudge[16] = "-";

		if (proxy.clientData[i].timenudgeKnown)
		{
			Com_sprintf(timeNudge, sizeof(timeNudge), "%i", proxy.clientData[i].timenudge);
		}

		// The rate the client asked for, the one the server applies (sv_minRate, sv_maxRate), and "trueRate": the bytes
		// sent in the last second. The rate is a limit, trueRate reaches it only when the client is short of rate
		const int userRate = atoi(Info_ValueForKey(cl->userinfo, "rate"));

		// No need for truncation "feature" if we move name to the end
		Q_strcat(status, sizeof(status), va("%5i %s %7s %8i %6i %8i %4i %7i %9s %5i %9i %2i %s^7\n", ps->persistant[PERS_SCORE], state, netPing, userRate, Proxy_Engine_Rate_Applied(cl), rates.rate, rates.fps, rates.packets, timeNudge, snaps, rates.snaps, i, cl->name));
	}

	currentClientData->lastTimeNetStatus = server.svs->time;

	char buffer[1012] = { 0 };
	std::size_t statusLength = std::strlen(status);
	std::size_t currentProgress = 0;

	while (currentProgress < statusLength)
	{
		Q_strncpyz(buffer, &status[currentProgress], sizeof(buffer));

		if (std::strlen(buffer) < 1012)
		{
			Q_strcat(buffer, sizeof(buffer), "\n");
		}

		proxy.trap->SendServerCommand(clientNum, va("print \"%s", buffer));
		currentProgress += std::strlen(buffer);
	}
}