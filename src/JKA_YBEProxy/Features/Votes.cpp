#include "JKA_YBEProxy/Features/Votes.hpp"
#include "JKA_YBEProxy/Proxy.hpp"

namespace ybeproxy::features::votes
{

// A kick vote stores a slot number: the game runs "clientkick <slot>" when the vote passes, 3 seconds later, and the
// engine kicks whoever holds the slot then. A target that leaves frees its slot after sv_zombietime, and the next client
// or bot of the slot was kicked instead. The address of every slot is kept when a vote starts, and the vote's kick is
// dropped when the slot no longer has the same address (empty for a bot, and once the client left)
static char voteAddresses[MAX_CLIENTS][sizeof(proxy.clientData[0].address)];

void ConfigstringSet(int num, const char* string)
{
	// The game sets CS_VOTE_TIME when a vote starts, and empties it when the vote ends
	if (num != CS_VOTE_TIME || !string || !string[0])
	{
		return;
	}

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		Q_strncpyz(voteAddresses[i], proxy.clientData[i].address, sizeof(voteAddresses[i]));
	}
}

bool ConsoleCommand(int exec, const char* text)
{
	// Only the vote's kick. The game's automatic kick of team killers uses EXEC_INSERT, and means the current client of
	// the slot. The vote keeps the case of the command the client typed
	if (exec != EXEC_APPEND || !text || Q_stricmpn(text, "clientkick ", 11))
	{
		return true;
	}

	const int slot = atoi(text + 11);

	if (slot < 0 || slot >= MAX_CLIENTS || !std::strcmp(proxy.clientData[slot].address, voteAddresses[slot]))
	{
		return true;
	}

	proxy.trap->SendServerCommand(-1, "print \"Vote kick canceled: the player left the server.\n\"");

	return false;
}

} // namespace ybeproxy::features::votes
