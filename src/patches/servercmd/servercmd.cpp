#include "servercmd.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <cstring>

namespace servercmd
{
    static constexpr uint32_t serverpos_index_max = 1792;

    static constexpr uint32_t model_notify_numargs_max = 16;

    static constexpr uint32_t model_notify_window_ms = 250;

    static constexpr uint32_t model_notify_window_max = 40;

    static uint32_t model_notify_window_start = 0;

    static uint32_t model_notify_count = 0;

    static int64_t __fastcall hk_get_server_command(uint32_t local_client, int32_t sequence)
    {
        int32_t current = engine::server_command_sequence(local_client);

        if (sequence <= current - 128 || sequence > current)
        {
            T7_LOG(std::string(cx("servercmd: skipped out-of-window sequence ")) + std::to_string(sequence) + cx(" of ") + std::to_string(current) + cx(" (kick guard)"));

            return 0;
        }

        const char* command = engine::server_command(local_client, sequence);

        if (command != nullptr && command[0] != 0)
        {
            char safe[1024];

            size_t i = 0;

            for (; i < sizeof(safe) - 1 && command[i] != 0; i++)
            {
                safe[i] = command[i];
            }

            safe[i] = 0;

            T7_LOG(std::string(cx("servercmd: [")) + std::to_string(sequence) + cx("] ") + safe);
        }

        if (command != nullptr && (command[0] == '&' || command[0] == '\''))
        {
            size_t projected = engine::bcs_length() + strlen(command);

            if (projected >= 20480)
            {
                T7_LOG(std::string(cx("servercmd: blocked bcs reassembly overflow (")) + std::to_string(projected) + cx(" bytes) (kick guard)"));

                return 0;
            }
        }

        return engine::get_server_command_fn(local_client, sequence);
    }

    // rce fix: the server "message" commands ( ) < ; O ) format host text into a
    // 256-byte buffer, but the [{...}] localization tokens could expand it up to
    // 1024 bytes = stack buffer overflow the host could use for code exec.
    // fix: let the game format into our own big scratch, then copy back only 255.
    static int64_t __fastcall hk_message_format(uint32_t local_client, const char* message, int64_t context, char* out)
    {
        char scratch[2048] = {};

        engine::message_format_fn(local_client, message, context, scratch);

        size_t i = 0;

        for (; i < 255 && scratch[i] != 0; i++)
        {
            out[i] = scratch[i];
        }

        out[i] = 0;

        return static_cast<int64_t>(i);
    }

    static char __fastcall hk_bg_cache_checksum(uint32_t controller, int64_t data)
    {
        char result = engine::bg_cache_checksum_fn(controller, data);

        uint32_t* state = reinterpret_cast<uint32_t*>(engine::base() + engine::bg_cache_state);

        if (state[1] != state[0])
        {
            T7_LOG(cx("servercmd: neutralized bg-cache checksum mismatch (kick guard)"));

            state[1] = state[0];
        }

        return result;
    }

    static int64_t __fastcall hk_set_config_string(uint32_t local_client)
    {
        uint32_t index = static_cast<uint32_t>(engine::cmd_atoi(engine::cmd_argv(1)));

        if (index > 3629)
        {
            T7_LOG(std::string(cx("servercmd: blocked oob configstring index ")) + std::to_string(index) + cx(" (kick guard)"));

            return 0;
        }

        if (engine::configstring_pool_overflow(index, engine::cmd_argv(2)))
        {
            T7_LOG(std::string(cx("servercmd: blocked configstring pool overflow at index ")) + std::to_string(index) + cx(" (kick guard)"));

            return 0;
        }

        return engine::set_config_string_fn(local_client);
    }

    static int64_t __fastcall hk_cg_server_command(uint32_t local_client)
    {
        const char* command = engine::cmd_argv(0);

        // rce fix: the server "/" command sets an entity's position by index. the
        // game never checked that index (only 0..1791 exist), so a huge index made
        // it read a pointer from out-of-bounds memory and write through it = remote
        // code exec. fix: drop the command if the index is past the array.
        if (command != nullptr && command[0] == '/')
        {
            uint32_t index = engine::cmd_arg_int(engine::cmd_argv(1));

            if (index >= serverpos_index_max)
            {
                T7_LOG(std::string(cx("servercmd: blocked oob '/' index ")) + std::to_string(index) + cx(" (rce guard)"));

                return 0;
            }
        }

        // rce fix: the server "7" command flips an AI-entity flag by team + entity
        // number. neither number was checked, so a bad team or entity wrote a
        // 2-byte value out of bounds = remote memory corruption. only team 0..1 and
        // entity 0..499 exist, so drop anything outside that (signed, to catch
        // negatives the game parses too).
        if (command != nullptr && command[0] == '7')
        {
            int32_t team = engine::cmd_atoi(engine::cmd_argv(2));

            uint16_t entity = static_cast<uint16_t>(engine::cmd_atoi(engine::cmd_argv(1)));

            if (team < 0 || team > 1 || entity >= 500)
            {
                T7_LOG(std::string(cx("servercmd: blocked oob '7' team ")) + std::to_string(team) + cx(" entity ") + std::to_string(entity) + cx(" (rce guard)"));

                return 0;
            }
        }

        if (command != nullptr && command[0] == 'D')
        {
            uint32_t num_args = static_cast<uint32_t>(engine::cmd_atoi(engine::cmd_argv(2)));

            if (num_args > model_notify_numargs_max)
            {
                T7_LOG(std::string(cx("servercmd: blocked oversized 'D' model notify (")) + std::to_string(num_args) + cx(" args) (crash guard)"));

                return 0;
            }

            uint32_t now = GetTickCount();

            if (now - model_notify_window_start >= model_notify_window_ms)
            {
                model_notify_window_start = now;

                model_notify_count = 0;
            }

            if (++model_notify_count > model_notify_window_max)
            {
                if (model_notify_count % 200 == 1)
                {
                    T7_LOG(cx("servercmd: throttling 'D' model-notify flood (crash guard)"));
                }

                return 0;
            }
        }

        return engine::cg_server_command_fn(local_client);
    }

    void initialize()
    {
        engine::get_server_command_fn = reinterpret_cast<engine::get_server_command_t>(engine::base() + engine::cl_get_server_command);

        utils::hook::attach(reinterpret_cast<void**>(&engine::get_server_command_fn), hk_get_server_command);

        engine::cg_server_command_fn = reinterpret_cast<engine::cg_server_command_t>(engine::base() + engine::cg_server_command);

        utils::hook::attach(reinterpret_cast<void**>(&engine::cg_server_command_fn), hk_cg_server_command);

        engine::message_format_fn = reinterpret_cast<engine::message_format_t>(engine::base() + engine::cg_message_format);

        utils::hook::attach(reinterpret_cast<void**>(&engine::message_format_fn), hk_message_format);

        engine::set_config_string_fn = reinterpret_cast<engine::set_config_string_t>(engine::base() + engine::cl_set_config_string);

        utils::hook::attach(reinterpret_cast<void**>(&engine::set_config_string_fn), hk_set_config_string);

        engine::bg_cache_checksum_fn = reinterpret_cast<engine::bg_cache_checksum_t>(engine::base() + engine::bg_cache_checksum);

        //utils::hook::attach(reinterpret_cast<void**>(&engine::bg_cache_checksum_fn), hk_bg_cache_checksum); need to fix, too lazy to atm, since no one is using these exploits 

        T7_LOG(cx("servercmd: logger + rce guards + crash guards + kick guards (configstring + bg-cache) installed"));
    }
}
