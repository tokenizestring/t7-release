#pragma once

#include "../../stdafx.hpp"

#include <d3d11.h>

namespace features::menulogo
{
    void initialize();

    void tick();

    ID3D11ShaderResourceView* current_srv();

    uint32_t width();

    uint32_t height();
}
