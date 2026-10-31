#include "JKA_YBEProxy/Engine/Restart.hpp"
#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Engine/EngineState.hpp"

namespace ybeproxy::engine::restart
{

/*
SV_Frame stops the server before two counters wrap:
- svs.time past 0x70000000, after about 21.7 days: it goes on across map changes, only SV_Shutdown resets it;
- svs.nextSnapshotEntities near 0x7FFFFFFE: it grows by the entities of every snapshot built, and a map change resets
  it, so a server that stays on one map with many players reaches it in hours.
It calls SV_Shutdown then "map_restart 0", but map_restart refuses to run once the server is down ("Server is not
running."): every player is dropped and the server stays without a map until someone restarts it.

The proxy acts a little before those limits. Its command runs at the next Cbuf_Execute of Com_Frame, before the next
SV_Frame and its checks:
- for svs.nextSnapshotEntities, "map <current map>": a normal map change, the players stay connected;
- for svs.time, "killserver" then "map <current map>", as ioq3 and OpenJK do after their shutdown. The players are
  dropped.
*/

// Once per load of the module, which the map change loads again
static bool requested = false;

void BeforeWrap(void)
{
	if (requested)
	{
		return;
	}

	// One minute before the limit of SV_Frame
	const bool timeWraps = server.svs->time > 0x70000000 - 60000;
	// One numSnapshotEntities before the limit of SV_Frame. A server frame adds at most sv_maxclients snapshots of
	// MAX_SNAPSHOT_ENTITIES (256), and numSnapshotEntities is at least sv_maxclients * 4 * 64
	const bool entitiesWrap = server.svs->nextSnapshotEntities >= 0x7FFFFFFE - 2 * server.svs->numSnapshotEntities;

	if (!timeWraps && !entitiesWrap)
	{
		return;
	}

	char mapname[MAX_QPATH];

	proxy.trap->Cvar_VariableStringBuffer("mapname", mapname, sizeof(mapname));
	proxy.trap->Print("----- Proxy: %s is about to wrap, loading %s again\n", timeWraps ? "svs.time" : "svs.nextSnapshotEntities", mapname);
	proxy.trap->SendConsoleCommand(EXEC_APPEND, va("%smap %s\n", timeWraps ? "killserver\n" : "", mapname));

	requested = true;
}

} // namespace ybeproxy::engine::restart
