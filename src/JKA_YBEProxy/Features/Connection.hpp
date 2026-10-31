#pragma once

#include "sdk/qcommon/q_shared.hpp"

// A new connection of a human: nullptr, or the reason to refuse it (missing "ip" key, ban, too many clients from the
// address). Never drops the client itself
const char* Proxy_Connection_Check(int clientNum, qboolean firstTime);
// The game accepted the client: keeps its address
void Proxy_Connection_Connected(int clientNum, qboolean isBot);
// The client entered the game: sends the welcome message after its first connection
void Proxy_Connection_Begin(int clientNum);
