#pragma once

namespace ybeproxy::module::game_library
{

// Loads the original game module (JKA_YBEProxy.dll / .so) and hands it the proxy's system call. Exits when it fails: no
// map can run without it
void Load(void);
// Unloads it, once it got GAME_SHUTDOWN
void Unload(void);

} // namespace ybeproxy::module::game_library
