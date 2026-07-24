#pragma once

#include "../../stdafx.hpp"

namespace utils::resource
{
    void set_module(HMODULE module);

    bool load(int id, const void*& data, size_t& size);
}
