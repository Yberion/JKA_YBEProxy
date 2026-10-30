#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Rate.hpp"
#include "JKA_YBEProxy/Proxy_Header.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Wrappers.hpp"

/*
The rate of a client is the most bytes per second it wants from the server. The engine waits
(size + 48) * 1000 / rate milliseconds after each message, so a client short of rate gets fewer snapshots, and its game
state and downloads come slower. The retail client sends 4000 by default.

SV_UserinfoChanged keeps the client's rate between 1000 and 90000 (99999 for a LAN client) in cl->rate, and
sv_maxRate caps it when sending (SV_RateMsec, downloads). sv_minRate is written into cl->rate after
SV_UserinfoChanged, so every place that reads cl->rate gets it, the SV_RateMsec of the engine included.
sv_maxRate is applied after it, so the cap wins over a higher floor, as in JK2MV. The same value in both gives every
client that rate.
*/

void Proxy_Engine_Rate_ApplyMinRate(client_t* cl)
{
	// A floor below 1000 changes nothing, cl->rate is at least 1000
	if (cl->rate < proxy.originalEngineCvars.sv_minRate.integer)
	{
		cl->rate = proxy.originalEngineCvars.sv_minRate.integer;
	}
}

void Proxy_Engine_Rate_MinRateChanged(void)
{
	client_t* cl = server.svs->clients;

	for (int i = 0; i < server.cvars.sv_maxclients->integer; ++i, ++cl)
	{
		// SV_UserinfoChanged reads the rate from the userinfo again, so a lower floor gives the clients their own rate
		// back. The rest of it (name, handicap, snaps) gives the same values again
		if (cl->state >= CS_CONNECTED && cl->netchan.remoteAddress.type != NA_BOT)
		{
			server.functions.SV_UserinfoChanged(cl);
		}
	}
}

int Proxy_Engine_Rate_Applied(const client_t* cl)
{
	const int maxRate = server.cvars.sv_maxRate->integer;

	// As SV_RateMsec: a sv_maxRate below 1000 counts as 1000
	if (maxRate && maxRate < cl->rate)
	{
		return (maxRate < 1000) ? 1000 : maxRate;
	}

	return cl->rate;
}
