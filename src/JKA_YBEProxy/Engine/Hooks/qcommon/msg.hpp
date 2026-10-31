#pragma once

#include "sdk/qcommon/qcommon.hpp"

namespace ybeproxy::engine::hooks
{

extern int (*Original_MSG_ReadBits)(msg_t*, int);
int Proxy_MSG_ReadBits(msg_t* msg, int bits);

} // namespace ybeproxy::engine::hooks
