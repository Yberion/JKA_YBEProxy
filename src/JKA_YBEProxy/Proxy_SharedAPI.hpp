#pragma once

#include "sdk/game/g_public.hpp"

// ==================================================
// IMPORT TABLE
// ==================================================

void Proxy_SharedAPI_LocateGameData(sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient);
void Proxy_SharedAPI_GetUsercmd(int clientNum, usercmd_t* ucmd);

// ==================================================
// EXPORT TABLE
// ==================================================

const char* Proxy_SharedAPI_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot);
// Called when the game accepted the client
void Proxy_SharedAPI_ClientConnected(int clientNum, qboolean isBot);
void Proxy_SharedAPI_ClientDisconnect(int clientNum);
void Proxy_SharedAPI_ClientBegin(int clientNum, qboolean allowTeamReset);
qboolean Proxy_SharedAPI_ClientCommand(int clientNum);
// Returns qtrue when the command is the proxy's ("mute", "unmute"), qfalse to let the game handle it
qboolean Proxy_SharedAPI_ConsoleCommand(void);
// The client number in argument 1 of the console command cmd, of a player in the game. Otherwise prints the usage and
// returns -1
int Proxy_SharedAPI_ConsoleClientNum(const char* cmd);
qboolean Proxy_SharedAPI_ClientUserinfoChanged(int clientNum);

// Checks the userinfo of a client (only if validate), then cleans it in place
// Returns nullptr, or the reason to reject the client. Never drops the client itself
// isAntiCheat, if not null, tells whether the reason is the anti-cheat's
const char* Proxy_SharedAPI_SanitizeUserinfo(int clientNum, bool validate, bool* isAntiCheat);

/*
// If we want to do something with the usercmd then call manually GetUsercmd
void Proxy_SharedAPI_ClientThink(int clientNum, usercmd_t* ucmd);
*/
