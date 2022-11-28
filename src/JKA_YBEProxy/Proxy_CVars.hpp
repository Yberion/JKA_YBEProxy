#pragma once

#include "sdk/qcommon/q_shared.hpp"

struct cvarTable_t {
	vmCvar_t*	vmCvar;
	const char*	cvarName;
	const char*	defaultString;
	int			cvarFlags;
	int			modificationCount;  // for tracking changes
	qboolean	trackChange;	    // track this variable, and announce if changed
};

void Proxy_OriginalEngine_CVars_Registration(void);
void Proxy_CVars_Registration(void);
void Proxy_UpdateAllCvars(void);
