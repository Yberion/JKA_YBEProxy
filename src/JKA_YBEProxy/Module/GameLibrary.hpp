#pragma once

// Loads the original game module (JKA_YBEProxy.dll / .so) and hands it the proxy's system call. Exits when it fails: no
// map can run without it
void Proxy_GameLibrary_Load(void);
// Unloads it, once it got GAME_SHUTDOWN
void Proxy_GameLibrary_Unload(void);
