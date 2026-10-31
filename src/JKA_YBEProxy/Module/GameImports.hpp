#pragma once

#include "sdk/qcommon/q_platform.hpp"

namespace ybeproxy::module::game_imports
{

// VM_DllSyscall can handle up to 1 (command) + 16 args
intptr_t QDECL Syscall(intptr_t command, ...);
// VM_Call can handle up to 1 (command) + 11 args
intptr_t QDECL Call(intptr_t command, ...);

} // namespace ybeproxy::module::game_imports
