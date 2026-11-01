#pragma once

namespace ybeproxy::features::votes
{

// A configstring that the game sets (G_SET_CONFIGSTRING): the start of a vote
void ConfigstringSet(int num, const char* string);
// A command that the game sends to the server console (G_SEND_CONSOLE_COMMAND). false: the proxy drops it
bool ConsoleCommand(int exec, const char* text);

} // namespace ybeproxy::features::votes
