#include "clientcmd.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"
#include "../../features/overlay/overlay.hpp"

#include <string>
#include <cstdlib>
#include <cstring>

namespace clientcmd
{
    static bool is_remote_video_command(const char* cmd)
    {
        return _stricmp(cmd, cx("video_start").c_str()) == 0 || _stricmp(cmd, cx("video_start_looped").c_str()) == 0;
    }

    static int64_t __fastcall hk_client_command(uint32_t client_num)
    {
        char cmd[32] = {};

        engine::sv_cmd_argv_buffer_fn(0, cmd, sizeof(cmd));

        std::string line;

        for (int i = 0; i < 24; i++)
        {
            char arg[256] = {};

            engine::sv_cmd_argv_buffer_fn(i, arg, sizeof(arg));

            if (arg[0] == 0)
            {
                break;
            }

            if (i > 0)
            {
                line += ' ';
            }

            line += arg;

            if (line.size() > 900)
            {
                break;
            }
        }

        T7_LOG(std::string(cx("clientcmd: [")) + std::to_string(client_num) + cx("] ") + line + cx("."));

        if (client_num != 0 && is_remote_video_command(cmd))
        {
            T7_LOG(cx("clientcmd: remote cinematic command dropped."));

            features::overlay::notify(cx("blocked remote screen takeover."), features::overlay::level::warn);

            return 0;
        }

        return engine::client_command_fn(client_num);
    }

    static int64_t __fastcall hk_mrp(int64_t entity)
    {
        char arg2[64] = {};

        char arg3[64] = {};

        engine::sv_cmd_argv_buffer_fn(2, arg2, sizeof(arg2));

        engine::sv_cmd_argv_buffer_fn(3, arg3, sizeof(arg3));

        if (static_cast<uint32_t>(atoi(arg2)) > 0x3F || static_cast<uint32_t>(atoi(arg3)) > 0x3F)
        {
            T7_LOG(cx("clientcmd: mrp out-of-range index, dropped."));

            features::overlay::notify(cx("blocked host crash attempt."), features::overlay::level::bad);

            return 0;
        }

        return engine::client_mrp_fn(entity);
    }

    void initialize()
    {
        engine::sv_cmd_argv_buffer_fn = reinterpret_cast<engine::sv_cmd_argv_buffer_t>(engine::base() + engine::sv_cmd_argv_buffer);

        engine::client_command_fn = reinterpret_cast<engine::client_command_t>(engine::base() + engine::client_command);

        utils::hook::attach(reinterpret_cast<void**>(&engine::client_command_fn), hk_client_command);

        engine::client_mrp_fn = reinterpret_cast<engine::client_mrp_t>(engine::base() + engine::client_mrp);

        utils::hook::attach(reinterpret_cast<void**>(&engine::client_mrp_fn), hk_mrp);

        T7_LOG(cx("clientcmd: logger + mrp oob guard + video takeover guard installed."));
    }
}
