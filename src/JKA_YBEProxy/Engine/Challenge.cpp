#include "JKA_YBEProxy/Engine/Challenge.hpp"
#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Engine/EngineState.hpp"

#include <random>

namespace ybeproxy::engine::challenge
{

/*
The ping that SV_DirectConnect compares with sv_minPing and sv_maxPing is the time between the last challengeResponse
and the connect. SV_GetChallenge keeps the challenge of an address that asks again and only moves its pingTime, so a
client can send a getchallenge right before a connect that carries the challenge of an earlier answer: its ping is
then close to 0, whatever its real ping. ioq3 and OpenJK make a new challenge at each getchallenge, so the connect has
to carry the last one, which the client only gets after a full round trip.
*/

void Renew(const netadr_t& from)
{
	challenge_t* const challenges = server.svs->challenges;

	// Same search as SV_GetChallenge (NET_CompareAdr: the base address and the port), which then sends this challenge
	// and sets its pingTime. A new address gets a new challenge from the original
	for (challenge_t* challenge = challenges; challenge < challenges + MAX_CHALLENGES; ++challenge)
	{
		if (!challenge->connected && from.port == challenge->adr.port
			&& server.common.functions.NET_CompareBaseAdr(from, challenge->adr))
		{
			// Not rand(): on Windows the proxy has its own C runtime, which the engine's srand() does not seed, so its
			// values could be guessed. Built at the first getchallenge, not when the module loads
			static std::random_device random;

			challenge->challenge = (int)random();

			return;
		}
	}
}

} // namespace ybeproxy::engine::challenge
