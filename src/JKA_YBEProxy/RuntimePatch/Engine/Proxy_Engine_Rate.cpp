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

snaps is the number of snapshots per second the client wants. SV_UserinfoChanged keeps it between 1 and 30 whatever
sv_fps is, so nobody gets more than 20 at sv_fps 40 (33 ms rounds up to two frames). It is read again here, between
sv_minSnaps and sv_maxSnaps, as in JK2MV, and never above sv_fps (one snapshot per frame at most), as in ioq3, Quake3e
and OpenJK. The server still sends on its frames only, so only sv_fps / n is reached.
*/

void Proxy_Engine_Rate_Apply(client_t* cl)
{
	// A floor below 1000 changes nothing, cl->rate is at least 1000
	if (cl->rate < proxy.originalEngineCvars.sv_minRate.integer)
	{
		cl->rate = proxy.originalEngineCvars.sv_minRate.integer;
	}

	// The engine resets an sv_fps below 1 to 10, at the next SV_Frame only
	const int fps = (server.cvars.sv_fps->integer < 1) ? 10 : server.cvars.sv_fps->integer;
	const int maxSnaps = (proxy.originalEngineCvars.sv_maxSnaps.integer > 0 && proxy.originalEngineCvars.sv_maxSnaps.integer < fps)
		? proxy.originalEngineCvars.sv_maxSnaps.integer
		: fps;
	// The cap wins over a higher floor
	const int minSnaps = (proxy.originalEngineCvars.sv_minSnaps.integer < maxSnaps) ? proxy.originalEngineCvars.sv_minSnaps.integer : maxSnaps;

	// As the engine: 20 without the key
	const char* val = Info_ValueForKey(cl->userinfo, "snaps");
	int snaps = val[0] ? atoi(val) : 20;

	snaps = (snaps < minSnaps) ? minSnaps : snaps;
	snaps = (snaps > maxSnaps) ? maxSnaps : snaps;
	snaps = (snaps < 1) ? 1 : snaps;

	cl->snapshotMsec = 1000 / snaps;
}

void Proxy_Engine_Rate_SettingsChanged(void)
{
	client_t* cl = server.svs->clients;

	for (int i = 0; i < server.cvars.sv_maxclients->integer; ++i, ++cl)
	{
		// SV_UserinfoChanged reads the rate and the snaps from the userinfo again, so a lower floor gives the clients
		// their own values back. The rest of it (name, handicap) gives the same values again
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
