#pragma once

#include "sdk/qcommon/q_shared.hpp"

namespace ybeproxy::features::connection
{

// A new connection of a human: nullptr, or the reason to refuse it (missing "ip" key, ban, too many clients from the
// address). Never drops the client itself
const char* Check(int clientNum, qboolean firstTime);
// The game accepted the client: keeps its address
void Connected(int clientNum, qboolean isBot);
// The client entered the game: sends the welcome message after its first connection
void Begin(int clientNum);

} // namespace ybeproxy::features::connection
