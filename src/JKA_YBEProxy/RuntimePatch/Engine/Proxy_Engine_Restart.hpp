#pragma once

// Called at each game frame: reloads the map before SV_Frame stops the server for good (see Proxy_Engine_Restart.cpp)
void Proxy_Engine_Restart_BeforeWrap(void);
