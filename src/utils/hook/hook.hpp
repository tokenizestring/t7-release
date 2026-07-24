#pragma once

#include "../../stdafx.hpp"

namespace utils::hook
{
    bool attach(void** original, void* detour);
}
