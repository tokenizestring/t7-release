#pragma once

#include <string>

namespace lua
{
    void initialize();

    void queue(const std::string& code);

    void tick();
}
