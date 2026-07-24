#include "log.hpp"
#include "../crypt/crypt.hpp"

#include <mutex>

namespace utils::log
{
    static std::ofstream log_file;

    static std::mutex log_mutex;

    void initialize()
    {
        char module_path[MAX_PATH];

        DWORD length = GetModuleFileNameA(nullptr, module_path, MAX_PATH);

        if (length > 0)
        {
            std::filesystem::path path(module_path);

            auto log_name = cx("t7-release.log");
            path = path.parent_path() / log_name.c_str();

            std::lock_guard<std::mutex> lock(log_mutex);

            log_file.open(path.string(), std::ios::out | std::ios::trunc);

            if (log_file.is_open())
            {
                log_file << cx("[t7] log initialized") << std::endl;
            }
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
