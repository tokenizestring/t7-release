#include "oob.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <cstring>
#include <cctype>
#include <string>

namespace oob
{
    static const char* const blocked_commands[] = { "mstart", "connectresponsemigration", "requeststats", "rcon", "echo", "relay", "vt", "@" };

    static bool is_blocked(const char* line)
    {
        std::string lowered = line;

        for (char& c : lowered)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        for (const char* entry : blocked_commands)
        {
            if (std::strstr(lowered.c_str(), entry) != nullptr)
            {
                return true;
            }
        }

        return false;
    }

    static char __fastcall hk_cl_connectionless(int local_client_num, void* from, void* msg)
    {
        engine::msg_s* m = static_cast<engine::msg_s*>(msg);

        engine::msg_s backup = *m;

        char buffer[1024];

        engine::msg_begin_reading(m);

        engine::msg_read_long(m);

        const char* line = engine::msg_read_string_line(m, buffer, sizeof(buffer) - 1);

        buffer[sizeof(buffer) - 1] = 0;

        *m = backup;

        bool blocked = is_blocked(line);

        if (line[0] != 0)
        {
            T7_LOG(std::string(cx("oob(cl): ")) + line + (blocked ? cx(" dropped") : cx("")) + cx("."));
        }

        return blocked ? 1 : engine::cl_connectionless(local_client_num, from, msg);
    }

    static int64_t __fastcall hk_sv_connectionless(void* from, void* msg)
    {
        engine::msg_s* m = static_cast<engine::msg_s*>(msg);

        engine::msg_s backup = *m;

        char buffer[1024];

        engine::msg_begin_reading(m);

        engine::msg_read_long(m);

        const char* line = engine::msg_read_string_line(m, buffer, sizeof(buffer) - 1);

        buffer[sizeof(buffer) - 1] = 0;

        *m = backup;

        bool blocked = is_blocked(line);

        if (line[0] != 0)
        {
            T7_LOG(std::string(cx("oob(sv): ")) + line + (blocked ? cx(" dropped") : cx("")) + cx("."));
        }

        return blocked ? 1 : engine::sv_connectionless(from, msg);
    }

    void initialize()
    {
        engine::cl_connectionless = reinterpret_cast<engine::cl_connectionless_t>(engine::base() + engine::cl_connectionless_cmd);

        engine::sv_connectionless = reinterpret_cast<engine::sv_connectionless_t>(engine::base() + engine::sv_connectionless_cmd);

        utils::hook::attach(reinterpret_cast<void**>(&engine::cl_connectionless), hk_cl_connectionless);

        utils::hook::attach(reinterpret_cast<void**>(&engine::sv_connectionless), hk_sv_connectionless);

        T7_LOG(cx("oob: protections installed."));
    }
}
