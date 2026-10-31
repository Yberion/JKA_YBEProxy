#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Features/Cvars.hpp"
#include "JKA_YBEProxy/Engine/Rate.hpp"

static cvarTable_t proxyOriginalEngineCVarTable[] =
{
	{ &proxy.originalEngineCvars.proxy_sv_pingFix, "proxy_sv_pingFix", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &proxy.originalEngineCvars.proxy_sv_enableNetStatus, "proxy_sv_enableNetStatus", "0", CVAR_ARCHIVE, 0, qfalse },
	// The engine's cvar, with its default and flags: registering it changes nothing
	{ &proxy.originalEngineCvars.sv_maxRate, "sv_maxRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse },
	// The flags of the engine's sv_maxRate
	{ &proxy.originalEngineCvars.sv_minRate, "sv_minRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse },
	// 0: no floor, and a cap of sv_fps
	{ &proxy.originalEngineCvars.sv_minSnaps, "sv_minSnaps", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &proxy.originalEngineCvars.sv_maxSnaps, "sv_maxSnaps", "0", CVAR_ARCHIVE, 0, qfalse },
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
	{ &proxy.cvars.proxy_sv_disableServerInfoVersion, "proxy_sv_disableServerInfoVersion", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &proxy.cvars.proxy_sv_allowSamePlayerNames, "proxy_sv_allowSamePlayerNames", "0", CVAR_ARCHIVE, 0, qfalse },
	// The proxy version in the serverinfo (getstatus), set by Proxy_UpdateServerInfoVersion
	{ nullptr, "YbeProxy", "", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },

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

// A forced set: it also replaces the value kept from an older proxy (the cvar survives the reload on a map change) or set in a cfg
// It does nothing when the value is the same, and the engine leaves a key with an empty value out of the serverinfo
static void Proxy_UpdateServerInfoVersion(void)
{
	proxy.trap->Cvar_Set("YbeProxy", proxy.cvars.proxy_sv_disableServerInfoVersion.integer ? "" : YBEPROXY_VERSION);
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
	Proxy_UpdateServerInfoVersion();
}

// ==================================================
// Proxy_UpdateAllCvars
// --------------------------------------------------
// Updates all the variables once per second of server time.
// ==================================================

static struct
{
	// The server time restarts at 0 on a map change, so a time before the last update means a new map
	int lastUpdateTime;
	// The rate settings the clients got (sv_minRate, sv_minSnaps, sv_maxSnaps, sv_fps). Reset with the module at each
	// map change: they are applied again, which changes nothing
	int appliedRates[4];
} updateState;

void Proxy_UpdateAllCvars(int levelTime)
{
	if (levelTime < updateState.lastUpdateTime || levelTime - updateState.lastUpdateTime >= 1000)
	{
		updateState.lastUpdateTime = levelTime;

		Proxy_UpdateCvarTable(proxyCVarTable, proxyCVarTableSize);
		Proxy_UpdateServerInfoVersion();

		if (proxy.isOriginalEngine)
		{
			Proxy_UpdateCvarTable(proxyOriginalEngineCVarTable, proxyOriginalEngineCVarTableSize);

			const int current[4] = {
				proxy.originalEngineCvars.sv_minRate.integer,
				proxy.originalEngineCvars.sv_minSnaps.integer,
				proxy.originalEngineCvars.sv_maxSnaps.integer,
				proxy.cvars.sv_fps.integer
			};

			if (std::memcmp(updateState.appliedRates, current, sizeof(current)))
			{
				std::memcpy(updateState.appliedRates, current, sizeof(current));
				Proxy_Engine_Rate_SettingsChanged();
			}
		}
	}
}

// Leaves out the engine cvars the proxy only reads (flags 0) and the YbeProxy key (no vmCvar, it is the version)
static void Proxy_PrintCvarTable(const cvarTable_t* table, std::size_t size)
{
	for (const cvarTable_t* item = table; item < table + size; ++item)
	{
		if (!item->vmCvar || !item->cvarFlags)
		{
			continue;
		}

		char value[MAX_CVAR_VALUE_STRING];

		// Read now: the vmCvar is updated once per second only
		proxy.trap->Cvar_VariableStringBuffer(item->cvarName, value, sizeof(value));
		proxy.trap->Print("%-36s %-10s %s\n", item->cvarName, value, item->defaultString);
	}
}

// ==================================================
// Proxy_CVars_PrintConfig
// --------------------------------------------------
// The "proxyconfig" console command.
// ==================================================

void Proxy_CVars_PrintConfig(void)
{
	char version[MAX_STRING_CHARS];

	proxy.trap->Cvar_VariableStringBuffer("version", version, sizeof(version));

	proxy.trap->Print("%s %s\n", YBEPROXY_NAME, YBEPROXY_VERSION);
	proxy.trap->Print("Engine: %s (%s)\n", version, proxy.isOriginalEngine ? "original engine, engine fixes on" : "not the original engine, no engine fixes");
	proxy.trap->Print("%-36s %-10s %s\n", "cvar", "value", "default");
	proxy.trap->Print("------------------------------------ ---------- -------\n");

	// Registered on the original engine only
	if (proxy.isOriginalEngine)
	{
		Proxy_PrintCvarTable(proxyOriginalEngineCVarTable, proxyOriginalEngineCVarTableSize);
	}

	Proxy_PrintCvarTable(proxyCVarTable, proxyCVarTableSize);

	// Engine and game cvars, value only: their default depends on the engine and the mod
	static const char* const serverSettings[] = {
		"sv_hostname", "sv_maxclients", "sv_privateClients", "sv_fps", "sv_maxRate", "sv_pure", "fraglimit", "timelimit"
	};

	proxy.trap->Print("\n%-36s %s\n", "server setting", "value");
	proxy.trap->Print("------------------------------------ ----------\n");

	for (const char* name : serverSettings)
	{
		// Already listed with the cvars of the original engine, next to sv_minRate
		if (proxy.isOriginalEngine && !std::strcmp(name, "sv_maxRate"))
		{
			continue;
		}

		char value[MAX_CVAR_VALUE_STRING];

		proxy.trap->Cvar_VariableStringBuffer(name, value, sizeof(value));
		proxy.trap->Print("%-36s %s\n", name, value);
	}
}
