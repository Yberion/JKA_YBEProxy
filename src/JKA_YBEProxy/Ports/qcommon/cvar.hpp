#pragma once

#include "sdk/qcommon/q_shared.hpp"

namespace ybeproxy::ports
{

cvar_t* Cvar_SetValue(const char* var_name, float value);

} // namespace ybeproxy::ports
