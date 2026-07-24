#include "log.hpp"
#include "../crypt/crypt.hpp"

#include <mutex>

namespace utils::log
{
    static std::ofstream log_file;

    static std::mutex log_mutex;

    std::filesystem::path root_directory()
    {
        char module_path[MAX_PATH];

        DWORD length = GetModuleFileNameA(nullptr, module_path, MAX_PATH);

        if (length == 0)
        {
            return {};
        }

        return std::filesystem::path(module_path).parent_path() / cx("t7-rework").c_str();
    }

    void initialize()
    {
        std::filesystem::path directory = root_directory() / cx("logs").c_str();

        std::error_code ec;

        std::filesystem::create_directories(directory, ec);

        std::lock_guard<std::mutex> lock(log_mutex);

        log_file.open((directory / cx("t7-release.log").c_str()).string(), std::ios::out | std::ios::trunc);

        if (log_file.is_open())
        {
            log_file << cx("[t7] log initialized") << std::endl;
        }
    }

    void write(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(log_mutex);

        if (log_file.is_open())
        {
            log_file << cx("[t7] ") << message;

            if (message.empty() || message.back() != '.')
            {
                log_file << '.';
            }

            log_file << std::endl;
        }
    }
}
