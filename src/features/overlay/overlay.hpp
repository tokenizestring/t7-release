#pragma once

#include "../../stdafx.hpp"

namespace features::overlay
{
    enum class level
    {
        info,

        good,

        warn,

        bad,
    };

    void initialize();

    void notify(const std::string& text, level lvl = level::info);
}
