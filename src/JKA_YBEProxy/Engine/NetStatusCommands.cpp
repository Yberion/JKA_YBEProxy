#include "JKA_YBEProxy/Engine/NetStatusCommands.hpp"
#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Engine/EngineState.hpp"
#include "JKA_YBEProxy/Ports/server/sv_game.hpp"
#include "JKA_YBEProxy/Engine/NetStatus.hpp"
#include "JKA_YBEProxy/Engine/Rate.hpp"

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

	Q_strcat(status, sizeof(status), "score gamePing netPing userRate   rate trueRate  fps packets timeNudge userSnaps snaps trueSnaps id name \n");
	Q_strcat(status, sizeof(status), "----- -------- ------- -------- ------ -------- ---- ------- --------- --------- ----- --------- -- ---------------\n");

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

		// The snaps the server applies (sv_minSnaps, sv_maxSnaps), limited by sv_fps. "trueSnaps" below is what the
		// server sent in the last second: the server sends on its frames, so a value between two frame rates gets the
		// lower one, and a client that is short of rate gets less
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
		const int userSnaps = atoi(Info_ValueForKey(cl->userinfo, "snaps"));

		// No need for truncation "feature" if we move name to the end
		Q_strcat(status, sizeof(status), va("%5i %s %7s %8i %6i %8i %4i %7i %9s %9i %5i %9i %2i %s^7\n", ps->persistant[PERS_SCORE], state, netPing, userRate, Proxy_Engine_Rate_Applied(cl), rates.rate, rates.fps, rates.packets, timeNudge, userSnaps, snaps, rates.snaps, i, cl->name));
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

	proxy.trap->SendServerCommand(clientNum, "print \"Run /explainNetStatus for the meaning of each column.\n\"");
}

/*
==================
Proxy_ClientCommand_ExplainNetStatus

Explains each column of /netStatus to the player
==================
*/

// Two prints: a server command holds at most MAX_STRING_CHARS characters, "print" and the quotes included
static const char explainNetStatusColumns[] =
	"Columns of /netStatus:\n"
	"score      Score\n"
	"gamePing   Ping of the scoreboard and status. It includes the wait before the client sends its next\n"
	"           packet, and the jitter. CON: connecting, ZMB: leaving, BOT: bot\n"
	"netPing    Ping of the network: the smallest round trip of the last 4 seconds, close to the ping of the OS\n"
	"userRate   The rate the client asked for (/rate)\n"
	"rate       The rate the server applies: userRate, raised to sv_minRate and capped by sv_maxRate\n"
	"trueRate   Bytes per second the server sent to the client in the last second\n"
	"fps        Commands received in the last second: the FPS of the client\n"
	"packets    Packets received in the last second\n";

static const char explainNetStatusMore[] =
	"timeNudge  Estimate of the cl_timenudge of the client. -: not measured yet, or fewer than about 10\n"
	"           snapshots per second\n"
	"userSnaps  The snaps the client asked for (/snaps)\n"
	"snaps      The snapshots per second the server applies: userSnaps between sv_minSnaps and sv_maxSnaps,\n"
	"           at most sv_fps\n"
	"trueSnaps  Snapshots per second the server sent to the client in the last second\n"
	"id         Client number\n"
	"name       Name\n"
	"packets is not cl_maxpackets: at 125 fps, cl_maxpackets 100 gives about 62 packets per second.\n"
	"trueSnaps below snaps with trueRate close to rate: the rate of the client is too low, raise /rate.\n";

static_assert(sizeof(explainNetStatusColumns) + 8 < MAX_STRING_CHARS, "one print per server command");
static_assert(sizeof(explainNetStatusMore) + 8 < MAX_STRING_CHARS, "one print per server command");

void Proxy_Engine_ClientCommand_ExplainNetStatus(int clientNum)
{
	ClientData_t *currentClientData = &proxy.clientData[clientNum];

	// The same limit as /netStatus, shared with it
	if (currentClientData->lastTimeNetStatus + 500 > server.svs->time)
	{
		return;
	}

	currentClientData->lastTimeNetStatus = server.svs->time;

	proxy.trap->SendServerCommand(clientNum, va("print \"%s\"", explainNetStatusColumns));
	proxy.trap->SendServerCommand(clientNum, va("print \"%s\"", explainNetStatusMore));
}

bool Proxy_Engine_NetStatusCommands_Run(int clientNum, const char* cmd)
{
	if (!proxy.originalEngineCvars.proxy_sv_enableNetStatus.integer)
	{
		return false;
	}

	if (!Q_stricmp(cmd, "netStatus") || !Q_stricmp(cmd, "showNet"))
	{
		Proxy_Engine_ClientCommand_NetStatus(clientNum);

		return true;
	}

	if (!Q_stricmp(cmd, "explainNetStatus"))
	{
		Proxy_Engine_ClientCommand_ExplainNetStatus(clientNum);

		return true;
	}

	return false;
}
