#pragma once

#include "sdk/qcommon/q_platform.hpp"

const char* Proxy_FS_GetCurrentGameDir(bool emptybase = false);

extern void (QDECL *Original_Com_Printf)(const char*, ...);
void QDECL Proxy_Com_Printf(const char* fmt, ...);