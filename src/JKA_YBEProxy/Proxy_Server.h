#pragma once

// ==================================================
// INCLUDE
// ==================================================

#include "game/g_local.h"
#include "server/server.h"

// ==================================================
// DEFINE
// ==================================================

#if defined(_WIN32) && !defined(MINGW32)
	// Function address to hook
	#define func_SV_CalcPings_addr 0x444220
	#define func_SV_SendMessageToClient_addr 0x444dc0
	
	// Function address to call
	#define func_SV_Netchan_Transmit_addr 0x444950
	#define func_SV_RateMsec_addr 0x444d60
	#define func_Netchan_TransmitNextFragment_addr 0x41a3b0
	#define func_Sys_IsLANAddress_addr 0x457490

	// Function address called in ASM
	#define func_Sys_Milliseconds_addr 0x4580E0

	// Variable address
	#define var_svs_addr 0x606218
#else
	// Function address to hook
	#define func_SV_CalcPings_addr 0x8057204
	#define func_SV_SendMessageToClient_addr 0x8058c84

	// Function address to call
	#define func_SV_Netchan_Transmit_addr 0x8057db4
	#define func_SV_RateMsec_addr 0x8058c04
	#define func_Netchan_TransmitNextFragment_addr 0x807ab74
	#define func_Sys_IsLANAddress_addr 0x80c5f84

	// Function address called in ASM
	#define func_Sys_Milliseconds_addr 0x80c6714

	// Variable address
	#define var_svs_addr 0x83121e0
#endif

// ==================================================
// STRUCT
// ==================================================

typedef struct serverFunctions_s
{
	void		(*SV_Netchan_Transmit)							(client_t*, msg_t*);
	int			(*SV_RateMsec)									(client_t*, int);
	void		(*Netchan_TransmitNextFragment)					(netchan_t*);
	qboolean	(*Sys_IsLANAddress)								(netadr_t);
} serverFunctions_t;