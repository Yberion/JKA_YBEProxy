#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "JKA_SDK/game/g_local.h"



typedef int	(*systemCallFuncPtr_t)(int command, ...);
typedef int	(*vmMainFuncPtr_t)(int command, int, int, int, int, int, int, int, int, int, int, int, int);
typedef void (*dllEntryFuncPtr_t)(systemCallFuncPtr_t);

typedef struct Proxy_s Proxy_t;

struct Proxy_s {
	void *jampgameHandle;

	vmMainFuncPtr_t originalVmMain;
	dllEntryFuncPtr_t originalDllEntry;
	systemCallFuncPtr_t originalSystemCall;

	int originalVmMainResponse;
};

// Globale structure that centralize everything
extern Proxy_t proxy;