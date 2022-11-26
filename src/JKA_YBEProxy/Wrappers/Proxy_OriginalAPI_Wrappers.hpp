#pragma once

#include "sdk/qcommon/q_platform.hpp"

// VM_DllSyscall can handle up to 1 (command) + 16 args
intptr_t QDECL Proxy_OriginalAPI_VM_DllSyscall(intptr_t command, ...);
// VM_Call can handle up to 1 (command) + 11 args
intptr_t QDECL Proxy_OriginalAPI_VM_Call(intptr_t command, ...);