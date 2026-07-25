#include "netchan.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <cstdint>
#include <cstdlib>
#include <string>

namespace netchan
{
    static bool block_enabled = false;

    static bool toggle_latch = false;

    static engine::net_send_packet_t original_net_send = nullptr;

    static std::string adr_string(const engine::netadr_s* adr)
    {
        return std::to_string(adr->ip[0]) + "." + std::to_string(adr->ip[1]) + "." + std::to_string(adr->ip[2]) + "." + std::to_string(adr->ip[3]) + ":" + std::to_string(adr->port);
    }

    static bool __fastcall hk_net_send(uint32_t sock, uint32_t length, const void* data, const void* to)
    {
        const engine::netadr_s* adr = reinterpret_cast<const engine::netadr_s*>(to);

        if (block_enabled && adr != nullptr && adr->type == engine::netadr_type_raw_udp)
        {
            T7_LOG(std::string(cx("netchan: send ")) + std::to_string(length) + cx("b to ") + adr_string(adr) + cx(" dropped."));

            return true;
        }

        return original_net_send(sock, length, data, to);
    }

    static char __fastcall hk_net_get_packet(void* socket, void* a2, void* a3, engine::netadr_s* from, int max_size, uint32_t* out_size, void* out_buffer)
    {
        char got = engine::net_get_packet_fn(socket, a2, a3, from, max_size, out_size, out_buffer);

        if (block_enabled && got != 0 && from != nullptr && from->type == engine::netadr_type_raw_udp)
        {
            T7_LOG(std::string(cx("netchan: recv ")) + std::to_string(out_size != nullptr ? *out_size : 0) + cx("b from ") + adr_string(from) + cx(" dropped."));

            return 0;
        }

        return got;
    }

    static void __fastcall hk_sv_packet_event(void* from, uint64_t xuid, void* msg)
    {
        uintptr_t base = engine::base();

        if (msg != nullptr)
        {
            int cur_size = *reinterpret_cast<int*>(static_cast<uint8_t*>(msg) + 28);

            uint8_t* data = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(msg) + 8);

            if (cur_size >= 4 && data != nullptr && *reinterpret_cast<int*>(data) == -1)
            {
                engine::sv_packet_event_fn(from, xuid, msg);

                return;
            }
        }

        int client_num = xuid == engine::loopback_xuid ? 0 : reinterpret_cast<engine::sv_client_num_for_xuid_t>(base + engine::sv_client_num_for_xuid)(xuid);

        if (client_num >= 0)
        {
            uintptr_t clients = *reinterpret_cast<uintptr_t*>(base + engine::svs_clients);

            if (clients != 0)
            {
                void* stored = reinterpret_cast<void*>(clients + engine::client_data_stride * static_cast<size_t>(client_num) + engine::client_netadr_offset);

                if (reinterpret_cast<engine::net_compare_base_adr_t>(base + engine::net_compare_base_adr)(from, stored) == 0)
                {
                    T7_LOG(std::string(cx("netchan: spoofed packet xuid ")) + std::to_string(xuid) + cx(" client ") + std::to_string(client_num) + cx(" dropped."));

                    return;
                }
            }
        }

