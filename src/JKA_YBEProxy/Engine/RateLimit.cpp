#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Engine/RateLimit.hpp"
#include "JKA_YBEProxy/Engine/EngineState.hpp"

#include <cstring>
#include <map>

namespace ybeproxy::engine::rate_limit
{

// ==================================================
// Modified version of OpenJK codemp/server/sv_main.cpp
// (SVC_RateLimit and SVC_BucketForAddress, which come
// from ioquake3), GPL. Changes:
// - times are unsigned: Sys_Milliseconds is an int that
//   wraps after about 24.8 days of uptime. In OpenJK the
//   garbage collector then stops until the next map
//   change, and the subtractions overflow (undefined
//   behaviour). In unsigned, the differences stay right
//   across the wrap, and a time in the future gives a
//   huge interval, which resets the bucket like OpenJK's
//   "interval < 0" test;
// - the address table has a size cap;
// - the IPv4 key is read from netadr_t.ip: the retail
//   netadr_t has no ipi.
// ==================================================

struct LeakyBucket
{
	unsigned int lastTime;
	int burst;
};

// Returns true when the request must be dropped
static bool Limit(LeakyBucket* bucket, int burst, unsigned int period, unsigned int now)
{
	const unsigned int interval = now - bucket->lastTime;
	const unsigned int expired = interval / period;

	if (expired > (unsigned int)bucket->burst)
	{
		bucket->burst = 0;
		bucket->lastTime = now;
	}
	else
	{
		bucket->burst -= (int)expired;
		bucket->lastTime = now - interval % period;
	}

	if (bucket->burst < burst)
	{
		bucket->burst++;

		return false;
	}

	return true;
}

// An entry lives until its bucket is empty, burst * period: 1 to 2 s for a spoofed address that sends one packet
// (the garbage collector runs once per second), so a spoofed flood of N new addresses per second keeps up to 2 * N entries
static constexpr std::size_t maxBuckets = 65536;

static struct
{
	std::map<uint32_t, LeakyBucket>	buckets;
	unsigned int						lastGC;
	// getstatus and getinfo, every address
	LeakyBucket						outbound;
	// rcon with a bad password, every address
	LeakyBucket						badPassword;
} rateLimit;

// Returns nullptr when the table is full: the caller drops the request
static LeakyBucket* BucketForAddress(const netadr_t& address, unsigned int period, unsigned int now)
{
	std::map<uint32_t, LeakyBucket>& bucketMap = rateLimit.buckets;

	if (now - rateLimit.lastGC > 1000)
	{
		rateLimit.lastGC = now;

		for (auto it = bucketMap.begin(); it != bucketMap.end();)
		{
			// Empty bucket: removing the entry loses nothing
			if (now - it->second.lastTime > (unsigned int)it->second.burst * period)
			{
				it = bucketMap.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	uint32_t key;

	std::memcpy(&key, address.ip, sizeof(key));

	auto it = bucketMap.find(key);

	if (it == bucketMap.end())
	{
		if (bucketMap.size() >= maxBuckets)
		{
			return nullptr;
		}

		it = bucketMap.emplace(key, LeakyBucket{ now, 0 }).first;
	}

	return &it->second;
}

// Per address (10, 1000): one bucket per address, shared by every limited command, as in ioq3 and Quake3e
// Protects the victim of a reflection attack. Only IPv4 addresses are limited (not loopback or bots)
bool AddressAllowed(const netadr_t& from, const char* command)
{
	if (from.type != NA_IP)
	{
		return true;
	}

	const unsigned int now = (unsigned int)proxy.trap->Milliseconds();
	LeakyBucket* bucket = BucketForAddress(from, 1000, now);

	if (bucket && !Limit(bucket, 10, 1000, now))
	{
		return true;
	}

	// Each drop is printed only with developer 1, as in ioq3 and Quake3e
	if (server.common.cvars.com_developer->integer)
	{
		server.common.functions.Com_Printf(bucket ? "%s: rate limit from %s exceeded, dropping request\n" : "%s: rate limit table full, dropping request from %s\n",
			command, server.common.functions.NET_AdrToString(from));
	}

	return false;
}

// getstatus / getinfo: per address, then a global outbound bucket (10, 100) that caps the total. Same values as ioq3
bool StatusAllowed(const netadr_t& from, const char* command)
{
	if (!AddressAllowed(from, command))
	{
		return false;
	}

	if (!Limit(&rateLimit.outbound, 10, 100, (unsigned int)proxy.trap->Milliseconds()))
	{
		return true;
	}

	if (server.common.cvars.com_developer->integer)
	{
		server.common.functions.Com_Printf("%s: rate limit exceeded, dropping request\n", command);
	}

	return false;
}

// Valid password: never limited, so admins keep rcon during a flood, even one that spoofs their address
// Bad password: one global bucket (10, 1000), about one guess per second whatever the source addresses
bool RconAllowed(void)
{
	const char* password = server.cvars.sv_rconPassword->string;

	if (password[0] && !std::strcmp(server.common.functions.Cmd_Argv(1), password))
	{
		return true;
	}

	if (!Limit(&rateLimit.badPassword, 10, 1000, (unsigned int)proxy.trap->Milliseconds()))
	{
		return true;
	}

	// Dropped without a reply, which also caps the "Bad rconpassword." replies sent to spoofed addresses
	if (server.common.cvars.com_developer->integer)
	{
		server.common.functions.Com_Printf("rcon: rate limit exceeded, dropping request\n");
	}

	return false;
}

} // namespace ybeproxy::engine::rate_limit
