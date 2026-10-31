#include "JKA_YBEProxy/Engine/Challenge.hpp"
#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Engine/EngineState.hpp"

#include <algorithm>
#include <bit>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <random>

namespace ybeproxy::engine::challenge
{

/*
Stateless challenges, as OpenJK (codemp/server/sv_challenge.cpp), TaystJK and Quake3e.

The original SV_GetChallenge stores every new address in svs.challenges (1024 entries) and overwrites the oldest one
when the table is full, and SV_DirectConnect only accepts a connect whose challenge is still there. A flood of spoofed
getchallenge from more than 1024 addresses during the round trip of a player erased the player's challenge: the retail
client then sends the same connect every 3 seconds and only gets "No or bad challenge for address.".

The proxy answers getchallenge itself and stores nothing: the challenge is a keyed hash of the address and of a
timestamp, so only the address that received it can send it back. A connect with a valid challenge gets its entry in
svs.challenges just before the original looks it up, so the table only holds addresses that answered.

A stateless challenge does not know when it was sent: sv_minPing and sv_maxPing are no longer checked on connect, as
in OpenJK, TaystJK and Quake3e.
*/

// The timestamp changes every 2^14 ms = 16.384 s: a challenge is valid for 16 to 32 s
static constexpr int timestampShift = 14;

// SipHash-2-4 (Aumasson and Bernstein) of a 16-byte message, given as two little-endian words: a keyed hash made for
// short inputs, much smaller than the HMAC-MD5 of OpenJK
static constexpr uint64_t SipHash24(const uint64_t (&key)[2], uint64_t m0, uint64_t m1)
{
	uint64_t v0 = 0x736f6d6570736575ULL ^ key[0];
	uint64_t v1 = 0x646f72616e646f6dULL ^ key[1];
	uint64_t v2 = 0x6c7967656e657261ULL ^ key[0];
	uint64_t v3 = 0x7465646279746573ULL ^ key[1];

	const auto sipRound = [&]() {
		v0 += v1; v1 = std::rotl(v1, 13) ^ v0; v0 = std::rotl(v0, 32);
		v2 += v3; v3 = std::rotl(v3, 16) ^ v2;
		v0 += v3; v3 = std::rotl(v3, 21) ^ v0;
		v2 += v1; v1 = std::rotl(v1, 17) ^ v2; v2 = std::rotl(v2, 32);
	};

	// The last word only holds the length of the message, in its top byte
	for (const uint64_t m : { m0, m1, (uint64_t)16 << 56 })
	{
		v3 ^= m;
		sipRound();
		sipRound();
		v0 ^= m;
	}

	v2 ^= 0xff;
	sipRound();
	sipRound();
	sipRound();
	sipRound();

	return v0 ^ v1 ^ v2 ^ v3;
}

// Test vector of the reference implementation: key 00..0f, message 00..0f
static_assert(SipHash24({ 0x0706050403020100ULL, 0x0f0e0d0c0b0a0908ULL }, 0x0706050403020100ULL, 0x0f0e0d0c0b0a0908ULL) == 0x3f2acc7f57c29bdbULL);

// The engine unloads the proxy on every map change: the key is kept in a cvar, so that a challenge sent just before a
// map change is still valid after it. CVAR_INTERNAL: cvarlist does not show it. CVAR_ROM: only code can set it
static constexpr const char* keyCvarName = "proxy_sv_challengeKey";

static uint64_t key[2];
static bool keyLoaded = false;

static void LoadKey(void)
{
	char hex[MAX_CVAR_VALUE_STRING];

	proxy.trap->Cvar_Register(nullptr, keyCvarName, "", CVAR_INTERNAL | CVAR_ROM);
	proxy.trap->Cvar_VariableStringBuffer(keyCvarName, hex, sizeof(hex));

	if (std::sscanf(hex, "%16" SCNx64 "%16" SCNx64, &key[0], &key[1]) != 2)
	{
		// Not rand(): on Windows the proxy has its own C runtime, which the engine's srand() does not seed
		std::random_device random;

		for (uint64_t& word : key)
		{
			word = (uint64_t)random() << 32 | random();
		}

		std::snprintf(hex, sizeof(hex), "%016" PRIx64 "%016" PRIx64, key[0], key[1]);
		proxy.trap->Cvar_Set(keyCvarName, hex);
	}

	keyLoaded = true;
}

static int Create(int timestamp, const netadr_t& from)
{
	if (!keyLoaded)
	{
		LoadKey();
	}

	uint32_t ip;

	std::memcpy(&ip, from.ip, sizeof(ip));

	const uint64_t hash = SipHash24(key, ip | (uint64_t)from.port << 32, (uint32_t)timestamp);

	// 31 bits of the hash, and the parity of the timestamp in the top bit (see Verify). The client reads it with atoi
	return (int)(((uint32_t)hash & 0x7FFFFFFF) | ((uint32_t)timestamp & 1) << 31);
}

static bool Verify(int challenge, const netadr_t& from)
{
	const int timestamp = server.svs->time >> timestampShift;

	// The top bit is the parity of the timestamp the challenge was made with: the current one, or the previous one when
	// the timestamp changed between the getchallenge and the connect
	return challenge == Create(timestamp - ((timestamp ^ (int)((uint32_t)challenge >> 31)) & 1), from);
}

void Answer(const netadr_t& from)
{
	server.common.functions.NET_OutOfBandPrint(NS_SERVER, from, "challengeResponse %i", Create(server.svs->time >> timestampShift, from));
}

void Admit(const netadr_t& from)
{
	const int challenge = std::atoi(Info_ValueForKey(server.common.functions.Cmd_Argv(1), "challenge"));
	challenge_t* const challenges = server.svs->challenges;
	challenge_t* entry = nullptr;
	challenge_t* oldest = challenges;

	// Same search as the original (NET_CompareAdr: the base address and the port)
	for (challenge_t* it = challenges; it < challenges + MAX_CHALLENGES; ++it)
	{
		if (it->challenge == challenge && from.port == it->adr.port && server.common.functions.NET_CompareBaseAdr(from, it->adr))
		{
			entry = it;
			break;
		}

		if (it->time < oldest->time)
		{
			oldest = it;
		}
	}

	// A connect sent again (lost packet, full server) finds its entry, even once its challenge has expired. Otherwise a
	// valid challenge takes the oldest entry, as in the original SV_GetChallenge
	if (!entry)
	{
		// The original then answers "No or bad challenge for address."
		if (!Verify(challenge, from))
		{
			return;
		}

		entry = oldest;
		entry->adr = from;
		entry->challenge = challenge;
		entry->time = server.svs->time;
		entry->firstTime = server.svs->time;
		entry->connected = qfalse;
	}

	// The original refuses a ping (svs.time - pingTime) below sv_minPing or above sv_maxPing. A ping of sv_minPing
	// passes both, so neither is checked (sv_minPing above sv_maxPing already refused everyone)
	char minPing[MAX_CVAR_VALUE_STRING];

	proxy.trap->Cvar_VariableStringBuffer("sv_minPing", minPing, sizeof(minPing));
	entry->pingTime = server.svs->time - std::max(0, (int)std::ceil(std::atof(minPing)));
}

} // namespace ybeproxy::engine::challenge
