#pragma once

#if defined(_M_X64) || defined(__x86_64__)
    #include "JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/x64/hde64.hpp"
#else
    #include "JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/x86/hde32.hpp"
#endif