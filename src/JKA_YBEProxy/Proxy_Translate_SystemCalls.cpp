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

#include "Proxy_Header.hpp"
#include "Proxy_Translate_SystemCalls.hpp"

NORETURN_PTR void (*Com_Error)(int level, const char* fmt, ...);
void (*Com_Printf)(const char* fmt, ...);

int PASSFLOAT(float x) {
	byteAlias_t fi;
	fi.f = x;
	return fi.i;
}

void trap_Print(const char* fmt) {
	proxy.originalSystemCall(G_PRINT, fmt);
}
NORETURN void trap_Error(const char* fmt) {
	proxy.originalSystemCall(G_ERROR, fmt);
	exit(1);
}
int trap_Milliseconds(void) {
	return proxy.originalSystemCall(G_MILLISECONDS);
}
void trap_PrecisionTimer_Start(void** theNewTimer) {
	proxy.originalSystemCall(G_PRECISIONTIMER_START, theNewTimer);
}
int trap_PrecisionTimer_End(void* theTimer) {
	return proxy.originalSystemCall(G_PRECISIONTIMER_END, theTimer);
}
void trap_Cvar_Register(vmCvar_t* cvar, const char* var_name, const char* value, uint32_t flags) {
	proxy.originalSystemCall(G_CVAR_REGISTER, cvar, var_name, value, flags);
}
void trap_Cvar_Update(vmCvar_t* cvar) {
	proxy.originalSystemCall(G_CVAR_UPDATE, cvar);
}
void trap_Cvar_Set(const char* var_name, const char* value) {
	proxy.originalSystemCall(G_CVAR_SET, var_name, value);
}
int trap_Cvar_VariableIntegerValue(const char* var_name) {
	return proxy.originalSystemCall(G_CVAR_VARIABLE_INTEGER_VALUE, var_name);
}
void trap_Cvar_VariableStringBuffer(const char* var_name, char* buffer, int bufsize) {
	proxy.originalSystemCall(G_CVAR_VARIABLE_STRING_BUFFER, var_name, buffer, bufsize);
}
int trap_Argc(void) {
	return proxy.originalSystemCall(G_ARGC);
}
void trap_Argv(int n, char* buffer, int bufferLength) {
	proxy.originalSystemCall(G_ARGV, n, buffer, bufferLength);
}
int trap_FS_FOpenFile(const char* qpath, fileHandle_t* f, fsMode_t mode) {
	return proxy.originalSystemCall(G_FS_FOPEN_FILE, qpath, f, mode);
}
void trap_FS_Read(void* buffer, int len, fileHandle_t f) {
	proxy.originalSystemCall(G_FS_READ, buffer, len, f);
}
void trap_FS_Write(const void* buffer, int len, fileHandle_t f) {
	proxy.originalSystemCall(G_FS_WRITE, buffer, len, f);
}
void trap_FS_FCloseFile(fileHandle_t f) {
	proxy.originalSystemCall(G_FS_FCLOSE_FILE, f);
}
void trap_SendConsoleCommand(int exec_when, const char* text) {
	proxy.originalSystemCall(G_SEND_CONSOLE_COMMAND, exec_when, text);
}
void trap_LocateGameData(sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGClient) {
	proxy.originalSystemCall(G_LOCATE_GAME_DATA, gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGClient);
}
void trap_DropClient(int clientNum, const char* reason) {
	proxy.originalSystemCall(G_DROP_CLIENT, clientNum, reason);
}
void trap_SendServerCommand(int clientNum, const char* text) {
	if (std::strlen(text) > 1022)
	{
		return;
	}

	proxy.originalSystemCall(G_SEND_SERVER_COMMAND, clientNum, text);
}
void trap_SetConfigstring(int num, const char* string) {
	proxy.originalSystemCall(G_SET_CONFIGSTRING, num, string);
}
void trap_GetConfigstring(int num, char* buffer, int bufferSize) {
	proxy.originalSystemCall(G_GET_CONFIGSTRING, num, buffer, bufferSize);
}
void trap_GetUserinfo(int num, char* buffer, int bufferSize) {
	proxy.originalSystemCall(G_GET_USERINFO, num, buffer, bufferSize);
}
void trap_SetUserinfo(int num, const char* buffer) {
	proxy.originalSystemCall(G_SET_USERINFO, num, buffer);
}
void trap_GetServerinfo(char* buffer, int bufferSize) {
	proxy.originalSystemCall(G_GET_SERVERINFO, buffer, bufferSize);
}
void trap_SetServerCull(float cullDistance) {
	proxy.originalSystemCall(G_SET_SERVER_CULL, PASSFLOAT(cullDistance));
}
void trap_SetBrushModel(sharedEntity_t* ent, const char* name) {
	proxy.originalSystemCall(G_SET_BRUSH_MODEL, ent, name);
}
void trap_Trace(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask) {
	proxy.originalSystemCall(G_TRACE, results, start, mins, maxs, end, passEntityNum, contentmask, 0, 10);
}
void trap_G2Trace(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, int g2TraceType, int traceLod) {
	proxy.originalSystemCall(G_G2TRACE, results, start, mins, maxs, end, passEntityNum, contentmask, g2TraceType, traceLod);
}
int trap_PointContents(const vec3_t point, int passEntityNum) {
	return proxy.originalSystemCall(G_POINT_CONTENTS, point, passEntityNum);
}
qboolean trap_InPVS(const vec3_t p1, const vec3_t p2) {
	return proxy.originalSystemCall(G_IN_PVS, p1, p2);
}
qboolean trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2) {
	return proxy.originalSystemCall(G_IN_PVS_IGNORE_PORTALS, p1, p2);
}
void trap_AdjustAreaPortalState(sharedEntity_t* ent, qboolean open) {
	proxy.originalSystemCall(G_ADJUST_AREA_PORTAL_STATE, ent, open);
}
qboolean trap_AreasConnected(int area1, int area2) {
	return proxy.originalSystemCall(G_AREAS_CONNECTED, area1, area2);
}
void trap_LinkEntity(sharedEntity_t* ent) {
	proxy.originalSystemCall(G_LINKENTITY, ent);
}
void trap_UnlinkEntity(sharedEntity_t* ent) {
	proxy.originalSystemCall(G_UNLINKENTITY, ent);
}
int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int* list, int maxcount) {
	return proxy.originalSystemCall(G_ENTITIES_IN_BOX, mins, maxs, list, maxcount);
}
qboolean trap_EntityContact(const vec3_t mins, const vec3_t maxs, const sharedEntity_t* ent) {
	return proxy.originalSystemCall(G_ENTITY_CONTACT, mins, maxs, ent);
}
int trap_BotAllocateClient(void) {
	return proxy.originalSystemCall(G_BOT_ALLOCATE_CLIENT);
}
void trap_BotFreeClient(int clientNum) {
	proxy.originalSystemCall(G_BOT_FREE_CLIENT, clientNum);
}
void trap_GetUsercmd(int clientNum, usercmd_t* cmd) {
	proxy.originalSystemCall(G_GET_USERCMD, clientNum, cmd);
}
qboolean trap_GetEntityToken(char* buffer, int bufferSize) {
	return proxy.originalSystemCall(G_GET_ENTITY_TOKEN, buffer, bufferSize);
}
void trap_SiegePersSet(siegePers_t* pers) {
	proxy.originalSystemCall(G_SIEGEPERSSET, pers);
}
void trap_SiegePersGet(siegePers_t* pers) {
	proxy.originalSystemCall(G_SIEGEPERSGET, pers);
}
int trap_FS_GetFileList(const char* path, const char* extension, char* listbuf, int bufsize) {
	return proxy.originalSystemCall(G_FS_GETFILELIST, path, extension, listbuf, bufsize);
}
int trap_DebugPolygonCreate(int color, int numPoints, vec3_t* points) {
	return proxy.originalSystemCall(G_DEBUG_POLYGON_CREATE, color, numPoints, points);
}
void trap_DebugPolygonDelete(int id) {
	proxy.originalSystemCall(G_DEBUG_POLYGON_DELETE, id);
}
int trap_RealTime(qtime_t* qtime) {
	return proxy.originalSystemCall(G_REAL_TIME, qtime);
}
void trap_SnapVector(float* v) {
	proxy.originalSystemCall(G_SNAPVECTOR, v);
}
void trap_TraceCapsule(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask) {
	proxy.originalSystemCall(G_TRACECAPSULE, results, start, mins, maxs, end, passEntityNum, contentmask, 0, 10);
}
qboolean trap_EntityContactCapsule(const vec3_t mins, const vec3_t maxs, const sharedEntity_t* ent) {
	return proxy.originalSystemCall(G_ENTITY_CONTACTCAPSULE, mins, maxs, ent);
}
int trap_SP_GetStringTextString(const char* text, char* buffer, int bufferLength) {
	return proxy.originalSystemCall(SP_GETSTRINGTEXTSTRING, text, buffer, bufferLength);
}
qboolean trap_ROFF_Clean(void) {
	return proxy.originalSystemCall(G_ROFF_CLEAN);
}
void trap_ROFF_UpdateEntities(void) {
	proxy.originalSystemCall(G_ROFF_UPDATE_ENTITIES);
}
int trap_ROFF_Cache(char* file) {
	return proxy.originalSystemCall(G_ROFF_CACHE, file);
}
qboolean trap_ROFF_Play(int entID, int roffID, qboolean doTranslation) {
	return proxy.originalSystemCall(G_ROFF_PLAY, entID, roffID, doTranslation);
}
qboolean trap_ROFF_Purge_Ent(int entID) {
	return proxy.originalSystemCall(G_ROFF_PURGE_ENT, entID);
}
void trap_TrueMalloc(void** ptr, int size) {
	proxy.originalSystemCall(G_TRUEMALLOC, ptr, size);
}
void trap_TrueFree(void** ptr) {
	proxy.originalSystemCall(G_TRUEFREE, ptr);
}
int trap_ICARUS_RunScript(sharedEntity_t* ent, const char* name) {
	return proxy.originalSystemCall(G_ICARUS_RUNSCRIPT, ent, name);
}
qboolean trap_ICARUS_RegisterScript(const char* name, qboolean bCalledDuringInterrogate) {
	return proxy.originalSystemCall(G_ICARUS_REGISTERSCRIPT, name, bCalledDuringInterrogate);
}
void trap_ICARUS_Init(void) {
	proxy.originalSystemCall(G_ICARUS_INIT);
}
qboolean trap_ICARUS_ValidEnt(sharedEntity_t* ent) {
	return proxy.originalSystemCall(G_ICARUS_VALIDENT, ent);
}
qboolean trap_ICARUS_IsInitialized(int entID) {
	return proxy.originalSystemCall(G_ICARUS_ISINITIALIZED, entID);
}
qboolean trap_ICARUS_MaintainTaskManager(int entID) {
	return proxy.originalSystemCall(G_ICARUS_MAINTAINTASKMANAGER, entID);
}
qboolean trap_ICARUS_IsRunning(int entID) {
	return proxy.originalSystemCall(G_ICARUS_ISRUNNING, entID);
}
qboolean trap_ICARUS_TaskIDPending(sharedEntity_t* ent, int taskID) {
	return proxy.originalSystemCall(G_ICARUS_TASKIDPENDING, ent, taskID);
}
void trap_ICARUS_InitEnt(sharedEntity_t* ent) {
	proxy.originalSystemCall(G_ICARUS_INITENT, ent);
}
void trap_ICARUS_FreeEnt(sharedEntity_t* ent) {
	proxy.originalSystemCall(G_ICARUS_FREEENT, ent);
}
void trap_ICARUS_AssociateEnt(sharedEntity_t* ent) {
	proxy.originalSystemCall(G_ICARUS_ASSOCIATEENT, ent);
}
void trap_ICARUS_Shutdown(void) {
	proxy.originalSystemCall(G_ICARUS_SHUTDOWN);
}
void trap_ICARUS_TaskIDSet(sharedEntity_t* ent, int taskType, int taskID) {
	proxy.originalSystemCall(G_ICARUS_TASKIDSET, ent, taskType, taskID);
}
void trap_ICARUS_TaskIDComplete(sharedEntity_t* ent, int taskType) {
	proxy.originalSystemCall(G_ICARUS_TASKIDCOMPLETE, ent, taskType);
}
void trap_ICARUS_SetVar(int taskID, int entID, const char* type_name, const char* data) {
	proxy.originalSystemCall(G_ICARUS_SETVAR, taskID, entID, type_name, data);
}
int trap_ICARUS_VariableDeclared(const char* type_name) {
	return proxy.originalSystemCall(G_ICARUS_VARIABLEDECLARED, type_name);
}
int trap_ICARUS_GetFloatVariable(const char* name, float* value) {
	return proxy.originalSystemCall(G_ICARUS_GETFLOATVARIABLE, name, value);
}
int trap_ICARUS_GetStringVariable(const char* name, const char* value) {
	return proxy.originalSystemCall(G_ICARUS_GETSTRINGVARIABLE, name, value);
}
int trap_ICARUS_GetVectorVariable(const char* name, const vec3_t value) {
	return proxy.originalSystemCall(G_ICARUS_GETVECTORVARIABLE, name, value);
}
void trap_Nav_Init(void) {
	proxy.originalSystemCall(G_NAV_INIT);
}
void trap_Nav_Free(void) {
	proxy.originalSystemCall(G_NAV_FREE);
}
qboolean trap_Nav_Load(const char* filename, int checksum) {
	return proxy.originalSystemCall(G_NAV_LOAD, filename, checksum);
}
qboolean trap_Nav_Save(const char* filename, int checksum) {
	return proxy.originalSystemCall(G_NAV_SAVE, filename, checksum);
}
int trap_Nav_AddRawPoint(vec3_t point, int flags, int radius) {
	return proxy.originalSystemCall(G_NAV_ADDRAWPOINT, point, flags, radius);
}
void trap_Nav_CalculatePaths(qboolean recalc) {
	proxy.originalSystemCall(G_NAV_CALCULATEPATHS, recalc);
}
void trap_Nav_HardConnect(int first, int second) {
	proxy.originalSystemCall(G_NAV_HARDCONNECT, first, second);
}
void trap_Nav_ShowNodes(void) {
	proxy.originalSystemCall(G_NAV_SHOWNODES);
}
void trap_Nav_ShowEdges(void) {
	proxy.originalSystemCall(G_NAV_SHOWEDGES);
}
void trap_Nav_ShowPath(int start, int end) {
	proxy.originalSystemCall(G_NAV_SHOWPATH, start, end);
}
int trap_Nav_GetNearestNode(sharedEntity_t* ent, int lastID, int flags, int targetID) {
	return proxy.originalSystemCall(G_NAV_GETNEARESTNODE, ent, lastID, flags, targetID);
}
int trap_Nav_GetBestNode(int startID, int endID, int rejectID) {
	return proxy.originalSystemCall(G_NAV_GETBESTNODE, startID, endID, rejectID);
}
int trap_Nav_GetNodePosition(int nodeID, vec3_t out) {
	return proxy.originalSystemCall(G_NAV_GETNODEPOSITION, nodeID, out);
}
int trap_Nav_GetNodeNumEdges(int nodeID) {
	return proxy.originalSystemCall(G_NAV_GETNODENUMEDGES, nodeID);
}
int trap_Nav_GetNodeEdge(int nodeID, int edge) {
	return proxy.originalSystemCall(G_NAV_GETNODEEDGE, nodeID, edge);
}
int trap_Nav_GetNumNodes(void) {
	return proxy.originalSystemCall(G_NAV_GETNUMNODES);
}
qboolean trap_Nav_Connected(int startID, int endID) {
	return proxy.originalSystemCall(G_NAV_CONNECTED, startID, endID);
}
int trap_Nav_GetPathCost(int startID, int endID) {
	return proxy.originalSystemCall(G_NAV_GETPATHCOST, startID, endID);
}
int trap_Nav_GetEdgeCost(int startID, int endID) {
	return proxy.originalSystemCall(G_NAV_GETEDGECOST, startID, endID);
}
int trap_Nav_GetProjectedNode(vec3_t origin, int nodeID) {
	return proxy.originalSystemCall(G_NAV_GETPROJECTEDNODE, origin, nodeID);
}
void trap_Nav_CheckFailedNodes(sharedEntity_t* ent) {
	proxy.originalSystemCall(G_NAV_CHECKFAILEDNODES, ent);
}
void trap_Nav_AddFailedNode(sharedEntity_t* ent, int nodeID) {
	proxy.originalSystemCall(G_NAV_ADDFAILEDNODE, ent, nodeID);
}
qboolean trap_Nav_NodeFailed(sharedEntity_t* ent, int nodeID) {
	return proxy.originalSystemCall(G_NAV_NODEFAILED, ent, nodeID);
}
qboolean trap_Nav_NodesAreNeighbors(int startID, int endID) {
	return proxy.originalSystemCall(G_NAV_NODESARENEIGHBORS, startID, endID);
}
void trap_Nav_ClearFailedEdge(failedEdge_t* failedEdge) {
	proxy.originalSystemCall(G_NAV_CLEARFAILEDEDGE, failedEdge);
}
void trap_Nav_ClearAllFailedEdges(void) {
	proxy.originalSystemCall(G_NAV_CLEARALLFAILEDEDGES);
}
int trap_Nav_EdgeFailed(int startID, int endID) {
	return proxy.originalSystemCall(G_NAV_EDGEFAILED, startID, endID);
}
void trap_Nav_AddFailedEdge(int entID, int startID, int endID) {
	proxy.originalSystemCall(G_NAV_ADDFAILEDEDGE, entID, startID, endID);
}
qboolean trap_Nav_CheckFailedEdge(failedEdge_t* failedEdge) {
	return proxy.originalSystemCall(G_NAV_CHECKFAILEDEDGE, failedEdge);
}
void trap_Nav_CheckAllFailedEdges(void) {
	proxy.originalSystemCall(G_NAV_CHECKALLFAILEDEDGES);
}
qboolean trap_Nav_RouteBlocked(int startID, int testEdgeID, int endID, int rejectRank) {
	return proxy.originalSystemCall(G_NAV_ROUTEBLOCKED, startID, testEdgeID, endID, rejectRank);
}
int trap_Nav_GetBestNodeAltRoute(int startID, int endID, int* pathCost, int rejectID) {
	return proxy.originalSystemCall(G_NAV_GETBESTNODEALTROUTE, startID, endID, pathCost, rejectID);
}
int trap_Nav_GetBestNodeAltRoute2(int startID, int endID, int rejectID) {
	return proxy.originalSystemCall(G_NAV_GETBESTNODEALT2, startID, endID, rejectID);
}
int trap_Nav_GetBestPathBetweenEnts(sharedEntity_t* ent, sharedEntity_t* goal, int flags) {
	return proxy.originalSystemCall(G_NAV_GETBESTPATHBETWEENENTS, ent, goal, flags);
}
int	trap_Nav_GetNodeRadius(int nodeID) {
	return proxy.originalSystemCall(G_NAV_GETNODERADIUS, nodeID);
}
void trap_Nav_CheckBlockedEdges(void) {
	proxy.originalSystemCall(G_NAV_CHECKBLOCKEDEDGES);
}
void trap_Nav_ClearCheckedNodes(void) {
	proxy.originalSystemCall(G_NAV_CLEARCHECKEDNODES);
}
int trap_Nav_CheckedNode(int wayPoint, int ent) {
	return proxy.originalSystemCall(G_NAV_CHECKEDNODE, wayPoint, ent);
}
void trap_Nav_SetCheckedNode(int wayPoint, int ent, int value) {
	proxy.originalSystemCall(G_NAV_SETCHECKEDNODE, wayPoint, ent, value);
}
void trap_Nav_FlagAllNodes(int newFlag) {
	proxy.originalSystemCall(G_NAV_FLAGALLNODES, newFlag);
}
qboolean trap_Nav_GetPathsCalculated(void) {
	return proxy.originalSystemCall(G_NAV_GETPATHSCALCULATED);
}
void trap_Nav_SetPathsCalculated(qboolean newVal) {
	proxy.originalSystemCall(G_NAV_SETPATHSCALCULATED, newVal);
}
void trap_SV_RegisterSharedMemory(char* memory) {
	proxy.originalSystemCall(G_SET_SHARED_BUFFER, memory);
}
int trap_BotLibSetup(void) {
	return proxy.originalSystemCall(BOTLIB_SETUP);
}
int trap_BotLibShutdown(void) {
	return proxy.originalSystemCall(BOTLIB_SHUTDOWN);
}
int trap_BotLibVarSet(char* var_name, char* value) {
	return proxy.originalSystemCall(BOTLIB_LIBVAR_SET, var_name, value);
}
int trap_BotLibVarGet(char* var_name, char* value, int size) {
	return proxy.originalSystemCall(BOTLIB_LIBVAR_GET, var_name, value, size);
}
int trap_BotLibDefine(char* string) {
	return proxy.originalSystemCall(BOTLIB_PC_ADD_GLOBAL_DEFINE, string);
}
int trap_BotLibStartFrame(float time) {
	return proxy.originalSystemCall(BOTLIB_START_FRAME, PASSFLOAT(time));
}
int trap_BotLibLoadMap(const char* mapname) {
	return proxy.originalSystemCall(BOTLIB_LOAD_MAP, mapname);
}
int trap_BotLibUpdateEntity(int ent, void* bue) {
	return proxy.originalSystemCall(BOTLIB_UPDATENTITY, ent, bue);
}
int trap_BotLibTest(int parm0, char* parm1, vec3_t parm2, vec3_t parm3) {
	return proxy.originalSystemCall(BOTLIB_TEST, parm0, parm1, parm2, parm3);
}
int trap_BotGetSnapshotEntity(int clientNum, int sequence) {
	return proxy.originalSystemCall(BOTLIB_GET_SNAPSHOT_ENTITY, clientNum, sequence);
}
int trap_BotGetServerCommand(int clientNum, char* message, int size) {
	return proxy.originalSystemCall(BOTLIB_GET_CONSOLE_MESSAGE, clientNum, message, size);
}
void trap_BotUserCommand(int clientNum, usercmd_t* ucmd) {
	proxy.originalSystemCall(BOTLIB_USER_COMMAND, clientNum, ucmd);
}
void trap_AAS_EntityInfo(int entnum, void* info) {
	proxy.originalSystemCall(BOTLIB_AAS_ENTITY_INFO, entnum, info);
}
int trap_AAS_Initialized(void) {
	return proxy.originalSystemCall(BOTLIB_AAS_INITIALIZED);
}
void trap_AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs) {
	proxy.originalSystemCall(BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX, presencetype, mins, maxs);
}
float trap_AAS_Time(void) {
	byteAlias_t fi;
	fi.i = proxy.originalSystemCall(BOTLIB_AAS_TIME);
	return fi.f;
}
int trap_AAS_PointAreaNum(vec3_t point) {
	return proxy.originalSystemCall(BOTLIB_AAS_POINT_AREA_NUM, point);
}
int trap_AAS_PointReachabilityAreaIndex(vec3_t point) {
	return proxy.originalSystemCall(BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX, point);
}
int trap_AAS_TraceAreas(vec3_t start, vec3_t end, int* areas, vec3_t* points, int maxareas) {
	return proxy.originalSystemCall(BOTLIB_AAS_TRACE_AREAS, start, end, areas, points, maxareas);
}
int trap_AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int* areas, int maxareas) {
	return proxy.originalSystemCall(BOTLIB_AAS_BBOX_AREAS, absmins, absmaxs, areas, maxareas);
}
int trap_AAS_AreaInfo(int areanum, void* info) {
	return proxy.originalSystemCall(BOTLIB_AAS_AREA_INFO, areanum, info);
}
int trap_AAS_PointContents(vec3_t point) {
	return proxy.originalSystemCall(BOTLIB_AAS_POINT_CONTENTS, point);
}
int trap_AAS_NextBSPEntity(int ent) {
	return proxy.originalSystemCall(BOTLIB_AAS_NEXT_BSP_ENTITY, ent);
}
int trap_AAS_ValueForBSPEpairKey(int ent, char* key, char* value, int size) {
	return proxy.originalSystemCall(BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY, ent, key, value, size);
}
int trap_AAS_VectorForBSPEpairKey(int ent, char* key, vec3_t v) {
	return proxy.originalSystemCall(BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY, ent, key, v);
}
int trap_AAS_FloatForBSPEpairKey(int ent, char* key, float* value) {
	return proxy.originalSystemCall(BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY, ent, key, value);
}
int trap_AAS_IntForBSPEpairKey(int ent, char* key, int* value) {
	return proxy.originalSystemCall(BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY, ent, key, value);
}
int trap_AAS_AreaReachability(int areanum) {
	return proxy.originalSystemCall(BOTLIB_AAS_AREA_REACHABILITY, areanum);
}
int trap_AAS_AreaTravelTimeToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags) {
	return proxy.originalSystemCall(BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA, areanum, origin, goalareanum, travelflags);
}
int trap_AAS_EnableRoutingArea(int areanum, int enable) {
	return proxy.originalSystemCall(BOTLIB_AAS_ENABLE_ROUTING_AREA, areanum, enable);
}
int trap_AAS_PredictRoute(void* route, int areanum, vec3_t origin, int goalareanum, int travelflags, int maxareas, int maxtime, int stopevent, int stopcontents, int stoptfl, int stopareanum) {
	return proxy.originalSystemCall(BOTLIB_AAS_PREDICT_ROUTE, route, areanum, origin, goalareanum, travelflags, maxareas, maxtime, stopevent, stopcontents, stoptfl, stopareanum);
}
int trap_AAS_AlternativeRouteGoals(vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags, void* altroutegoals, int maxaltroutegoals, int type) {
	return proxy.originalSystemCall(BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL, start, startareanum, goal, goalareanum, travelflags, altroutegoals, maxaltroutegoals, type);
}
int trap_AAS_Swimming(vec3_t origin) {
	return proxy.originalSystemCall(BOTLIB_AAS_SWIMMING, origin);
}
int trap_AAS_PredictClientMovement(void* move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize) {
	return proxy.originalSystemCall(BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT, move, entnum, origin, presencetype, onground, velocity, cmdmove, cmdframes, maxframes, PASSFLOAT(frametime), stopevent, stopareanum, visualize);
}
void trap_EA_Say(int client, char* str) {
	proxy.originalSystemCall(BOTLIB_EA_SAY, client, str);
}
void trap_EA_SayTeam(int client, char* str) {
	proxy.originalSystemCall(BOTLIB_EA_SAY_TEAM, client, str);
}
void trap_EA_Command(int client, char* command) {
	proxy.originalSystemCall(BOTLIB_EA_COMMAND, client, command);
}
void trap_EA_Action(int client, int action) {
	proxy.originalSystemCall(BOTLIB_EA_ACTION, client, action);
}
void trap_EA_Gesture(int client) {
	proxy.originalSystemCall(BOTLIB_EA_GESTURE, client);
}
void trap_EA_Talk(int client) {
	proxy.originalSystemCall(BOTLIB_EA_TALK, client);
}
void trap_EA_Attack(int client) {
	proxy.originalSystemCall(BOTLIB_EA_ATTACK, client);
}
void trap_EA_Alt_Attack(int client) {
	proxy.originalSystemCall(BOTLIB_EA_ALT_ATTACK, client);
}
void trap_EA_ForcePower(int client) {
	proxy.originalSystemCall(BOTLIB_EA_FORCEPOWER, client);
}
void trap_EA_Use(int client) {
	proxy.originalSystemCall(BOTLIB_EA_USE, client);
}
void trap_EA_Respawn(int client) {
	proxy.originalSystemCall(BOTLIB_EA_RESPAWN, client);
}
void trap_EA_Crouch(int client) {
	proxy.originalSystemCall(BOTLIB_EA_CROUCH, client);
}
void trap_EA_MoveUp(int client) {
	proxy.originalSystemCall(BOTLIB_EA_MOVE_UP, client);
}
void trap_EA_MoveDown(int client) {
	proxy.originalSystemCall(BOTLIB_EA_MOVE_DOWN, client);
}
void trap_EA_MoveForward(int client) {
	proxy.originalSystemCall(BOTLIB_EA_MOVE_FORWARD, client);
}
void trap_EA_MoveBack(int client) {
	proxy.originalSystemCall(BOTLIB_EA_MOVE_BACK, client);
}
void trap_EA_MoveLeft(int client) {
	proxy.originalSystemCall(BOTLIB_EA_MOVE_LEFT, client);
}
void trap_EA_MoveRight(int client) {
	proxy.originalSystemCall(BOTLIB_EA_MOVE_RIGHT, client);
}
void trap_EA_SelectWeapon(int client, int weapon) {
	proxy.originalSystemCall(BOTLIB_EA_SELECT_WEAPON, client, weapon);
}
void trap_EA_Jump(int client) {
	proxy.originalSystemCall(BOTLIB_EA_JUMP, client);
}
void trap_EA_DelayedJump(int client) {
	proxy.originalSystemCall(BOTLIB_EA_DELAYED_JUMP, client);
}
void trap_EA_Move(int client, vec3_t dir, float speed) {
	proxy.originalSystemCall(BOTLIB_EA_MOVE, client, dir, PASSFLOAT(speed));
}
void trap_EA_View(int client, vec3_t viewangles) {
	proxy.originalSystemCall(BOTLIB_EA_VIEW, client, viewangles);
}
void trap_EA_EndRegular(int client, float thinktime) {
	proxy.originalSystemCall(BOTLIB_EA_END_REGULAR, client, PASSFLOAT(thinktime));
}
void trap_EA_GetInput(int client, float thinktime, void* input) {
	proxy.originalSystemCall(BOTLIB_EA_GET_INPUT, client, PASSFLOAT(thinktime), input);
}
void trap_EA_ResetInput(int client) {
	proxy.originalSystemCall(BOTLIB_EA_RESET_INPUT, client);
}
int trap_BotLoadCharacter(char* charfile, float skill) {
	return proxy.originalSystemCall(BOTLIB_AI_LOAD_CHARACTER, charfile, PASSFLOAT(skill));
}
void trap_BotFreeCharacter(int character) {
	proxy.originalSystemCall(BOTLIB_AI_FREE_CHARACTER, character);
}
float trap_Characteristic_Float(int character, int index) {
	byteAlias_t fi;
	fi.i = proxy.originalSystemCall(BOTLIB_AI_CHARACTERISTIC_FLOAT, character, index);
	return fi.f;
}
float trap_Characteristic_BFloat(int character, int index, float min, float max) {
	byteAlias_t fi;
	fi.i = proxy.originalSystemCall(BOTLIB_AI_CHARACTERISTIC_BFLOAT, character, index, PASSFLOAT(min), PASSFLOAT(max));
	return fi.f;
}
int trap_Characteristic_Integer(int character, int index) {
	return proxy.originalSystemCall(BOTLIB_AI_CHARACTERISTIC_INTEGER, character, index);
}
int trap_Characteristic_BInteger(int character, int index, int min, int max) {
	return proxy.originalSystemCall(BOTLIB_AI_CHARACTERISTIC_BINTEGER, character, index, min, max);
}
void trap_Characteristic_String(int character, int index, char* buf, int size) {
	proxy.originalSystemCall(BOTLIB_AI_CHARACTERISTIC_STRING, character, index, buf, size);
}
int trap_BotAllocChatState(void) {
	return proxy.originalSystemCall(BOTLIB_AI_ALLOC_CHAT_STATE);
}
void trap_BotFreeChatState(int handle) {
	proxy.originalSystemCall(BOTLIB_AI_FREE_CHAT_STATE, handle);
}
void trap_BotQueueConsoleMessage(int chatstate, int type, char* message) {
	proxy.originalSystemCall(BOTLIB_AI_QUEUE_CONSOLE_MESSAGE, chatstate, type, message);
}
void trap_BotRemoveConsoleMessage(int chatstate, int handle) {
	proxy.originalSystemCall(BOTLIB_AI_REMOVE_CONSOLE_MESSAGE, chatstate, handle);
}
int trap_BotNextConsoleMessage(int chatstate, void* cm) {
	return proxy.originalSystemCall(BOTLIB_AI_NEXT_CONSOLE_MESSAGE, chatstate, cm);
}
int trap_BotNumConsoleMessages(int chatstate) {
	return proxy.originalSystemCall(BOTLIB_AI_NUM_CONSOLE_MESSAGE, chatstate);
}
void trap_BotInitialChat(int chatstate, char* type, int mcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7) {
	proxy.originalSystemCall(BOTLIB_AI_INITIAL_CHAT, chatstate, type, mcontext, var0, var1, var2, var3, var4, var5, var6, var7);
}
int	trap_BotNumInitialChats(int chatstate, char* type) {
	return proxy.originalSystemCall(BOTLIB_AI_NUM_INITIAL_CHATS, chatstate, type);
}
int trap_BotReplyChat(int chatstate, char* message, int mcontext, int vcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7) {
	return proxy.originalSystemCall(BOTLIB_AI_REPLY_CHAT, chatstate, message, mcontext, vcontext, var0, var1, var2, var3, var4, var5, var6, var7);
}
int trap_BotChatLength(int chatstate) {
	return proxy.originalSystemCall(BOTLIB_AI_CHAT_LENGTH, chatstate);
}
void trap_BotEnterChat(int chatstate, int client, int sendto) {
	proxy.originalSystemCall(BOTLIB_AI_ENTER_CHAT, chatstate, client, sendto);
}
void trap_BotGetChatMessage(int chatstate, char* buf, int size) {
	proxy.originalSystemCall(BOTLIB_AI_GET_CHAT_MESSAGE, chatstate, buf, size);
}
int trap_StringContains(char* str1, char* str2, int casesensitive) {
	return proxy.originalSystemCall(BOTLIB_AI_STRING_CONTAINS, str1, str2, casesensitive);
}
int trap_BotFindMatch(char* str, void* match, unsigned long int context) {
	return proxy.originalSystemCall(BOTLIB_AI_FIND_MATCH, str, match, context);
}
void trap_BotMatchVariable(void* match, int variable, char* buf, int size) {
	proxy.originalSystemCall(BOTLIB_AI_MATCH_VARIABLE, match, variable, buf, size);
}
void trap_UnifyWhiteSpaces(char* string) {
	proxy.originalSystemCall(BOTLIB_AI_UNIFY_WHITE_SPACES, string);
}
void trap_BotReplaceSynonyms(char* string, unsigned long int context) {
	proxy.originalSystemCall(BOTLIB_AI_REPLACE_SYNONYMS, string, context);
}
int trap_BotLoadChatFile(int chatstate, char* chatfile, char* chatname) {
	return proxy.originalSystemCall(BOTLIB_AI_LOAD_CHAT_FILE, chatstate, chatfile, chatname);
}
void trap_BotSetChatGender(int chatstate, int gender) {
	proxy.originalSystemCall(BOTLIB_AI_SET_CHAT_GENDER, chatstate, gender);
}
void trap_BotSetChatName(int chatstate, char* name, int client) {
	proxy.originalSystemCall(BOTLIB_AI_SET_CHAT_NAME, chatstate, name, client);
}
void trap_BotResetGoalState(int goalstate) {
	proxy.originalSystemCall(BOTLIB_AI_RESET_GOAL_STATE, goalstate);
}
void trap_BotResetAvoidGoals(int goalstate) {
	proxy.originalSystemCall(BOTLIB_AI_RESET_AVOID_GOALS, goalstate);
}
void trap_BotRemoveFromAvoidGoals(int goalstate, int number) {
	proxy.originalSystemCall(BOTLIB_AI_REMOVE_FROM_AVOID_GOALS, goalstate, number);
}
void trap_BotPushGoal(int goalstate, void* goal) {
	proxy.originalSystemCall(BOTLIB_AI_PUSH_GOAL, goalstate, goal);
}
void trap_BotPopGoal(int goalstate) {
	proxy.originalSystemCall(BOTLIB_AI_POP_GOAL, goalstate);
}
void trap_BotEmptyGoalStack(int goalstate) {
	proxy.originalSystemCall(BOTLIB_AI_EMPTY_GOAL_STACK, goalstate);
}
void trap_BotDumpAvoidGoals(int goalstate) {
	proxy.originalSystemCall(BOTLIB_AI_DUMP_AVOID_GOALS, goalstate);
}
void trap_BotDumpGoalStack(int goalstate) {
	proxy.originalSystemCall(BOTLIB_AI_DUMP_GOAL_STACK, goalstate);
}
void trap_BotGoalName(int number, char* name, int size) {
	proxy.originalSystemCall(BOTLIB_AI_GOAL_NAME, number, name, size);
}
int trap_BotGetTopGoal(int goalstate, void* goal) {
	return proxy.originalSystemCall(BOTLIB_AI_GET_TOP_GOAL, goalstate, goal);
}
int trap_BotGetSecondGoal(int goalstate, void* goal) {
	return proxy.originalSystemCall(BOTLIB_AI_GET_SECOND_GOAL, goalstate, goal);
}
int trap_BotChooseLTGItem(int goalstate, vec3_t origin, int* inventory, int travelflags) {
	return proxy.originalSystemCall(BOTLIB_AI_CHOOSE_LTG_ITEM, goalstate, origin, inventory, travelflags);
}
int trap_BotChooseNBGItem(int goalstate, vec3_t origin, int* inventory, int travelflags, void* ltg, float maxtime) {
	return proxy.originalSystemCall(BOTLIB_AI_CHOOSE_NBG_ITEM, goalstate, origin, inventory, travelflags, ltg, PASSFLOAT(maxtime));
}
int trap_BotTouchingGoal(vec3_t origin, void* goal) {
	return proxy.originalSystemCall(BOTLIB_AI_TOUCHING_GOAL, origin, goal);
}
int trap_BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, void* goal) {
	return proxy.originalSystemCall(BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE, viewer, eye, viewangles, goal);
}
int trap_BotGetLevelItemGoal(int index, char* classname, void* goal) {
	return proxy.originalSystemCall(BOTLIB_AI_GET_LEVEL_ITEM_GOAL, index, classname, goal);
}
int trap_BotGetNextCampSpotGoal(int num, void* goal) {
	return proxy.originalSystemCall(BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL, num, goal);
}
int trap_BotGetMapLocationGoal(char* name, void* goal) {
	return proxy.originalSystemCall(BOTLIB_AI_GET_MAP_LOCATION_GOAL, name, goal);
}
float trap_BotAvoidGoalTime(int goalstate, int number) {
	byteAlias_t fi;
	fi.i = proxy.originalSystemCall(BOTLIB_AI_AVOID_GOAL_TIME, goalstate, number);
	return fi.f;
}
void trap_BotSetAvoidGoalTime(int goalstate, int number, float avoidtime) {
	proxy.originalSystemCall(BOTLIB_AI_SET_AVOID_GOAL_TIME, goalstate, number, PASSFLOAT(avoidtime));
}
void trap_BotInitLevelItems(void) {
	proxy.originalSystemCall(BOTLIB_AI_INIT_LEVEL_ITEMS);
}
void trap_BotUpdateEntityItems(void) {
	proxy.originalSystemCall(BOTLIB_AI_UPDATE_ENTITY_ITEMS);
}
int trap_BotLoadItemWeights(int goalstate, char* filename) {
	return proxy.originalSystemCall(BOTLIB_AI_LOAD_ITEM_WEIGHTS, goalstate, filename);
}
void trap_BotFreeItemWeights(int goalstate) {
	proxy.originalSystemCall(BOTLIB_AI_FREE_ITEM_WEIGHTS, goalstate);
}
void trap_BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child) {
	proxy.originalSystemCall(BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC, parent1, parent2, child);
}
void trap_BotSaveGoalFuzzyLogic(int goalstate, char* filename) {
	proxy.originalSystemCall(BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC, goalstate, filename);
}
void trap_BotMutateGoalFuzzyLogic(int goalstate, float range) {
	proxy.originalSystemCall(BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC, goalstate, PASSFLOAT(range));
}
int trap_BotAllocGoalState(int state) {
	return proxy.originalSystemCall(BOTLIB_AI_ALLOC_GOAL_STATE, state);
}
void trap_BotFreeGoalState(int handle) {
	proxy.originalSystemCall(BOTLIB_AI_FREE_GOAL_STATE, handle);
}
void trap_BotResetMoveState(int movestate) {
	proxy.originalSystemCall(BOTLIB_AI_RESET_MOVE_STATE, movestate);
}
void trap_BotAddAvoidSpot(int movestate, vec3_t origin, float radius, int type) {
	proxy.originalSystemCall(BOTLIB_AI_ADD_AVOID_SPOT, movestate, origin, PASSFLOAT(radius), type);
}
void trap_BotMoveToGoal(void* result, int movestate, void* goal, int travelflags) {
	proxy.originalSystemCall(BOTLIB_AI_MOVE_TO_GOAL, result, movestate, goal, travelflags);
}
int trap_BotMoveInDirection(int movestate, vec3_t dir, float speed, int type) {
	return proxy.originalSystemCall(BOTLIB_AI_MOVE_IN_DIRECTION, movestate, dir, PASSFLOAT(speed), type);
}
void trap_BotResetAvoidReach(int movestate) {
	proxy.originalSystemCall(BOTLIB_AI_RESET_AVOID_REACH, movestate);
}
void trap_BotResetLastAvoidReach(int movestate) {
	proxy.originalSystemCall(BOTLIB_AI_RESET_LAST_AVOID_REACH, movestate);
}
int trap_BotReachabilityArea(vec3_t origin, int testground) {
	return proxy.originalSystemCall(BOTLIB_AI_REACHABILITY_AREA, origin, testground);
}
int trap_BotMovementViewTarget(int movestate, void* goal, int travelflags, float lookahead, vec3_t target) {
	return proxy.originalSystemCall(BOTLIB_AI_MOVEMENT_VIEW_TARGET, movestate, goal, travelflags, PASSFLOAT(lookahead), target);
}
int trap_BotPredictVisiblePosition(vec3_t origin, int areanum, void* goal, int travelflags, vec3_t target) {
	return proxy.originalSystemCall(BOTLIB_AI_PREDICT_VISIBLE_POSITION, origin, areanum, goal, travelflags, target);
}
int trap_BotAllocMoveState(void) {
	return proxy.originalSystemCall(BOTLIB_AI_ALLOC_MOVE_STATE);
}
void trap_BotFreeMoveState(int handle) {
	proxy.originalSystemCall(BOTLIB_AI_FREE_MOVE_STATE, handle);
}
void trap_BotInitMoveState(int handle, void* initmove) {
	proxy.originalSystemCall(BOTLIB_AI_INIT_MOVE_STATE, handle, initmove);
}
int trap_BotChooseBestFightWeapon(int weaponstate, int* inventory) {
	return proxy.originalSystemCall(BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON, weaponstate, inventory);
}
void trap_BotGetWeaponInfo(int weaponstate, int weapon, void* weaponinfo) {
	proxy.originalSystemCall(BOTLIB_AI_GET_WEAPON_INFO, weaponstate, weapon, weaponinfo);
}
int trap_BotLoadWeaponWeights(int weaponstate, char* filename) {
	return proxy.originalSystemCall(BOTLIB_AI_LOAD_WEAPON_WEIGHTS, weaponstate, filename);
}
int trap_BotAllocWeaponState(void) {
	return proxy.originalSystemCall(BOTLIB_AI_ALLOC_WEAPON_STATE);
}
void trap_BotFreeWeaponState(int weaponstate) {
	proxy.originalSystemCall(BOTLIB_AI_FREE_WEAPON_STATE, weaponstate);
}
void trap_BotResetWeaponState(int weaponstate) {
	proxy.originalSystemCall(BOTLIB_AI_RESET_WEAPON_STATE, weaponstate);
}
int trap_GeneticParentsAndChildSelection(int numranks, float* ranks, int* parent1, int* parent2, int* child) {
	return proxy.originalSystemCall(BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION, numranks, ranks, parent1, parent2, child);
}
int trap_PC_LoadSource(const char* filename) {
	return proxy.originalSystemCall(BOTLIB_PC_LOAD_SOURCE, filename);
}
int trap_PC_FreeSource(int handle) {
	return proxy.originalSystemCall(BOTLIB_PC_FREE_SOURCE, handle);
}
int trap_PC_ReadToken(int handle, pc_token_t* pc_token) {
	return proxy.originalSystemCall(BOTLIB_PC_READ_TOKEN, handle, pc_token);
}
int trap_PC_SourceFileAndLine(int handle, char* filename, int* line) {
	return proxy.originalSystemCall(BOTLIB_PC_SOURCE_FILE_AND_LINE, handle, filename, line);
}
qhandle_t trap_R_RegisterSkin(const char* name) {
	return proxy.originalSystemCall(G_R_REGISTERSKIN, name);
}
void trap_G2_ListModelBones(void* ghlInfo, int frame) {
	proxy.originalSystemCall(G_G2_LISTBONES, ghlInfo, frame);
}
void trap_G2_ListModelSurfaces(void* ghlInfo) {
	proxy.originalSystemCall(G_G2_LISTSURFACES, ghlInfo);
}
qboolean trap_G2_HaveWeGhoul2Models(void* ghoul2) {
	return (qboolean)(proxy.originalSystemCall(G_G2_HAVEWEGHOULMODELS, ghoul2));
}
void trap_G2_SetGhoul2ModelIndexes(void* ghoul2, qhandle_t* modelList, qhandle_t* skinList) {
	proxy.originalSystemCall(G_G2_SETMODELS, ghoul2, modelList, skinList);
}
qboolean trap_G2API_GetBoltMatrix(void* ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t* matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t* modelList, vec3_t scale) {
	return (qboolean)(proxy.originalSystemCall(G_G2_GETBOLT, ghoul2, modelIndex, boltIndex, matrix, angles, position, frameNum, modelList, scale));
}
qboolean trap_G2API_GetBoltMatrix_NoReconstruct(void* ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t* matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t* modelList, vec3_t scale) {
	return (qboolean)(proxy.originalSystemCall(G_G2_GETBOLT_NOREC, ghoul2, modelIndex, boltIndex, matrix, angles, position, frameNum, modelList, scale));
}
qboolean trap_G2API_GetBoltMatrix_NoRecNoRot(void* ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t* matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t* modelList, vec3_t scale) {
	return (qboolean)(proxy.originalSystemCall(G_G2_GETBOLT_NOREC_NOROT, ghoul2, modelIndex, boltIndex, matrix, angles, position, frameNum, modelList, scale));
}
int trap_G2API_InitGhoul2Model(void** ghoul2Ptr, const char* fileName, int modelIndex, qhandle_t customSkin, qhandle_t customShader, int modelFlags, int lodBias) {
	return proxy.originalSystemCall(G_G2_INITGHOUL2MODEL, ghoul2Ptr, fileName, modelIndex, customSkin, customShader, modelFlags, lodBias);
}
qboolean trap_G2API_SetSkin(void* ghoul2, int modelIndex, qhandle_t customSkin, qhandle_t renderSkin) {
	return proxy.originalSystemCall(G_G2_SETSKIN, ghoul2, modelIndex, customSkin, renderSkin);
}
int trap_G2API_Ghoul2Size(void* ghlInfo) {
	return proxy.originalSystemCall(G_G2_SIZE, ghlInfo);
}
int trap_G2API_AddBolt(void* ghoul2, int modelIndex, const char* boneName) {
	return proxy.originalSystemCall(G_G2_ADDBOLT, ghoul2, modelIndex, boneName);
}
void trap_G2API_SetBoltInfo(void* ghoul2, int modelIndex, int boltInfo) {
	proxy.originalSystemCall(G_G2_SETBOLTINFO, ghoul2, modelIndex, boltInfo);
}
qboolean trap_G2API_SetBoneAngles(void* ghoul2, int modelIndex, const char* boneName, const vec3_t angles, const int flags, const int up, const int right, const int forward, qhandle_t* modelList, int blendTime, int currentTime) {
	return (proxy.originalSystemCall(G_G2_ANGLEOVERRIDE, ghoul2, modelIndex, boneName, angles, flags, up, right, forward, modelList, blendTime, currentTime));
}
qboolean trap_G2API_SetBoneAnim(void* ghoul2, const int modelIndex, const char* boneName, const int startFrame, const int endFrame, const int flags, const float animSpeed, const int currentTime, const float setFrame, const int blendTime) {
	return proxy.originalSystemCall(G_G2_PLAYANIM, ghoul2, modelIndex, boneName, startFrame, endFrame, flags, PASSFLOAT(animSpeed), currentTime, PASSFLOAT(setFrame), blendTime);
}
qboolean trap_G2API_GetBoneAnim(void* ghoul2, const char* boneName, const int currentTime, float* currentFrame, int* startFrame, int* endFrame, int* flags, float* animSpeed, int* modelList, const int modelIndex) {
	return proxy.originalSystemCall(G_G2_GETBONEANIM, ghoul2, boneName, currentTime, currentFrame, startFrame, endFrame, flags, animSpeed, modelList, modelIndex);
}
void trap_G2API_GetGLAName(void* ghoul2, int modelIndex, char* fillBuf) {
	proxy.originalSystemCall(G_G2_GETGLANAME, ghoul2, modelIndex, fillBuf);
}
int trap_G2API_CopyGhoul2Instance(void* g2From, void* g2To, int modelIndex) {
	return proxy.originalSystemCall(G_G2_COPYGHOUL2INSTANCE, g2From, g2To, modelIndex);
}
void trap_G2API_CopySpecificGhoul2Model(void* g2From, int modelFrom, void* g2To, int modelTo) {
	proxy.originalSystemCall(G_G2_COPYSPECIFICGHOUL2MODEL, g2From, modelFrom, g2To, modelTo);
}
void trap_G2API_DuplicateGhoul2Instance(void* g2From, void** g2To) {
	proxy.originalSystemCall(G_G2_DUPLICATEGHOUL2INSTANCE, g2From, g2To);
}
qboolean trap_G2API_HasGhoul2ModelOnIndex(void* ghlInfo, int modelIndex) {
	return proxy.originalSystemCall(G_G2_HASGHOUL2MODELONINDEX, ghlInfo, modelIndex);
}
qboolean trap_G2API_RemoveGhoul2Model(void* ghlInfo, int modelIndex) {
	return proxy.originalSystemCall(G_G2_REMOVEGHOUL2MODEL, ghlInfo, modelIndex);
}
qboolean trap_G2API_RemoveGhoul2Models(void* ghlInfo) {
	return proxy.originalSystemCall(G_G2_REMOVEGHOUL2MODELS, ghlInfo);
}
void trap_G2API_CleanGhoul2Models(void** ghoul2Ptr) {
	proxy.originalSystemCall(G_G2_CLEANMODELS, ghoul2Ptr);
}
void trap_G2API_CollisionDetect(CollisionRecord_t* collRecMap, void* ghoul2, const vec3_t angles, const vec3_t position, int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, int traceFlags, int useLod, float fRadius) {
	proxy.originalSystemCall(G_G2_COLLISIONDETECT, collRecMap, ghoul2, angles, position, frameNumber, entNum, rayStart, rayEnd, scale, traceFlags, useLod, PASSFLOAT(fRadius));
}
void trap_G2API_CollisionDetectCache(CollisionRecord_t* collRecMap, void* ghoul2, const vec3_t angles, const vec3_t position, int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, int traceFlags, int useLod, float fRadius) {
	proxy.originalSystemCall(G_G2_COLLISIONDETECTCACHE, collRecMap, ghoul2, angles, position, frameNumber, entNum, rayStart, rayEnd, scale, traceFlags, useLod, PASSFLOAT(fRadius));
}
void trap_G2API_GetSurfaceName(void* ghoul2, int surfNumber, int modelIndex, char* fillBuf) {
	proxy.originalSystemCall(G_G2_GETSURFACENAME, ghoul2, surfNumber, modelIndex, fillBuf);
}
qboolean trap_G2API_SetRootSurface(void* ghoul2, const int modelIndex, const char* surfaceName) {
	return proxy.originalSystemCall(G_G2_SETROOTSURFACE, ghoul2, modelIndex, surfaceName);
}
qboolean trap_G2API_SetSurfaceOnOff(void* ghoul2, const char* surfaceName, const int flags) {
	return proxy.originalSystemCall(G_G2_SETSURFACEONOFF, ghoul2, surfaceName, flags);
}
qboolean trap_G2API_SetNewOrigin(void* ghoul2, const int boltIndex) {
	return proxy.originalSystemCall(G_G2_SETNEWORIGIN, ghoul2, boltIndex);
}
qboolean trap_G2API_DoesBoneExist(void* ghoul2, int modelIndex, const char* boneName) {
	return proxy.originalSystemCall(G_G2_DOESBONEEXIST, ghoul2, modelIndex, boneName);
}
int trap_G2API_GetSurfaceRenderStatus(void* ghoul2, const int modelIndex, const char* surfaceName) {
	return proxy.originalSystemCall(G_G2_GETSURFACERENDERSTATUS, ghoul2, modelIndex, surfaceName);
}
void trap_G2API_AbsurdSmoothing(void* ghoul2, qboolean status) {
	proxy.originalSystemCall(G_G2_ABSURDSMOOTHING, ghoul2, status);
}
void trap_G2API_SetRagDoll(void* ghoul2, sharedRagDollParams_t* params) {
	proxy.originalSystemCall(G_G2_SETRAGDOLL, ghoul2, params);
}
void trap_G2API_AnimateG2Models(void* ghoul2, int time, sharedRagDollUpdateParams_t* params) {
	proxy.originalSystemCall(G_G2_ANIMATEG2MODELS, ghoul2, time, params);
}
qboolean trap_G2API_RagPCJConstraint(void* ghoul2, const char* boneName, vec3_t min, vec3_t max) {
	return proxy.originalSystemCall(G_G2_RAGPCJCONSTRAINT, ghoul2, boneName, min, max);
}
qboolean trap_G2API_RagPCJGradientSpeed(void* ghoul2, const char* boneName, const float speed) {
	return proxy.originalSystemCall(G_G2_RAGPCJGRADIENTSPEED, ghoul2, boneName, PASSFLOAT(speed));
}
qboolean trap_G2API_RagEffectorGoal(void* ghoul2, const char* boneName, vec3_t pos) {
	return proxy.originalSystemCall(G_G2_RAGEFFECTORGOAL, ghoul2, boneName, pos);
}
qboolean trap_G2API_GetRagBonePos(void* ghoul2, const char* boneName, vec3_t pos, vec3_t entAngles, vec3_t entPos, vec3_t entScale) {
	return proxy.originalSystemCall(G_G2_GETRAGBONEPOS, ghoul2, boneName, pos, entAngles, entPos, entScale);
}
qboolean trap_G2API_RagEffectorKick(void* ghoul2, const char* boneName, vec3_t velocity) {
	return proxy.originalSystemCall(G_G2_RAGEFFECTORKICK, ghoul2, boneName, velocity);
}
qboolean trap_G2API_RagForceSolve(void* ghoul2, qboolean force) {
	return proxy.originalSystemCall(G_G2_RAGFORCESOLVE, ghoul2, force);
}
qboolean trap_G2API_SetBoneIKState(void* ghoul2, int time, const char* boneName, int ikState, sharedSetBoneIKStateParams_t* params) {
	return proxy.originalSystemCall(G_G2_SETBONEIKSTATE, ghoul2, time, boneName, ikState, params);
}
qboolean trap_G2API_IKMove(void* ghoul2, int time, sharedIKMoveParams_t* params) {
	return proxy.originalSystemCall(G_G2_IKMOVE, ghoul2, time, params);
}
qboolean trap_G2API_RemoveBone(void* ghoul2, const char* boneName, int modelIndex) {
	return proxy.originalSystemCall(G_G2_REMOVEBONE, ghoul2, boneName, modelIndex);
}
void trap_G2API_AttachInstanceToEntNum(void* ghoul2, int entityNum, qboolean serverb) {
	proxy.originalSystemCall(G_G2_ATTACHINSTANCETOENTNUM, ghoul2, entityNum, serverb);
}
void trap_G2API_ClearAttachedInstance(int entityNum) {
	proxy.originalSystemCall(G_G2_CLEARATTACHEDINSTANCE, entityNum);
}
void trap_G2API_CleanEntAttachments(void) {
	proxy.originalSystemCall(G_G2_CLEANENTATTACHMENTS);
}
qboolean trap_G2API_OverrideServer(void* serverInstance) {
	return proxy.originalSystemCall(G_G2_OVERRIDESERVER, serverInstance);
}
const char* trap_SetActiveSubBSP(int index) {
	return (char*)proxy.originalSystemCall(G_SET_ACTIVE_SUBBSP, index);
}
int	trap_CM_RegisterTerrain(const char* config) {
	return proxy.originalSystemCall(G_CM_REGISTER_TERRAIN, config);
}
void trap_RMG_Init(void) {
	proxy.originalSystemCall(G_RMG_INIT);
}
void trap_Bot_UpdateWaypoints(int wpnum, wpobject_t** wps) {
	proxy.originalSystemCall(G_BOT_UPDATEWAYPOINTS, wpnum, wps);
}
void trap_Bot_CalculatePaths(int rmg) {
	proxy.originalSystemCall(G_BOT_CALCULATEPATHS, rmg);
}

