#pragma once

#include "sdk/qcommon/q_shared.hpp"

// Returns qtrue when the command is the proxy's ("mute", "unmute", "proxyconfig"...), qfalse to let the game handle it
qboolean Proxy_ConsoleCommands_Run(void);
// The client number in argument 1 of the console command cmd, of a player in the game. Otherwise prints the usage and
// returns -1
int Proxy_SharedAPI_ConsoleClientNum(const char* cmd);
