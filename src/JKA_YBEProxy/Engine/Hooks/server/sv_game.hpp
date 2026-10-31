#pragma once

#include "sdk/game/g_public.hpp"

namespace ybeproxy::engine::hooks
{

extern svEntity_t* (*Original_SV_SvEntityForGentity)(sharedEntity_t*);
svEntity_t* Proxy_SV_SvEntityForGentity(sharedEntity_t* gEnt);

} // namespace ybeproxy::engine::hooks
