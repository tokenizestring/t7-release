#pragma once

#include "../../stdafx.hpp"

namespace utils::log
{
    void initialize();

    void write(const std::string& message);

    std::filesystem::path root_directory();
}

#ifdef T7_NO_LOG
#define T7_LOG_INIT() ((void)0)
#define T7_LOG(message) ((void)0)
#else
#define T7_LOG_INIT() utils::log::initialize()
#define T7_LOG(message) utils::log::write(message)
#endif
