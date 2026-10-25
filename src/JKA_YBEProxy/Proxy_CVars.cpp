#include "JKA_YBEProxy/Proxy_Header.hpp"
#include "JKA_YBEProxy/Proxy_CVars.hpp"

static cvarTable_t proxyOriginalEngineCVarTable[] =
{
	{ &proxy.originalEngineCvars.proxy_sv_pingFix, "proxy_sv_pingFix", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &proxy.originalEngineCvars.proxy_sv_enableNetStatus, "proxy_sv_enableNetStatus", "0", CVAR_ARCHIVE, 0, qfalse },
};
static constexpr std::size_t proxyOriginalEngineCVarTableSize = ARRAY_LEN(proxyOriginalEngineCVarTable);

static cvarTable_t proxyCVarTable[] =
{
	// New cvars
	{ &proxy.cvars.proxy_sv_maxCallVoteMapRestartValue, "proxy_sv_maxCallVoteMapRestartValue", "60", CVAR_ARCHIVE, 0, qfalse },
	{ &proxy.cvars.proxy_sv_modelPathLength, "proxy_sv_modelPathLength", "64", CVAR_ARCHIVE, 0, qfalse },
	{ &proxy.cvars.proxy_sv_disableKillCmd, "proxy_sv_disableKillCmd", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &proxy.cvars.proxy_sv_maxClientsPerIP, "proxy_sv_maxClientsPerIP", "3", CVAR_ARCHIVE, 0, qfalse },
	{ &proxy.cvars.proxy_sv_disableWelcomeMessage, "proxy_sv_disableWelcomeMessage", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &proxy.cvars.proxy_sv_banIPs, "proxy_sv_banIPs", "", CVAR_ARCHIVE, 0, qfalse },

	// get cvars
	// Flags 0: Cvar_Get ORs the flags into the engine's cvar
	{ &proxy.cvars.sv_fps, "sv_fps", "20", 0, 0, qfalse },
	// The engine's sv_gametype variable is the "g_gametype" cvar
	{ &proxy.cvars.sv_gametype, "g_gametype", "0", 0, 0, qfalse }
};
static constexpr std::size_t proxyCVarTableSize = ARRAY_LEN(proxyCVarTable);

static void Proxy_RegisterCvarTable(cvarTable_t* table, std::size_t size)
{
	for (cvarTable_t* item = table; item < table + size; ++item)
	{
		proxy.trap->Cvar_Register(item->vmCvar, item->cvarName, item->defaultString, item->cvarFlags);

		if (item->vmCvar)
		{
			item->modificationCount = item->vmCvar->modificationCount;
		}
	}
}

static void Proxy_UpdateCvarTable(cvarTable_t* table, std::size_t size)
{
	for (cvarTable_t* item = table; item < table + size; ++item)
	{
		if (item->vmCvar)
		{
			int vmCvarModificationCount = item->vmCvar->modificationCount;
			proxy.trap->Cvar_Update(item->vmCvar);

			if (item->modificationCount != vmCvarModificationCount)
			{
				item->modificationCount = vmCvarModificationCount;

				if (item->trackChange)
				{
					proxy.trap->SendServerCommand(-1, va("print \"Server: %s changed to %s\n\"",
						item->cvarName, item->vmCvar->string));
				}
			}
		}
	}
}

// ==================================================
// Proxy_OriginalEngine_CVars_Registration
// --------------------------------------------------
// Those are the variables that will affect engine modifications.
// ==================================================

void Proxy_OriginalEngine_CVars_Registration(void)
{
	proxy.trap->Print("----- Proxy: Initializing original engine Proxy CVars\n");

	Proxy_RegisterCvarTable(proxyOriginalEngineCVarTable, proxyOriginalEngineCVarTableSize);

	proxy.trap->Print("----- Proxy: Original engine Proxy CVars properly initialized\n");
}

// ==================================================
// Proxy_CVars_Registration
// --------------------------------------------------
// Those are the variables that won't affect engine modifications.
// ==================================================

void Proxy_CVars_Registration(void)
{
	Proxy_RegisterCvarTable(proxyCVarTable, proxyCVarTableSize);
}

// ==================================================
// Proxy_UpdateAllCvars
// --------------------------------------------------
// Updates all the variables once per second of server time.
// ==================================================

void Proxy_UpdateAllCvars(int levelTime)
{
	// The server time restarts at 0 on a map change, so a time before the last update means a new map
	static int lastUpdateTime = 0;

	if (levelTime < lastUpdateTime || levelTime - lastUpdateTime >= 1000)
	{
		lastUpdateTime = levelTime;

		Proxy_UpdateCvarTable(proxyCVarTable, proxyCVarTableSize);

		if (proxy.isOriginalEngine)
		{
			Proxy_UpdateCvarTable(proxyOriginalEngineCVarTable, proxyOriginalEngineCVarTableSize);
		}
	}
}
