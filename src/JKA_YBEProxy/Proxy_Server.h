#pragma once

// ==================================================
// INCLUDE
// ==================================================

#include "JKA_YBEProxy/Proxy_Header.h"

// ==================================================
// DEFINE
// ==================================================

#if defined(_WIN32) && !defined(MINGW32)
	// Function address
	#define func_SV_CalcPings_addr 0x444220
	#define func_Sys_Milliseconds_addr 0x4580E0

	// Variable address
	#define var_svs_addr 0x606218
#else
	// Function address
	#define func_SV_CalcPings_addr 0x8057204
	#define func_Sys_Milliseconds_addr 0x80c6714

	// Variable address
	#define var_svs_addr 0x83121e0
#endif

// ==================================================
// STRUCTS
// ==================================================


// ==================================================
// EXTERN VARIABLE
// ==================================================

// ==================================================
// FUNCTION
// ==================================================