// Translate import table funcptrs to syscalls

int SVSyscall_FS_Read(void* buffer, int len, fileHandle_t f)
{
	trap_FS_Read(buffer, len, f);
	return 0;
}

int SVSyscall_FS_Write(const void* buffer, int len, fileHandle_t f)
{
	trap_FS_Write(buffer, len, f);
	return 0;
}

qboolean SVSyscall_EntityContact(const vec3_t mins, const vec3_t maxs, const sharedEntity_t* ent, int capsule)
{
	if (capsule)
	{
		return trap_EntityContactCapsule(mins, maxs, ent);
	}
	else
	{
		return trap_EntityContact(mins, maxs, ent);
	}
}

void SVSyscall_Trace(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, int capsule, int traceFlags, int useLod) {
	if (capsule)
	{
		trap_TraceCapsule(results, start, mins, maxs, end, passEntityNum, contentmask);
	}
	else if (traceFlags)
	{
		trap_G2Trace(results, start, mins, maxs, end, passEntityNum, contentmask, traceFlags, useLod);
	}
	else
	{
		trap_Trace(results, start, mins, maxs, end, passEntityNum, contentmask);
	}
}

NORETURN void QDECL G_Error(int errorLevel, const char* error, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	trap_Error(text);
}

void QDECL G_Printf(const char* msg, ...) {
	va_list argptr;
	char text[4096] = { 0 };
	int ret;

	va_start(argptr, msg);
	ret = Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	if (ret == -1)
		trap_Print("G_Printf: overflow of 4096 bytes buffer\n");
	else
		trap_Print(text);
}

