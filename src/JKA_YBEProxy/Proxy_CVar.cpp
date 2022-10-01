#include "Proxy_Header.hpp"

typedef struct cvarTable_s {
	vmCvar_t*	vmCvar;
	char*		cvarName;
	char*		defaultString;
	int			cvarFlags;
	int			modificationCount;  // for tracking changes
	qboolean	trackChange;	    // track this variable, and announce if changed
} cvarTable_t;

static cvarTable_t proxyOldAPICVarTable[] =
{
	{ &proxy.cvarsOldAPI.sv_pingFix, "sv_pingFix", "1", CVAR_ARCHIVE, 0, qfalse }
};
static const size_t proxyOldAPICVarTableSize = ARRAY_LEN(proxyOldAPICVarTable);

// ==================================================
// Proxy_OldAPI_CVars_Registration
// --------------------------------------------------
// Registers all the variables in the proxyOldAPICVarTable array.
// Those are the variables that will affect engine modifications.
// ==================================================

void Proxy_OldAPI_CVars_Registration(void)
{
	unsigned int i = 0;
	const cvarTable_t* currentCVarTable = nullptr;

	for (i = 0, currentCVarTable = proxyOldAPICVarTable; i < proxyOldAPICVarTableSize; ++i, ++currentCVarTable)
	{
		proxy.trap->Cvar_Register(currentCVarTable->vmCvar, currentCVarTable->cvarName, currentCVarTable->defaultString, currentCVarTable->cvarFlags);
	}
}

// ==================================================
// Proxy_OldAPI_UpdateCvars
// --------------------------------------------------
// Updates all the variables.
// ==================================================

void Proxy_OldAPI_UpdateCvars(void)
{
	unsigned int i = 0;
	cvarTable_t* currentCVarTable = nullptr;

	for (i = 0, currentCVarTable = proxyOldAPICVarTable; i < proxyOldAPICVarTableSize; ++i, ++currentCVarTable)
	{
		if (currentCVarTable->vmCvar)
		{
			int vmCvarModificationCount = currentCVarTable->vmCvar->modificationCount;
			proxy.trap->Cvar_Update(currentCVarTable->vmCvar);

			if (currentCVarTable->modificationCount != vmCvarModificationCount)
			{
				currentCVarTable->modificationCount = vmCvarModificationCount;

				if (currentCVarTable->trackChange)
				{
					proxy.trap->SendServerCommand(-1, va("print \"Server: %s changed to %s\n\"",
						currentCVarTable->cvarName, currentCVarTable->vmCvar->string));
				}
			}
		}
	}
}
