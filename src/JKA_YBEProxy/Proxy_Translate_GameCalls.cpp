/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// this file is only included when building a dll

#include "Proxy_Header.hpp"

static void G_InitGame(int levelTime, int randomSeed, int restart)
{

}

static void G_ShutdownGame(int restart)
{

}

static char* ClientConnect(int clientNum, qboolean firstTime, qboolean isBot)
{

}

static qboolean ClientUserinfoChanged(int clientNum)
{

}

static void ClientDisconnect(int clientNum)
{

}

static void ClientBegin(int clientNum, qboolean allowTeamReset)
{

}

static void ClientCommand(int clientNum)
{

}

static void G_RunFrame(int levelTime)
{

}

static qboolean	ConsoleCommand(void)
{

}

static int BotAIStartFrame(int time)
{

}

static void G_ROFF_NotetrackCallback(gentity_t* cent, const char* notetrack)
{

}

static void SpawnRMGEntity(void)
{

}

static int* ICARUS_PlaySound(void)
{

}

static qboolean ICARUS_Set(void)
{
}

static void ICARUS_Lerp2Pos(void)
{
}

static void ICARUS_Lerp2Origin(void)
{
}

static void ICARUS_Lerp2Angles(void)
{
}

static int ICARUS_GetTag(void)
{
}

static void ICARUS_Lerp2Start(void)
{
}

static void ICARUS_Lerp2End(void)
{
}

static void ICARUS_Use(void)
{
}

static void ICARUS_Kill(void)
{
}

static void ICARUS_Remove(void)
{
}

static void ICARUS_Play(void)
{
}

static int ICARUS_GetFloat(void)
{
}

static int ICARUS_GetVector(void)
{
}

static int ICARUS_GetString(void)
{
}

static void ICARUS_SoundIndex(void)
{
}

static int ICARUS_GetSetIDForString(void)
{
}

static qboolean NAV_ClearPathToPoint(int entID, vec3_t pmins, vec3_t pmaxs, vec3_t point, int clipmask, int okToHitEnt)
{
}

static qboolean NPC_ClearLOS2(int entID, const vec3_t end)
{
}

static int NAVNEW_ClearPathBetweenPoints(vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int ignore, int clipmask)
{
}

static qboolean NAV_CheckNodeFailedForEnt(int entID, int nodeNum)
{
}

static qboolean NAV_EntIsUnlockedDoor(int entityNum)
{
}

static qboolean NAV_EntIsDoor(int entityNum)
{
}

static qboolean NAV_EntIsBreakable(int entityNum)
{
}

static qboolean NAV_EntIsRemovableUsable(int entNum)
{
}

static void NAV_FindCombatPointWaypoints(void)
{
}

static int BG_GetItemIndexByTag(int tag, int type)
{
}

void Proxy_Translate_GameCalls(void)
{
	static gameExport_t export = { 0 };

	proxy.trap = &export;
}