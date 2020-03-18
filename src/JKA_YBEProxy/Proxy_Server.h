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
	#define func_SV_UserMove_addr 0x43c210
	
	// Function address to call
	#define func_SV_ClientEnterWorld_addr 0x43b230
	#define func_SV_ClientThink_addr 0x43bba0
	#define func_SV_DropClient_addr 0x43bbf0
	#define func_SV_Netchan_Transmit_addr 0x444950
	#define func_SV_RateMsec_addr 0x444d60
	#define func_Com_HashKey_addr 0x410370
	#define func_Com_Printf_addr 0x40fbe0
	#define func_Netchan_TransmitNextFragment_addr 0x41a3b0
	#define func_MSG_ReadByte_addr 0x4189f0
	#define func_MSG_ReadDeltaUsercmdKey_addr 0x418b50
	#define func_Sys_IsLANAddress_addr 0x457490

	// Function address called in ASM
	#define func_Sys_Milliseconds_addr 0x4580E0

	// Variable address
	#define var_svs_addr 0x606218
	#define var_svsClients_addr 0x606224
	#define var_sv_addr 0x567F30

	// cvar (for addr check in function: 0x442f60)
	#define cvar_sv_fps_addr 0x6102d0
	#define cvar_sv_maxclients_addr 0x610278
	#define cvar_sv_pure_addr 0x60620c
#else
	// Function address to hook
	#define func_SV_CalcPings_addr 0x8057204
	#define func_SV_SendMessageToClient_addr 0x8058c84
	#define func_SV_UserMove_addr 0x804e6c4

	// Function address to call
	#define func_SV_ClientEnterWorld_addr 0x804d444
	#define func_SV_ClientThink_addr 0x804e634
	#define func_SV_DropClient_addr 0x804cb84
	#define func_SV_Netchan_Transmit_addr 0x8057db4
	#define func_SV_RateMsec_addr 0x8058c04
	#define func_Com_HashKey_addr 0x8073b14
	#define func_Com_Printf_addr 0x8072ca4
	#define func_Netchan_TransmitNextFragment_addr 0x807ab74
	#define func_MSG_ReadByte_addr 0x8077df4
	#define func_MSG_ReadDeltaUsercmdKey_addr 0x8078b34
	#define func_Sys_IsLANAddress_addr 0x80c5f84

	// Function address called in ASM
	#define func_Sys_Milliseconds_addr 0x80c6714

	// Variable address
	#define var_svs_addr 0x83121e0
	#define var_svsClients_addr 0x83121ec
	#define var_sv_addr 0x8273ec0

	// cvar (for addr check in function: 0x8055824)
	#define cvar_sv_fps_addr 0x8273e84
	#define cvar_sv_maxclients_addr 0x8273ea4
	#define cvar_sv_pure_addr 0x83121a8
	
#endif

#define getClientNumFromAddr(client_ptr) ((client_ptr) - (*(client_t **)var_svsClients_addr))

// ==================================================
// STRUCT
// ==================================================

typedef struct serverFunctions_s
{
	void		(*SV_ClientEnterWorld)							(client_t*, usercmd_t*);
	void		(*SV_ClientThink)								(client_t*, usercmd_t*);
	void		(*SV_DropClient)								(client_t*, const char*);
	void		(*SV_Netchan_Transmit)							(client_t*, msg_t*);
	int			(*SV_RateMsec)									(client_t*, int);
	int			(*Com_HashKey)									(char*, int);
	void		(*Com_Printf)									(const char*, ...);
	void		(*Netchan_TransmitNextFragment)					(netchan_t*);
	int			(*MSG_ReadByte)									(msg_t*);
	void		(*MSG_ReadDeltaUsercmdKey)						(msg_t*, int, usercmd_t*, usercmd_t*);
	qboolean	(*Sys_IsLANAddress)								(netadr_t);
} serverFunctions_t;

typedef struct serverCvar_s
{
	cvar_t* sv_fps;
	cvar_t* sv_maxclients;
	cvar_t* sv_pure;
} serverCvars_t;