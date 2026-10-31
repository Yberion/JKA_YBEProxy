#pragma once

#include "sdk/qcommon/q_shared.hpp"

namespace ybeproxy::features::bans
{

// Called before the game loads: takes over the IP ban list from the game (see Bans.cpp)
void BeforeGameInit(void);

// "addip", "removeip", "listip", "ban" and "unban". Returns qfalse when the game has to handle the command
qboolean ConsoleCommand(const char* cmd);

// ip is the "ip" key of the userinfo ("1.2.3.4:29070"). Always qfalse when the proxy did not take over the list
qboolean IsBanned(const char* ip);

} // namespace ybeproxy::features::bans
