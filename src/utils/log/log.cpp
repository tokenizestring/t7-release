#include "log.hpp"
#include "../crypt/crypt.hpp"

#include <mutex>

namespace utils::log
{
    static std::mutex log_mutex;

    static std::filesystem::path log_path;

    static size_t written_bytes = 0;

    static constexpr size_t max_bytes = 4 * 1024 * 1024;

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

        log_path = directory / cx("t7-release.log").c_str();

        std::ofstream file(log_path.string(), std::ios::out | std::ios::trunc);

        if (file.is_open())
        {
            file << cx("[t7] log initialized.") << '\n';
        }

        written_bytes = 0;
    }

    void write(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(log_mutex);

        if (log_path.empty())
        {
            return;
        }

        std::ios::openmode mode = written_bytes > max_bytes ? std::ios::trunc : std::ios::app;

        std::ofstream file(log_path.string(), std::ios::out | mode);

        if (!file.is_open())
        {
            return;
        }

        if (mode == std::ios::trunc)
        {
            written_bytes = 0;
        }

        file << cx("[t7] ") << message;

        if (message.empty() || message.back() != '.')
        {
            file << '.';
        }

        file << '\n';

        written_bytes += message.size() + 8;
    }
}
