#pragma once

namespace ybeproxy::features::chat
{

// Called before the game runs a client command, and once it has run it
void ClientCommand(int clientNum, const char* cmd);
void ClientCommandDone(void);
// The text of a G_SEND_SERVER_COMMAND of the game: text itself, another text, or nullptr to drop it (say_team of a
// spectator below the team gametypes, see Chat.cpp)
const char* TeamChat(int clientNum, const char* text);

} // namespace ybeproxy::features::chat
