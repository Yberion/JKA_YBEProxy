#include "JKA_YBEProxy/Proxy.hpp"
#include "sdk/game/g_public.hpp"
#include "JKA_YBEProxy/Module/GameImports.hpp"
#include "JKA_YBEProxy/Module/Main.hpp"
#include "JKA_YBEProxy/Features/Chat.hpp"

#include <cstdarg>

namespace ybeproxy::module::game_imports
{

static void LocateGameData(sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient)
{
	proxy.locatedGameData.g_entities = gEnts;
	proxy.locatedGameData.num_entities = numGEntities;
	proxy.locatedGameData.g_entitySize = sizeofGEntity_t;

	proxy.locatedGameData.g_clients = clients;
	proxy.locatedGameData.g_clientSize = sizeofGameClient;
}

static void GetUsercmd(int /*clientNum*/, usercmd_t* ucmd)
{
	if (ucmd->forcesel == FP_LEVITATION || ucmd->forcesel >= NUM_FORCE_POWERS)
	{
		ucmd->forcesel = 0xFFu;
	}

	ucmd->angles[ROLL] = 0;

	// A signed char reaches -128 but only +127: a negative move would be one unit longer than the longest positive one
	// (ioq3 bug 5273, OpenJK, Quake3e and TaystJK do it when they read the usercmd)
	if (ucmd->forwardmove == -128)
	{
		ucmd->forwardmove = -127;
	}

	if (ucmd->rightmove == -128)
	{
		ucmd->rightmove = -127;
	}

	if (ucmd->upmove == -128)
	{
		ucmd->upmove = -127;
	}
}

intptr_t QDECL Syscall(intptr_t command, ...)
{
	intptr_t args[16];
	
	va_list ap;

	va_start(ap, command);

	for (std::size_t i = 0; i < sizeof(args) / sizeof(args[i]); ++i)
	{
		args[i] = va_arg(ap, intptr_t);
	}

	va_end(ap);

	switch (command)
	{
		//==================================================
		case G_LOCATE_GAME_DATA: // (sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient)
		//==================================================
		{
			LocateGameData((sharedEntity_t*)args[0], (int)args[1], (int)args[2], (playerState_t*)args[3], (int)args[4]);

			break;
		}
		//==================================================
		case G_GET_USERCMD: // (int clientNum, usercmd_t* cmd)
		//==================================================
		{
			// The server set the usercmd to the last he knows about
			int response = proxy.originalSystemCall(command, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15]);
			
			// We then alter the usercmd if needed before "sending" it back to the original game module
			GetUsercmd((int)args[0], (usercmd_t*)args[1]);
			
			return response;
		}
		//==================================================
		case G_SEND_SERVER_COMMAND: // (int clientNum, const char* text)
		//==================================================
		{
			const char* text = features::chat::TeamChat((int)args[0], (const char*)args[1]);

			if (!text)
			{
				return 0;
			}

			args[1] = (intptr_t)text;

			break;
		}
		//==================================================
		// Ghoul2 calls
		//==================================================
		// The engine uses the ghoul2 instance of the game without a test, so a null one crashes the server. The game passes
		// one when a model failed to load. Return what the engine returns for an empty instance, as OpenJK does; the bolt
		// matrix of G_G2_GETBOLT is left as it is
		//==================================================
		case G_G2_HAVEWEGHOULMODELS: // (void* ghoul2, ...) for all of these
		case G_G2_SETMODELS:
		case G_G2_GETBOLT:
		case G_G2_GETBOLT_NOREC:
		case G_G2_GETBOLT_NOREC_NOROT:
		case G_G2_SETSKIN:
		case G_G2_SIZE:
		case G_G2_SETBOLTINFO:
		case G_G2_ANGLEOVERRIDE:
		case G_G2_PLAYANIM:
		case G_G2_GETBONEANIM:
		case G_G2_GETGLANAME:
		case G_G2_SETROOTSURFACE:
		case G_G2_SETSURFACEONOFF:
		case G_G2_SETNEWORIGIN:
		case G_G2_DOESBONEEXIST:
		case G_G2_ABSURDSMOOTHING:
		case G_G2_SETRAGDOLL:
		case G_G2_ANIMATEG2MODELS:
		case G_G2_RAGPCJCONSTRAINT:
		case G_G2_RAGPCJGRADIENTSPEED:
		case G_G2_RAGEFFECTORGOAL:
		case G_G2_GETRAGBONEPOS:
		case G_G2_RAGEFFECTORKICK:
		case G_G2_RAGFORCESOLVE:
		case G_G2_SETBONEIKSTATE:
		case G_G2_IKMOVE:
		case G_G2_REMOVEBONE:
		case G_G2_ATTACHINSTANCETOENTNUM:
		case G_G2_OVERRIDESERVER:
		case G_G2_GETSURFACENAME:
		{
			if (!args[0])
			{
				return 0;
			}

			break;
		}
		case G_G2_ADDBOLT: // (void* ghoul2, ...) for both
		case G_G2_GETSURFACERENDERSTATUS:
		{
			if (!args[0])
			{
				return -1;
			}

			break;
		}
		case G_G2_COPYGHOUL2INSTANCE: // (void* g2From, void* g2To, int modelIndex)
		case G_G2_DUPLICATEGHOUL2INSTANCE: // (void* g2From, void** g2To)
		{
			if (!args[0] || !args[1])
			{
				return 0;
			}

			break;
		}
		case G_G2_COPYSPECIFICGHOUL2MODEL: // (void* g2From, int modelFrom, void* g2To, int modelTo)
		{
			if (!args[0] || !args[2])
			{
				return 0;
			}

			break;
		}
		case G_G2_COLLISIONDETECT: // (CollisionRecord_t* collRecMap, void* ghoul2, ...) for both
		case G_G2_COLLISIONDETECTCACHE:
		{
			if (!args[1])
			{
				return 0;
			}

			break;
		}
		case G_G2_HASGHOUL2MODELONINDEX: // (void** ghoul2Ptr, ...) for all of these, the engine uses *ghoul2Ptr without a test
		case G_G2_REMOVEGHOUL2MODEL:
		case G_G2_REMOVEGHOUL2MODELS:
		{
			if (!args[0] || !*(void**)args[0])
			{
				return 0;
			}

			break;
		}
		default:
			break;
	}

	return proxy.originalSystemCall(command, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15]);
}

intptr_t QDECL Call(intptr_t command, ...)
{
	intptr_t args[11];

	va_list ap;

	va_start(ap, command);

	for (std::size_t i = 0; i < sizeof(args) / sizeof(args[i]); ++i)
	{
		args[i] = va_arg(ap, intptr_t);
	}

	va_end(ap);

	return vmMain(command, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10]);
}

} // namespace ybeproxy::module::game_imports
