#pragma once

#include "../../stdafx.hpp"

#include <d3d11.h>

namespace overlay::renderer
{
    uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

    bool init(ID3D11Device* device);

    void shutdown();

    bool ready();

    void begin(ID3D11DeviceContext* context, ID3D11RenderTargetView* target, float screen_w, float screen_h);

    void draw_rect(float x, float y, float w, float h, uint32_t color);

    void draw_image(float x, float y, float w, float h, ID3D11ShaderResourceView* srv);

    void draw_text(float x, float y, const char* text, uint32_t color);

    void draw_text_shadow(float x, float y, const char* text, uint32_t color, uint32_t shadow);

    float text_width(const char* text);

    float line_height();

    void end();
}
