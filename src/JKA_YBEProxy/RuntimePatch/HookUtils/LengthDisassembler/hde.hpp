#pragma once

#if defined(_M_X64) || defined(__x86_64__)
    #include "x64/hde64.hpp"
#else
    #include "x86/hde32.hpp"
#endif