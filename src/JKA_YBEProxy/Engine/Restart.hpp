#pragma once

namespace ybeproxy::engine::restart
{

// Called at each game frame: reloads the map before SV_Frame stops the server for good (see Restart.cpp)
void BeforeWrap(void);

} // namespace ybeproxy::engine::restart
