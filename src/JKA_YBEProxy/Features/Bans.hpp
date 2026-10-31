#pragma once

#include "sdk/qcommon/q_shared.hpp"

// Called before the game loads: takes over the IP ban list from the game (see Bans.cpp)
void Proxy_Bans_BeforeGameInit(void);

// "addip", "removeip", "listip", "ban" and "unban". Returns qfalse when the game has to handle the command
qboolean Proxy_Bans_ConsoleCommand(const char* cmd);

// ip is the "ip" key of the userinfo ("1.2.3.4:29070"). Always qfalse when the proxy did not take over the list
qboolean Proxy_Bans_IsBanned(const char* ip);
