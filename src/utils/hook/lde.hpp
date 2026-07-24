#pragma once

#include <cstdint>

namespace utils::hook::lde
{
    struct instruction
    {
        int length;

        int riprel_disp_offset;

        int rel_offset;

        int rel_size;
    };

    bool decode(const uint8_t* code, instruction& out);
}
