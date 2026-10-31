#pragma once

// ==================================================
// PLATFORM SPECIFIC STUFF
// ==================================================

// /!\ Should start at 0 because it will be used as index to array
enum Proxy_Platform_e
{
	PROXY_WINDOWS = 0,
	PROXY_LINUX,

	PROXY_PLATFORM_LENGTH
};

#ifdef _MSC_VER
	#include <windows.h>

	#define PROXY_LIBRARY_EXT "dll"

	#define YBEProxy_OpenLibrary(a) (void *)LoadLibrary(a)
	#define YBEProxy_CloseLibrary(a) FreeLibrary((HMODULE)a)
	#define YBEProxy_GetFunctionAddress(a, b) GetProcAddress((HMODULE)a, b)

	#define ORIGINAL_ENGINE_VERSION "(internal)JAmp: v1.0.1.0 win-x86 Oct 30 2003"

	constexpr Proxy_Platform_e PROXY_PLATFORM = Proxy_Platform_e::PROXY_WINDOWS;
#else
	#include <dlfcn.h>

	#define PROXY_LIBRARY_EXT "so"

	#define YBEProxy_OpenLibrary(a) dlopen(a, RTLD_NOW)
	#define YBEProxy_CloseLibrary(a) dlclose(a)
	#define YBEProxy_GetFunctionAddress(a, b) dlsym(a, b)

	#define ORIGINAL_ENGINE_VERSION "JAmp: v1.0.1.1 linux-i386 Nov 10 2003"

	constexpr Proxy_Platform_e PROXY_PLATFORM = Proxy_Platform_e::PROXY_LINUX;
#endif
