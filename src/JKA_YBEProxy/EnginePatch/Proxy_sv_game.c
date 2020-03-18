#include "Proxy_EnginePatch.h"

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
static int Proxy_SV_NumForGentity(sharedEntity_t* ent)
{
	int		num;

	num = ((byte*)ent - (byte*)proxy.server.sv->gentities) / proxy.server.sv->gentitySize;

	return num;
}

svEntity_t* Proxy_SV_SvEntityForGentity(sharedEntity_t* gEnt)
{
	if (!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "SV_SvEntityForGentity: bad gEnt\n");
	}

	// Proxy -------------->
	// return &sv.svEntities[ gEnt->s.number ];
	return &proxy.server.sv->svEntities[Proxy_SV_NumForGentity(gEnt)];
	// Proxy <--------------
}
