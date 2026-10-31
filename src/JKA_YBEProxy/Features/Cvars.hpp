#pragma once

#include "sdk/qcommon/q_shared.hpp"

namespace ybeproxy::features::cvars
{

struct CvarTable {
	vmCvar_t*	vmCvar;
	const char*	cvarName;
	const char*	defaultString;
	int			cvarFlags;
	int			modificationCount;  // for tracking changes
	qboolean	trackChange;	    // track this variable, and announce if changed
};

void OriginalEngineRegistration(void);
void Registration(void);
void UpdateAll(int levelTime);
// "proxyconfig": the version, the engine, and every proxy cvar with its value
// proxyconfig: to the server console when clientNum is -1, to the console of the client otherwise
void PrintConfig(int clientNum);

} // namespace ybeproxy::features::cvars
