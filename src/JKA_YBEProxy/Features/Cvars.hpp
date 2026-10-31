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
void PrintConfig(void);

} // namespace ybeproxy::features::cvars