void Proxy_Translate_SystemCalls(void) {
	static gameImport_t import = { 0 };

	proxy.trap = &import;

	Com_Error = G_Error;
	Com_Printf = G_Printf;

	proxy.trap->Print = Com_Printf;
	proxy.trap->Error = Com_Error;
	proxy.trap->Milliseconds = trap_Milliseconds;
	proxy.trap->PrecisionTimerStart = trap_PrecisionTimer_Start;
	proxy.trap->PrecisionTimerEnd = trap_PrecisionTimer_End;
	proxy.trap->SV_RegisterSharedMemory = trap_SV_RegisterSharedMemory;
	proxy.trap->RealTime = trap_RealTime;
	proxy.trap->TrueMalloc = trap_TrueMalloc;
	proxy.trap->TrueFree = trap_TrueFree;
	proxy.trap->SnapVector = trap_SnapVector;
	proxy.trap->Cvar_Register = trap_Cvar_Register;
	proxy.trap->Cvar_Set = trap_Cvar_Set;
	proxy.trap->Cvar_Update = trap_Cvar_Update;
	proxy.trap->Cvar_VariableIntegerValue = trap_Cvar_VariableIntegerValue;
	proxy.trap->Cvar_VariableStringBuffer = trap_Cvar_VariableStringBuffer;
	proxy.trap->Argc = trap_Argc;
	proxy.trap->Argv = trap_Argv;
	proxy.trap->FS_Close = trap_FS_FCloseFile;
	proxy.trap->FS_GetFileList = trap_FS_GetFileList;
	proxy.trap->FS_Open = trap_FS_FOpenFile;
	proxy.trap->FS_Read = SVSyscall_FS_Read;
	proxy.trap->FS_Write = SVSyscall_FS_Write;
	proxy.trap->AdjustAreaPortalState = trap_AdjustAreaPortalState;
	proxy.trap->AreasConnected = trap_AreasConnected;
	proxy.trap->DebugPolygonCreate = trap_DebugPolygonCreate;
	proxy.trap->DebugPolygonDelete = trap_DebugPolygonDelete;
	proxy.trap->DropClient = trap_DropClient;
	proxy.trap->EntitiesInBox = trap_EntitiesInBox;
	proxy.trap->EntityContact = SVSyscall_EntityContact;
	proxy.trap->Trace = SVSyscall_Trace;
	proxy.trap->GetConfigstring = trap_GetConfigstring;
	proxy.trap->GetEntityToken = trap_GetEntityToken;
	proxy.trap->GetServerinfo = trap_GetServerinfo;
	proxy.trap->GetUsercmd = trap_GetUsercmd;
	proxy.trap->GetUserinfo = trap_GetUserinfo;
	proxy.trap->InPVS = trap_InPVS;
	proxy.trap->InPVSIgnorePortals = trap_InPVSIgnorePortals;
	proxy.trap->LinkEntity = trap_LinkEntity;
	proxy.trap->LocateGameData = trap_LocateGameData;
	proxy.trap->PointContents = trap_PointContents;
	proxy.trap->SendConsoleCommand = trap_SendConsoleCommand;
	proxy.trap->SendServerCommand = trap_SendServerCommand;
	proxy.trap->SetBrushModel = trap_SetBrushModel;
	proxy.trap->SetConfigstring = trap_SetConfigstring;
	proxy.trap->SetServerCull = trap_SetServerCull;
	proxy.trap->SetUserinfo = trap_SetUserinfo;
	proxy.trap->SiegePersSet = trap_SiegePersSet;
	proxy.trap->SiegePersGet = trap_SiegePersGet;
	proxy.trap->UnlinkEntity = trap_UnlinkEntity;
	proxy.trap->ROFF_Clean = trap_ROFF_Clean;
	proxy.trap->ROFF_UpdateEntities = trap_ROFF_UpdateEntities;
	proxy.trap->ROFF_Cache = trap_ROFF_Cache;
	proxy.trap->ROFF_Play = trap_ROFF_Play;
	proxy.trap->ROFF_Purge_Ent = trap_ROFF_Purge_Ent;
	proxy.trap->ICARUS_RunScript = trap_ICARUS_RunScript;
	proxy.trap->ICARUS_RegisterScript = trap_ICARUS_RegisterScript;
	proxy.trap->ICARUS_Init = trap_ICARUS_Init;
	proxy.trap->ICARUS_ValidEnt = trap_ICARUS_ValidEnt;
	proxy.trap->ICARUS_IsInitialized = trap_ICARUS_IsInitialized;
	proxy.trap->ICARUS_MaintainTaskManager = trap_ICARUS_MaintainTaskManager;
	proxy.trap->ICARUS_IsRunning = trap_ICARUS_IsRunning;
	proxy.trap->ICARUS_TaskIDPending = trap_ICARUS_TaskIDPending;
	proxy.trap->ICARUS_InitEnt = trap_ICARUS_InitEnt;
	proxy.trap->ICARUS_FreeEnt = trap_ICARUS_FreeEnt;
	proxy.trap->ICARUS_AssociateEnt = trap_ICARUS_AssociateEnt;
	proxy.trap->ICARUS_Shutdown = trap_ICARUS_Shutdown;
	proxy.trap->ICARUS_TaskIDSet = trap_ICARUS_TaskIDSet;
	proxy.trap->ICARUS_TaskIDComplete = trap_ICARUS_TaskIDComplete;
	proxy.trap->ICARUS_SetVar = trap_ICARUS_SetVar;
	proxy.trap->ICARUS_VariableDeclared = trap_ICARUS_VariableDeclared;
	proxy.trap->ICARUS_GetFloatVariable = trap_ICARUS_GetFloatVariable;
	proxy.trap->ICARUS_GetStringVariable = trap_ICARUS_GetStringVariable;
	proxy.trap->ICARUS_GetVectorVariable = trap_ICARUS_GetVectorVariable;
	proxy.trap->Nav_Init = trap_Nav_Init;
	proxy.trap->Nav_Free = trap_Nav_Free;
	proxy.trap->Nav_Load = trap_Nav_Load;
	proxy.trap->Nav_Save = trap_Nav_Save;
	proxy.trap->Nav_AddRawPoint = trap_Nav_AddRawPoint;
	proxy.trap->Nav_CalculatePaths = trap_Nav_CalculatePaths;
	proxy.trap->Nav_HardConnect = trap_Nav_HardConnect;
	proxy.trap->Nav_ShowNodes = trap_Nav_ShowNodes;
	proxy.trap->Nav_ShowEdges = trap_Nav_ShowEdges;
	proxy.trap->Nav_ShowPath = trap_Nav_ShowPath;
	proxy.trap->Nav_GetNearestNode = trap_Nav_GetNearestNode;
	proxy.trap->Nav_GetBestNode = trap_Nav_GetBestNode;
	proxy.trap->Nav_GetNodePosition = trap_Nav_GetNodePosition;
	proxy.trap->Nav_GetNodeNumEdges = trap_Nav_GetNodeNumEdges;
	proxy.trap->Nav_GetNodeEdge = trap_Nav_GetNodeEdge;
	proxy.trap->Nav_GetNumNodes = trap_Nav_GetNumNodes;
	proxy.trap->Nav_Connected = trap_Nav_Connected;
	proxy.trap->Nav_GetPathCost = trap_Nav_GetPathCost;
	proxy.trap->Nav_GetEdgeCost = trap_Nav_GetEdgeCost;
	proxy.trap->Nav_GetProjectedNode = trap_Nav_GetProjectedNode;
	proxy.trap->Nav_CheckFailedNodes = trap_Nav_CheckFailedNodes;
	proxy.trap->Nav_AddFailedNode = trap_Nav_AddFailedNode;
	proxy.trap->Nav_NodeFailed = trap_Nav_NodeFailed;
	proxy.trap->Nav_NodesAreNeighbors = trap_Nav_NodesAreNeighbors;
	proxy.trap->Nav_ClearFailedEdge = trap_Nav_ClearFailedEdge;
	proxy.trap->Nav_ClearAllFailedEdges = trap_Nav_ClearAllFailedEdges;
	proxy.trap->Nav_EdgeFailed = trap_Nav_EdgeFailed;
	proxy.trap->Nav_AddFailedEdge = trap_Nav_AddFailedEdge;
	proxy.trap->Nav_CheckFailedEdge = trap_Nav_CheckFailedEdge;
	proxy.trap->Nav_CheckAllFailedEdges = trap_Nav_CheckAllFailedEdges;
	proxy.trap->Nav_RouteBlocked = trap_Nav_RouteBlocked;
	proxy.trap->Nav_GetBestNodeAltRoute = trap_Nav_GetBestNodeAltRoute;
	proxy.trap->Nav_GetBestNodeAltRoute2 = trap_Nav_GetBestNodeAltRoute2;
	proxy.trap->Nav_GetBestPathBetweenEnts = trap_Nav_GetBestPathBetweenEnts;
	proxy.trap->Nav_GetNodeRadius = trap_Nav_GetNodeRadius;
	proxy.trap->Nav_CheckBlockedEdges = trap_Nav_CheckBlockedEdges;
	proxy.trap->Nav_ClearCheckedNodes = trap_Nav_ClearCheckedNodes;
	proxy.trap->Nav_CheckedNode = trap_Nav_CheckedNode;
	proxy.trap->Nav_SetCheckedNode = trap_Nav_SetCheckedNode;
	proxy.trap->Nav_FlagAllNodes = trap_Nav_FlagAllNodes;
	proxy.trap->Nav_GetPathsCalculated = trap_Nav_GetPathsCalculated;
	proxy.trap->Nav_SetPathsCalculated = trap_Nav_SetPathsCalculated;
	proxy.trap->BotAllocateClient = trap_BotAllocateClient;
	proxy.trap->BotFreeClient = trap_BotFreeClient;
	proxy.trap->BotLoadCharacter = trap_BotLoadCharacter;
	proxy.trap->BotFreeCharacter = trap_BotFreeCharacter;
	proxy.trap->Characteristic_Float = trap_Characteristic_Float;
	proxy.trap->Characteristic_BFloat = trap_Characteristic_BFloat;
	proxy.trap->Characteristic_Integer = trap_Characteristic_Integer;
	proxy.trap->Characteristic_BInteger = trap_Characteristic_BInteger;
	proxy.trap->Characteristic_String = trap_Characteristic_String;
	proxy.trap->BotAllocChatState = trap_BotAllocChatState;
	proxy.trap->BotFreeChatState = trap_BotFreeChatState;
	proxy.trap->BotQueueConsoleMessage = trap_BotQueueConsoleMessage;
	proxy.trap->BotRemoveConsoleMessage = trap_BotRemoveConsoleMessage;
	proxy.trap->BotNextConsoleMessage = trap_BotNextConsoleMessage;
	proxy.trap->BotNumConsoleMessages = trap_BotNumConsoleMessages;
	proxy.trap->BotInitialChat = trap_BotInitialChat;
	proxy.trap->BotReplyChat = trap_BotReplyChat;
	proxy.trap->BotChatLength = trap_BotChatLength;
	proxy.trap->BotEnterChat = trap_BotEnterChat;
	proxy.trap->StringContains = trap_StringContains;
	proxy.trap->BotFindMatch = trap_BotFindMatch;
	proxy.trap->BotMatchVariable = trap_BotMatchVariable;
	proxy.trap->UnifyWhiteSpaces = trap_UnifyWhiteSpaces;
	proxy.trap->BotReplaceSynonyms = trap_BotReplaceSynonyms;
	proxy.trap->BotLoadChatFile = trap_BotLoadChatFile;
	proxy.trap->BotSetChatGender = trap_BotSetChatGender;
	proxy.trap->BotSetChatName = trap_BotSetChatName;
	proxy.trap->BotResetGoalState = trap_BotResetGoalState;
	proxy.trap->BotResetAvoidGoals = trap_BotResetAvoidGoals;
	proxy.trap->BotPushGoal = trap_BotPushGoal;
	proxy.trap->BotPopGoal = trap_BotPopGoal;
	proxy.trap->BotEmptyGoalStack = trap_BotEmptyGoalStack;
	proxy.trap->BotDumpAvoidGoals = trap_BotDumpAvoidGoals;
	proxy.trap->BotDumpGoalStack = trap_BotDumpGoalStack;
	proxy.trap->BotGoalName = trap_BotGoalName;
	proxy.trap->BotGetTopGoal = trap_BotGetTopGoal;
	proxy.trap->BotGetSecondGoal = trap_BotGetSecondGoal;
	proxy.trap->BotChooseLTGItem = trap_BotChooseLTGItem;
	proxy.trap->BotChooseNBGItem = trap_BotChooseNBGItem;
	proxy.trap->BotTouchingGoal = trap_BotTouchingGoal;
	proxy.trap->BotItemGoalInVisButNotVisible = trap_BotItemGoalInVisButNotVisible;
	proxy.trap->BotGetLevelItemGoal = trap_BotGetLevelItemGoal;
	proxy.trap->BotAvoidGoalTime = trap_BotAvoidGoalTime;
	proxy.trap->BotInitLevelItems = trap_BotInitLevelItems;
	proxy.trap->BotUpdateEntityItems = trap_BotUpdateEntityItems;
	proxy.trap->BotLoadItemWeights = trap_BotLoadItemWeights;
	proxy.trap->BotFreeItemWeights = trap_BotFreeItemWeights;
	proxy.trap->BotSaveGoalFuzzyLogic = trap_BotSaveGoalFuzzyLogic;
	proxy.trap->BotAllocGoalState = trap_BotAllocGoalState;
	proxy.trap->BotFreeGoalState = trap_BotFreeGoalState;
	proxy.trap->BotResetMoveState = trap_BotResetMoveState;
	proxy.trap->BotMoveToGoal = trap_BotMoveToGoal;
	proxy.trap->BotMoveInDirection = trap_BotMoveInDirection;
	proxy.trap->BotResetAvoidReach = trap_BotResetAvoidReach;
	proxy.trap->BotResetLastAvoidReach = trap_BotResetLastAvoidReach;
	proxy.trap->BotReachabilityArea = trap_BotReachabilityArea;
	proxy.trap->BotMovementViewTarget = trap_BotMovementViewTarget;
	proxy.trap->BotAllocMoveState = trap_BotAllocMoveState;
	proxy.trap->BotFreeMoveState = trap_BotFreeMoveState;
	proxy.trap->BotInitMoveState = trap_BotInitMoveState;
	proxy.trap->BotChooseBestFightWeapon = trap_BotChooseBestFightWeapon;
	proxy.trap->BotGetWeaponInfo = trap_BotGetWeaponInfo;
	proxy.trap->BotLoadWeaponWeights = trap_BotLoadWeaponWeights;
	proxy.trap->BotAllocWeaponState = trap_BotAllocWeaponState;
	proxy.trap->BotFreeWeaponState = trap_BotFreeWeaponState;
	proxy.trap->BotResetWeaponState = trap_BotResetWeaponState;
	proxy.trap->GeneticParentsAndChildSelection = trap_GeneticParentsAndChildSelection;
	proxy.trap->BotInterbreedGoalFuzzyLogic = trap_BotInterbreedGoalFuzzyLogic;
	proxy.trap->BotMutateGoalFuzzyLogic = trap_BotMutateGoalFuzzyLogic;
	proxy.trap->BotGetNextCampSpotGoal = trap_BotGetNextCampSpotGoal;
	proxy.trap->BotGetMapLocationGoal = trap_BotGetMapLocationGoal;
	proxy.trap->BotNumInitialChats = trap_BotNumInitialChats;
	proxy.trap->BotGetChatMessage = trap_BotGetChatMessage;
	proxy.trap->BotRemoveFromAvoidGoals = trap_BotRemoveFromAvoidGoals;
	proxy.trap->BotPredictVisiblePosition = trap_BotPredictVisiblePosition;
	proxy.trap->BotSetAvoidGoalTime = trap_BotSetAvoidGoalTime;
	proxy.trap->BotAddAvoidSpot = trap_BotAddAvoidSpot;
	proxy.trap->BotLibSetup = trap_BotLibSetup;
	proxy.trap->BotLibShutdown = trap_BotLibShutdown;
	proxy.trap->BotLibVarSet = trap_BotLibVarSet;
	proxy.trap->BotLibVarGet = trap_BotLibVarGet;
	proxy.trap->BotLibDefine = trap_BotLibDefine;
	proxy.trap->BotLibStartFrame = trap_BotLibStartFrame;
	proxy.trap->BotLibLoadMap = trap_BotLibLoadMap;
	proxy.trap->BotLibUpdateEntity = trap_BotLibUpdateEntity;
	proxy.trap->BotLibTest = trap_BotLibTest;
	proxy.trap->BotGetSnapshotEntity = trap_BotGetSnapshotEntity;
	proxy.trap->BotGetServerCommand = trap_BotGetServerCommand;
	proxy.trap->BotUserCommand = trap_BotUserCommand;
	proxy.trap->BotUpdateWaypoints = trap_Bot_UpdateWaypoints;
	proxy.trap->BotCalculatePaths = trap_Bot_CalculatePaths;
	proxy.trap->AAS_EnableRoutingArea = trap_AAS_EnableRoutingArea;
	proxy.trap->AAS_BBoxAreas = trap_AAS_BBoxAreas;
	proxy.trap->AAS_AreaInfo = trap_AAS_AreaInfo;
	proxy.trap->AAS_EntityInfo = trap_AAS_EntityInfo;
	proxy.trap->AAS_Initialized = trap_AAS_Initialized;
	proxy.trap->AAS_PresenceTypeBoundingBox = trap_AAS_PresenceTypeBoundingBox;
	proxy.trap->AAS_Time = trap_AAS_Time;
	proxy.trap->AAS_PointAreaNum = trap_AAS_PointAreaNum;
	proxy.trap->AAS_TraceAreas = trap_AAS_TraceAreas;
	proxy.trap->AAS_PointContents = trap_AAS_PointContents;
	proxy.trap->AAS_NextBSPEntity = trap_AAS_NextBSPEntity;
	proxy.trap->AAS_ValueForBSPEpairKey = trap_AAS_ValueForBSPEpairKey;
	proxy.trap->AAS_VectorForBSPEpairKey = trap_AAS_VectorForBSPEpairKey;
	proxy.trap->AAS_FloatForBSPEpairKey = trap_AAS_FloatForBSPEpairKey;
	proxy.trap->AAS_IntForBSPEpairKey = trap_AAS_IntForBSPEpairKey;
	proxy.trap->AAS_AreaReachability = trap_AAS_AreaReachability;
	proxy.trap->AAS_AreaTravelTimeToGoalArea = trap_AAS_AreaTravelTimeToGoalArea;
	proxy.trap->AAS_Swimming = trap_AAS_Swimming;
	proxy.trap->AAS_PredictClientMovement = trap_AAS_PredictClientMovement;
	proxy.trap->AAS_AlternativeRouteGoals = trap_AAS_AlternativeRouteGoals;
	proxy.trap->AAS_PredictRoute = trap_AAS_PredictRoute;
	proxy.trap->AAS_PointReachabilityAreaIndex = trap_AAS_PointReachabilityAreaIndex;
	proxy.trap->EA_Say = trap_EA_Say;
	proxy.trap->EA_SayTeam = trap_EA_SayTeam;
	proxy.trap->EA_Command = trap_EA_Command;
	proxy.trap->EA_Action = trap_EA_Action;
	proxy.trap->EA_Gesture = trap_EA_Gesture;
	proxy.trap->EA_Talk = trap_EA_Talk;
	proxy.trap->EA_Attack = trap_EA_Attack;
	proxy.trap->EA_Alt_Attack = trap_EA_Alt_Attack;
	proxy.trap->EA_ForcePower = trap_EA_ForcePower;
	proxy.trap->EA_Use = trap_EA_Use;
	proxy.trap->EA_Respawn = trap_EA_Respawn;
	proxy.trap->EA_Crouch = trap_EA_Crouch;
	proxy.trap->EA_MoveUp = trap_EA_MoveUp;
	proxy.trap->EA_MoveDown = trap_EA_MoveDown;
	proxy.trap->EA_MoveForward = trap_EA_MoveForward;
	proxy.trap->EA_MoveBack = trap_EA_MoveBack;
	proxy.trap->EA_MoveLeft = trap_EA_MoveLeft;
	proxy.trap->EA_MoveRight = trap_EA_MoveRight;
	proxy.trap->EA_SelectWeapon = trap_EA_SelectWeapon;
	proxy.trap->EA_Jump = trap_EA_Jump;
	proxy.trap->EA_DelayedJump = trap_EA_DelayedJump;
	proxy.trap->EA_Move = trap_EA_Move;
	proxy.trap->EA_View = trap_EA_View;
	proxy.trap->EA_EndRegular = trap_EA_EndRegular;
	proxy.trap->EA_GetInput = trap_EA_GetInput;
	proxy.trap->EA_ResetInput = trap_EA_ResetInput;
	proxy.trap->PC_LoadSource = trap_PC_LoadSource;
	proxy.trap->PC_FreeSource = trap_PC_FreeSource;
	proxy.trap->PC_ReadToken = trap_PC_ReadToken;
	proxy.trap->PC_SourceFileAndLine = trap_PC_SourceFileAndLine;
	proxy.trap->R_RegisterSkin = trap_R_RegisterSkin;
	proxy.trap->SetActiveSubBSP = trap_SetActiveSubBSP;
	proxy.trap->CM_RegisterTerrain = trap_CM_RegisterTerrain;
	proxy.trap->RMG_Init = trap_RMG_Init;
	proxy.trap->G2API_ListModelBones = trap_G2_ListModelBones;
	proxy.trap->G2API_ListModelSurfaces = trap_G2_ListModelSurfaces;
	proxy.trap->G2API_HaveWeGhoul2Models = trap_G2_HaveWeGhoul2Models;
	proxy.trap->G2API_SetGhoul2ModelIndexes = trap_G2_SetGhoul2ModelIndexes;
	proxy.trap->G2API_GetBoltMatrix = trap_G2API_GetBoltMatrix;
	proxy.trap->G2API_GetBoltMatrix_NoReconstruct = trap_G2API_GetBoltMatrix_NoReconstruct;
	proxy.trap->G2API_GetBoltMatrix_NoRecNoRot = trap_G2API_GetBoltMatrix_NoRecNoRot;
	proxy.trap->G2API_InitGhoul2Model = trap_G2API_InitGhoul2Model;
	proxy.trap->G2API_SetSkin = trap_G2API_SetSkin;
	proxy.trap->G2API_Ghoul2Size = trap_G2API_Ghoul2Size;
	proxy.trap->G2API_AddBolt = trap_G2API_AddBolt;
	proxy.trap->G2API_SetBoltInfo = trap_G2API_SetBoltInfo;
	proxy.trap->G2API_SetBoneAngles = trap_G2API_SetBoneAngles;
	proxy.trap->G2API_SetBoneAnim = trap_G2API_SetBoneAnim;
	proxy.trap->G2API_GetBoneAnim = trap_G2API_GetBoneAnim;
	proxy.trap->G2API_GetGLAName = trap_G2API_GetGLAName;
	proxy.trap->G2API_CopyGhoul2Instance = trap_G2API_CopyGhoul2Instance;
	proxy.trap->G2API_CopySpecificGhoul2Model = trap_G2API_CopySpecificGhoul2Model;
	proxy.trap->G2API_DuplicateGhoul2Instance = trap_G2API_DuplicateGhoul2Instance;
	proxy.trap->G2API_HasGhoul2ModelOnIndex = trap_G2API_HasGhoul2ModelOnIndex;
	proxy.trap->G2API_RemoveGhoul2Model = trap_G2API_RemoveGhoul2Model;
	proxy.trap->G2API_RemoveGhoul2Models = trap_G2API_RemoveGhoul2Models;
	proxy.trap->G2API_CleanGhoul2Models = trap_G2API_CleanGhoul2Models;
	proxy.trap->G2API_CollisionDetect = trap_G2API_CollisionDetect;
	proxy.trap->G2API_CollisionDetectCache = trap_G2API_CollisionDetectCache;
	proxy.trap->G2API_SetRootSurface = trap_G2API_SetRootSurface;
	proxy.trap->G2API_SetSurfaceOnOff = trap_G2API_SetSurfaceOnOff;
	proxy.trap->G2API_SetNewOrigin = trap_G2API_SetNewOrigin;
	proxy.trap->G2API_DoesBoneExist = trap_G2API_DoesBoneExist;
	proxy.trap->G2API_GetSurfaceRenderStatus = trap_G2API_GetSurfaceRenderStatus;
	proxy.trap->G2API_AbsurdSmoothing = trap_G2API_AbsurdSmoothing;
	proxy.trap->G2API_SetRagDoll = trap_G2API_SetRagDoll;
	proxy.trap->G2API_AnimateG2Models = trap_G2API_AnimateG2Models;
	proxy.trap->G2API_RagPCJConstraint = trap_G2API_RagPCJConstraint;
	proxy.trap->G2API_RagPCJGradientSpeed = trap_G2API_RagPCJGradientSpeed;
	proxy.trap->G2API_RagEffectorGoal = trap_G2API_RagEffectorGoal;
	proxy.trap->G2API_GetRagBonePos = trap_G2API_GetRagBonePos;
	proxy.trap->G2API_RagEffectorKick = trap_G2API_RagEffectorKick;
	proxy.trap->G2API_RagForceSolve = trap_G2API_RagForceSolve;
	proxy.trap->G2API_SetBoneIKState = trap_G2API_SetBoneIKState;
	proxy.trap->G2API_IKMove = trap_G2API_IKMove;
	proxy.trap->G2API_RemoveBone = trap_G2API_RemoveBone;
	proxy.trap->G2API_AttachInstanceToEntNum = trap_G2API_AttachInstanceToEntNum;
	proxy.trap->G2API_ClearAttachedInstance = trap_G2API_ClearAttachedInstance;
	proxy.trap->G2API_CleanEntAttachments = trap_G2API_CleanEntAttachments;
	proxy.trap->G2API_OverrideServer = trap_G2API_OverrideServer;
	proxy.trap->G2API_GetSurfaceName = trap_G2API_GetSurfaceName;
}