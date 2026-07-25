#pragma once

#include "../../stdafx.hpp"

namespace overlay::font
{
    struct glyph
    {
        float u0;

        float v0;

        float u1;

        float v1;

        float width;

        float height;

        float advance;
    };

    bool build();

    const uint8_t* atlas_pixels();

    uint32_t atlas_width();

    uint32_t atlas_height();

    const glyph& get(char c);

    const glyph& white_texel();

    float line_height();

    float text_width(const char* text);
}