        engine::sv_packet_event_fn(from, xuid, msg);
    }

    static int64_t __fastcall hk_write_reliable_commands(int32_t* client, void* msg)
    {
        if (client != nullptr)
        {
            int reliable_sequence = client[5132];

            int reliable_acknowledge = client[5133];

            if (reliable_acknowledge < 0 || reliable_acknowledge > reliable_sequence)
            {
                client[5133] = reliable_sequence;

                T7_LOG(std::string(cx("netchan: bad reliable ack ")) + std::to_string(reliable_acknowledge) + cx(" seq ") + std::to_string(reliable_sequence) + cx(", ignored."));
            }
            else if (reliable_acknowledge < reliable_sequence - engine::max_reliable_commands)
            {
                client[5133] = reliable_sequence - engine::max_reliable_commands;

                T7_LOG(std::string(cx("netchan: stale reliable ack ")) + std::to_string(reliable_acknowledge) + cx(" seq ") + std::to_string(reliable_sequence) + cx(", ignored."));
            }
        }

        return engine::sv_write_reliable_commands_fn(client, msg);
    }

    static int64_t __fastcall hk_parse_gamestate(uint32_t local_client, void* msg, double a3, double a4)
    {
        uintptr_t base = engine::base();

        void* client = reinterpret_cast<engine::cl_get_local_client_globals_t>(base + engine::cl_get_local_client_globals)(local_client);

        int server_id = client != nullptr ? *reinterpret_cast<int*>(static_cast<uint8_t*>(client) + 46708) : 0;

        const char* config_string = reinterpret_cast<engine::cl_get_config_string_t>(base + engine::cl_get_config_string)(3);

        bool invalid = server_id != 0 && config_string != nullptr && ((static_cast<uint8_t>(atoi(config_string)) ^ static_cast<uint8_t>(server_id)) & 0xF0) == 0;

        if (invalid)
        {
            T7_LOG(std::string(cx("netchan: gamedata server_id=")) + std::to_string(server_id) + cx(" cs=\"") + std::string(config_string) + cx("\", dropped."));

            return 0;
        }

        return engine::cl_parse_gamestate_fn(local_client, msg, a3, a4);
    }

    static uint64_t reassemble_blocked = 0;

    static char __fastcall hk_netchan_reassemble(int32_t client, int64_t channel, int64_t message, void* out_msgid, void* out_src, void* out_seq)
    {
        int32_t chan = static_cast<int32_t>(channel);

        if (message != 0 && client >= 0 && client < engine::netchan_msg_client_max && chan >= 0 && chan < engine::netchan_msg_channel_max)
        {
            uint64_t* table = reinterpret_cast<uint64_t*>(engine::base() + engine::netchan_msg_table);

            uintptr_t entry = table[engine::netchan_msg_entry_stride * client + chan];

            while (entry != 0 && *reinterpret_cast<uint8_t*>(entry + engine::netchan_msg_complete_flag) == 0)
            {
                entry = *reinterpret_cast<uintptr_t*>(entry + engine::netchan_msg_bucket_next);
            }

            if (entry != 0)
            {
                uint32_t capacity = *reinterpret_cast<uint32_t*>(message + engine::netchan_msgbuf_capacity);

                uintptr_t node = *reinterpret_cast<uintptr_t*>(entry + engine::netchan_msg_fragment_head);

                while (node != 0)
                {
                    uint32_t index = *reinterpret_cast<uint8_t*>(node + engine::netchan_fragment_index);

                    uint32_t length = *reinterpret_cast<uint32_t*>(node + engine::netchan_fragment_length);

                    if (engine::netchan_reassemble_stride * index + length > capacity)
                    {
                        *reinterpret_cast<uint32_t*>(message + engine::netchan_msgbuf_error) = 1;

                        *reinterpret_cast<uint32_t*>(message + engine::netchan_msgbuf_written) = 0;

                        if (reassemble_blocked++ % 64 == 0)
                        {
                            T7_LOG(std::string(cx("netchan: oob reassembly client ")) + std::to_string(client) + cx(" channel ") + std::to_string(chan) + cx(" frag ") + std::to_string(index) + cx(" len ") + std::to_string(length) + cx(" cap ") + std::to_string(capacity) + cx(", dropped."));
                        }

                        return 0;
                    }

                    node = *reinterpret_cast<uintptr_t*>(node);
                }
            }
        }

        return engine::netchan_reassemble_fn(client, channel, message, out_msgid, out_src, out_seq);
    }

    void initialize()
    {
        engine::cl_parse_gamestate_fn = reinterpret_cast<engine::cl_parse_gamestate_t>(engine::base() + engine::cl_parse_gamestate);

        utils::hook::attach(reinterpret_cast<void**>(&engine::cl_parse_gamestate_fn), hk_parse_gamestate);

        engine::sv_write_reliable_commands_fn = reinterpret_cast<engine::sv_write_reliable_commands_t>(engine::base() + engine::sv_write_reliable_commands);

        utils::hook::attach(reinterpret_cast<void**>(&engine::sv_write_reliable_commands_fn), hk_write_reliable_commands);

        engine::sv_packet_event_fn = reinterpret_cast<engine::sv_packet_event_t>(engine::base() + engine::sv_packet_event);

        utils::hook::attach(reinterpret_cast<void**>(&engine::sv_packet_event_fn), hk_sv_packet_event);

        original_net_send = reinterpret_cast<engine::net_send_packet_t>(engine::base() + engine::net_send);

        utils::hook::attach(reinterpret_cast<void**>(&original_net_send), hk_net_send);

        engine::net_get_packet_fn = reinterpret_cast<engine::net_get_packet_t>(engine::base() + engine::net_get_packet);

        utils::hook::attach(reinterpret_cast<void**>(&engine::net_get_packet_fn), hk_net_get_packet);

        engine::netchan_reassemble_fn = reinterpret_cast<engine::netchan_reassemble_t>(engine::base() + engine::netchan_reassemble);

        utils::hook::attach(reinterpret_cast<void**>(&engine::netchan_reassemble_fn), hk_netchan_reassemble);

        T7_LOG(cx("netchan: gamestate + reliable + spoof + reassembly-oob protections installed, udp block ready (F7 toggles, default off)."));
    }

    void tick()
    {
        bool down = (GetAsyncKeyState(VK_F7) & 0x8000) != 0;

        if (down && !toggle_latch)
        {
            toggle_latch = true;

            block_enabled = !block_enabled;

            if (block_enabled) T7_LOG(cx("netchan: enabled - raw udp blocked (breaks dedi / direct connect)."));
            else T7_LOG(cx("netchan: disabled - raw udp restored."));
        }
        else if (!down)
        {
            toggle_latch = false;
        }
    }
}
