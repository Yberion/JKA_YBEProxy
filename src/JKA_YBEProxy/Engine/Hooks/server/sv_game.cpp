#include "JKA_YBEProxy/Engine/EngineState.hpp"
#include "JKA_YBEProxy/Engine/Hooks/server/sv_game.hpp"

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
// Modified version of JK2MV src/server/sv_game.cpp, GPL: returns -1 when ent is not an element of gentities
// (before LocateGameData, outside the array, or not on an element boundary), instead of a bad index
static int Proxy_SV_NumForGentity(sharedEntity_t* ent)
{
	const std::ptrdiff_t offset = (byte*)ent - (byte*)server.sv->gentities;

	if (server.sv->gentitySize <= 0 || offset < 0 || offset % server.sv->gentitySize)
	{
		return -1;
	}

	return (int)(offset / server.sv->gentitySize);
}

svEntity_t* (*Original_SV_SvEntityForGentity)(sharedEntity_t*);
svEntity_t* Proxy_SV_SvEntityForGentity(sharedEntity_t* gEnt)
{
	// Proxy -------------->
	// From JK2MV (mv_fixplayerghosting): the index comes from the address, not from s.number,
	// against the player ghosting bug (players walking through an affected player)
	if (!gEnt)
	{
		Com_Error(ERR_DROP, "SV_SvEntityForGentity: null gEnt\n");
	}

	int num = Proxy_SV_NumForGentity(gEnt);

	// Not an element of gentities: fall back to s.number, as JK2MV does
	if ((unsigned)num >= (unsigned)MAX_GENTITIES)
	{
		num = gEnt->s.number;
	}

	if ((unsigned)num >= (unsigned)MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "SV_SvEntityForGentity: bad gEnt\n");
	}

	// return &sv.svEntities[ gEnt->s.number ];
	return &server.sv->svEntities[num];
	// Proxy <--------------
}
