#include "JKA_YBEProxy/Proxy_Header.hpp"
#include "Proxy_Engine_Client.hpp"
#include "Proxy_Engine_Utils.hpp"

void Proxy_Engine_Client_UpdateUcmdStats(std::size_t clientNum, usercmd_t* cmd, std::size_t packetIndex)
{
	ClientData_t* currentClientData = &proxy.clientData[clientNum];

	currentClientData->cmdIndex++;
	const std::size_t cmdIndex = currentClientData->cmdIndex & (CMD_MASK - 1);
	currentClientData->cmdStats[cmdIndex].serverTime = cmd->serverTime;
	currentClientData->cmdStats[cmdIndex].packetIndex = packetIndex;
}

// Update value of packets and FPS
// From JK2MV (updated version by fau)
void Proxy_Engine_Client_CalcPacketsAndFPS(int clientNum, int* packets, int* fps)
{
	int			lastCmdTime;
	std::size_t	lastPacketIndex = 0;
	int			i;

	ClientData_t *currentClientData = &proxy.clientData[clientNum];

	lastCmdTime = currentClientData->cmdStats[currentClientData->cmdIndex & (CMD_MASK - 1)].serverTime;

	for (i = 0; i < CMD_MASK; i++)
	{
		ucmdStat_t* stat = &currentClientData->cmdStats[i];

		if (stat->serverTime + 1000 >= lastCmdTime)
		{
			(*fps)++;

			if (lastPacketIndex != stat->packetIndex)
			{
				lastPacketIndex = stat->packetIndex;
				(*packets)++;
			}
		}
	}
}

// Some clientside timings (delta or whatever) that we can't calc here (I think)
static inline int Proxy_GetTimenudgeMagicOffset(int svFps)
{
#if defined(_WIN32) && !defined(MINGW32)
	constexpr int magicOffsets[] = {
		21, 18, 18, 17, 18, 16, 14, 13, 12, 13, // [20, 29]
		13, 20, 10, 9, 9, 9, 8, 9, 9, 9,		// [30, 39]
		9, 5, 6, 6, 5, 5, 6, 6, 5, 5, 			// [40, 49]
		5, 5, 5, 4, 4, 4, 5, 5, 5, 3, 			// [50, 59]
		0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 			// [60, 69] DO NOT USE 60, 61, 62
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0,			// [70, 79]
		0										// [80[
	};
#else
	constexpr int magicOffsets[] = {
		18, 15, 15, 13, 14, 14, 11, 10, 9, 10,				// [20, 29]
		10, 0, 0, 0, 0, -21, -21, -21, -21, -19,			// [30, 39] DO NOT USE 31, 32, 33, 34
		-19, 0, 0, 0, 0, -19, -17, -17, -16, -17, 			// [40, 49] DO NOT USE 41, 42, 43, 44
		-17, -17, -17, -17, -17, -17, -16, -15, -15, 0,		// [50, 59] DO NOT USE 59
		0, 0, 0, -30, -30, -30, -30, -28, -28, -28, 		// [60, 69] DO NOT USE 60, 61, 62
		-28, -28, -28, -28, -28, -28, -27, -26, -26, -27,	// [70, 79]
		-27													// [80[
	};
#endif
	constexpr int tableSize = sizeof(magicOffsets) / sizeof(magicOffsets[0]);

	int index = 0;
	int magicOffset = 0;
	
	if (svFps >= 20)
	{
		index = svFps - 20; // the first offset starts at sv_fps 20, so just -20 to get the index

		if (svFps >= tableSize + 20)
		{
			index = tableSize - 1; // last index of the tab, the offset should be 0 if sv_fps greater than 80
		}

		magicOffset = magicOffsets[index];
	}

	return magicOffset;
}

void Proxy_Engine_Client_UpdateTimenudge(client_t* client, usercmd_t* cmd, int _Milliseconds)
{
	ClientData_t *currentClientData = &proxy.clientData[getClientNumFromAddr(client)];

	currentClientData->timenudgeData.delayCount++;
	currentClientData->timenudgeData.delaySum += cmd->serverTime - server.svs->time;
	currentClientData->timenudgeData.pingSum += client->ping;

	// Wait 1500 ms so we have enough data when we'll calcul an approximation of the timenudge
	if (_Milliseconds < currentClientData->timenudgeData.lastTimeTimeNudgeCalculation + 1500)
	{
		return;
	}

	const int svFps = server.cvars.sv_fps->integer;
	const int magicOffset = Proxy_GetTimenudgeMagicOffset(svFps);

	// ((serverTime - sv.time) + ping -magicOffset + (1000/sv_fps)) * -1
	currentClientData->timenudge =
		(
			(currentClientData->timenudgeData.delaySum / (float)currentClientData->timenudgeData.delayCount)
			+ (currentClientData->timenudgeData.pingSum / (float)currentClientData->timenudgeData.delayCount)
			- magicOffset
			+ (1000 / svFps)
		) * -1;

	currentClientData->timenudgeData.delayCount = 0;
	currentClientData->timenudgeData.delaySum = 0;
	currentClientData->timenudgeData.pingSum = 0;

	currentClientData->timenudgeData.lastTimeTimeNudgeCalculation = _Milliseconds;
}