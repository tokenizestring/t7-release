#include "matchmaking.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/mem/mem.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <cstring>
#include <string>
#include <intrin.h>

namespace matchmaking
{
    static int get_playlist_version()
    {
        const int store_offset = 100000;

        int ffotd_version = engine::live_storage_get_ffotd_version_fn();

        int playlist_version = engine::playlist_get_version_number_fn();

        return playlist_version + ffotd_version * (store_offset - 1);
    }

    static int64_t __fastcall hk_set_matchmaking_info(void* session)
    {
        int64_t result = engine::lobby_set_matchmaking_info_fn(session);

        static bool logged = false;

        if (!logged)
        {
            logged = true;

            T7_LOG(cx("matchmaking: set_matchmaking_info override active."));
        }

        engine::lobby_online_s* online = reinterpret_cast<engine::lobby_online_s*>(engine::base() + engine::lobby_online_state);

        engine::matchmaking_info_s& mm = online->mm;

        mm.server_type = 2000;

        mm.latency_band = 0;

        mm.game_type = 0;

        mm.max_players = 12;

        mm.num_players = 11;

        mm.show_in_matchmaking = 1;

        mm.netcode_version = 18532;

        mm.map_packs = 2;

        mm.playlist_version = get_playlist_version();

        mm.playlist_number = 1;

        mm.is_empty = 0;

        mm.team_max = 6;

        mm.active = 1;

        mm.dirty = 0;

        mm.skill = 0.01f;

        mm.geo1 = 0;

        mm.geo2 = 0;

        mm.geo3 = 0;

        mm.geo4 = 0;

        int location_count = *reinterpret_cast<int*>(engine::base() + engine::qos_location_count);

        if (location_count > 0)
        {
            int index = static_cast<int>(GetTickCount() % static_cast<uint32_t>(location_count));

            const char* location = reinterpret_cast<const char*>(engine::base() + engine::qos_location_table + 160 * index);

            T7_LOG(std::string(cx("matchmaking: advertise location \"")) + std::string(location) + cx("\" code ") + std::to_string(*reinterpret_cast<const int*>(location)) + cx("."));

            mm.server_location = *reinterpret_cast<const int*>(location);
        }

        return result;
    }

    static bool __fastcall hk_lobby_host_update_matchmaking_session()
    {
        if (!engine::lobby_advertise)
        {
            return false;
        }

        static uint64_t last = 0;

        uint64_t now = GetTickCount64();

        if (last != 0 && now - last < 3000)
        {
            return true;
        }

        void* session = engine::lobby_host_data_get_session_fn(engine::lobby_type_game);

        if (session != nullptr)
        {
            engine::lobby_qos_listener_enable_fn(session);

            hk_set_matchmaking_info(session);

            *reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(session) + engine::lobby_session_advertised_offset) = true;

            void* task_def = reinterpret_cast<void*>(engine::base() + engine::live_session_task_def);

            engine::live_session_update_fn(task_def, reinterpret_cast<void*>(engine::base() + engine::live_matchmaking_info));
        }

        last = now;

        return true;
    }

    void create_session()
    {
        void* session = engine::lobby_host_data_get_session_fn != nullptr ? engine::lobby_host_data_get_session_fn(engine::lobby_type_game) : nullptr;

        if (session != nullptr)
        {
            *reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(session) + engine::lobby_session_advertised_offset) = true;

            engine::lobby_online_create_session_fn(session);
        }
    }

    static char __fastcall hk_lobby_session_set_max_clients(void* state, int count)
    {
        static bool logged = false;

        if (!logged)
        {
            logged = true;

            T7_LOG(std::string(cx("matchmaking: set_max_clients ")) + std::to_string(count) + cx(" forced to 18."));
        }

        return engine::lobby_session_set_max_clients_fn(state, 18);
    }

    bool __fastcall hk_lobby_host_task_search()
    {
        return false;
    }

    bool __fastcall hk_live_can_host_server()
    {
        return true;
    }

    static int64_t __fastcall hk_bd_session_id_serialize(void* this_ptr, void* byte_buffer)
    {
        int64_t result = engine::bd_session_id_serialize_fn(this_ptr, byte_buffer);

        if (_ReturnAddress() == reinterpret_cast<void*>(engine::base() + engine::bd_session_id_serialize_return))
        {
            engine::bd_byte_buffer_write_uint32_fn(byte_buffer, 11);

            static int bd_count = 0;

            if (bd_count++ % 32 == 0)
            {
                T7_LOG(cx("matchmaking: bd_session_id extra uint32 appended."));
            }
        }

        return result;
    }

    static bool __fastcall hk_handle_join_request(int controller, engine::netadr_s adr, int64_t xuid, engine::lobby_msg_s* lobby_msg)
    {
        T7_LOG(std::string(cx("matchmaking: join request from ")) + std::to_string(xuid) + cx("."));

        engine::msg_join_lobby_s request = {};

        if (!engine::lobby_msg_join_package_fn(&request, lobby_msg))
        {
            return false;
        }

        engine::msg_join_response_s response = {};

        response.lobby_type = request.target_lobby;

        response.lobby_params.network_mode = *reinterpret_cast<engine::lobby_network_mode_e*>(engine::base() + engine::network_mode_global);

        response.lobby_params.main_mode = *reinterpret_cast<engine::lobby_main_mode_e*>(engine::base() + engine::main_mode_global);

        response.response = engine::join_result_success;

        strcpy_s(response.name, sizeof(response.name), cx("reworked").c_str());

        response.server_location = 0;

        response.reservation_key = 69;

        int channel = engine::lobby_netchan_global_channel_fn(0);

        if (!engine::lobby_send_join_response_fn(0, channel, engine::lobby_module_host, adr, xuid, &response))
        {
            T7_LOG(std::string(cx("matchmaking: join response failed for ")) + std::to_string(xuid) + cx("."));

            return false;
        }

        return true;
    }

    static int64_t __fastcall hk_handle_member_info(int controller, engine::netadr_s adr, int64_t xuid, engine::lobby_msg_s* lobby_msg)
    {
        T7_LOG(std::string(cx("matchmaking: member info from ")) + std::to_string(xuid) + cx("."));

        engine::msg_join_member_info_s info = {};

        if (!engine::lobby_package_glob_fn(lobby_msg, cx("serializedadr").c_str(), info.serialized_adr.xnaddr, 37, 0x25))
        {
            return 0;
        }

        if (!engine::lobby_package_uint64(lobby_msg, cx("reservationkey").c_str(), &info.reservation_key))
        {
            return 0;
        }

        if (!engine::lobby_package_int(lobby_msg, cx("lobbytype").c_str(), &info.target_lobby))
        {
            return 0;
        }

        if (!engine::msg_fixed_client_info_package_fn(&info.fixed_client_info_msg, lobby_msg))
        {
            return 0;
        }

        if (!engine::msg_mutable_client_info_package_fn(&info.mutable_client_info_msg, lobby_msg))
        {
            return 0;
        }

        void* state = engine::lobby_host_lobby_state_fn(info.target_lobby);

        if (state != nullptr)
        {
            int client_count = engine::lobby_session_client_count_fn(state, 0);

            if (client_count >= 18)
            {
                T7_LOG(std::string(cx("matchmaking: lobby full (")) + std::to_string(client_count) + cx("/18), rejected ") + std::to_string(xuid) + cx("."));

                return 0;
            }
        }

        engine::lobby_add_joining_client_fn(0, adr, xuid, &info, 69);

        return 0;
    }

    void initialize()
    {
        engine::lobby_msg_join_package_fn = reinterpret_cast<engine::lobby_msg_join_package_t>(engine::base() + engine::lobby_msg_join_package);

        engine::lobby_send_join_response_fn = reinterpret_cast<engine::lobby_send_join_response_t>(engine::base() + engine::lobby_send_join_response);

        engine::lobby_netchan_global_channel_fn = reinterpret_cast<engine::lobby_netchan_global_channel_t>(engine::base() + engine::lobby_netchan_get_global_channel);

        engine::lobby_host_lobby_state_fn = reinterpret_cast<engine::lobby_host_lobby_state_t>(engine::base() + engine::lobby_host_session_get_lobby_state);

        engine::lobby_session_client_count_fn = reinterpret_cast<engine::lobby_session_client_count_t>(engine::base() + engine::lobby_session_get_client_count);

        engine::lobby_add_joining_client_fn = reinterpret_cast<engine::lobby_add_joining_client_t>(engine::base() + engine::lobby_host_add_joining_client);

        engine::lobby_package_glob_fn = reinterpret_cast<engine::lobby_package_glob_t>(engine::base() + engine::lobby_msg_package_glob);

        engine::msg_fixed_client_info_package_fn = reinterpret_cast<engine::msg_fixed_client_info_package_t>(engine::base() + engine::msg_fixed_client_info_package);

        engine::msg_mutable_client_info_package_fn = reinterpret_cast<engine::msg_mutable_client_info_package_t>(engine::base() + engine::msg_mutable_client_info_package);

        engine::lobby_handle_join_request_fn = reinterpret_cast<engine::lobby_handle_join_request_t>(engine::base() + engine::lobby_handle_join_request);

        utils::hook::attach(reinterpret_cast<void**>(&engine::lobby_handle_join_request_fn), hk_handle_join_request);

        engine::lobby_handle_member_info_fn = reinterpret_cast<engine::lobby_handle_member_info_t>(engine::base() + engine::lobby_handle_member_info);

        utils::hook::attach(reinterpret_cast<void**>(&engine::lobby_handle_member_info_fn), hk_handle_member_info);

        engine::lobby_host_data_get_session_fn = reinterpret_cast<engine::lobby_host_data_get_session_t>(engine::base() + engine::lobby_host_data_get_session);

        engine::lobby_qos_listener_enable_fn = reinterpret_cast<engine::lobby_qos_listener_enable_t>(engine::base() + engine::lobby_qos_listener_enable);

        engine::live_session_update_fn = reinterpret_cast<engine::live_session_update_t>(engine::base() + engine::live_session_update);

        engine::lobby_online_create_session_fn = reinterpret_cast<engine::lobby_online_create_session_t>(engine::base() + engine::lobby_online_create_session);

        engine::live_storage_get_ffotd_version_fn = reinterpret_cast<engine::live_storage_get_ffotd_version_t>(engine::base() + engine::live_storage_get_ffotd_version);

        engine::playlist_get_version_number_fn = reinterpret_cast<engine::playlist_get_version_number_t>(engine::base() + engine::playlist_get_version_number);

        engine::lobby_set_matchmaking_info_fn = reinterpret_cast<engine::lobby_set_matchmaking_info_t>(engine::base() + engine::lobby_set_matchmaking_info);

        utils::hook::attach(reinterpret_cast<void**>(&engine::lobby_set_matchmaking_info_fn), hk_set_matchmaking_info);

        engine::lobby_host_update_matchmaking_session_fn = reinterpret_cast<engine::lobby_host_update_matchmaking_session_t>(engine::base() + engine::lobby_host_update_matchmaking_session);

        utils::hook::attach(reinterpret_cast<void**>(&engine::lobby_host_update_matchmaking_session_fn), hk_lobby_host_update_matchmaking_session);

        engine::lobby_session_set_max_clients_fn = reinterpret_cast<engine::lobby_session_set_max_clients_t>(engine::base() + engine::lobby_session_set_max_clients);

        utils::hook::attach(reinterpret_cast<void**>(&engine::lobby_session_set_max_clients_fn), hk_lobby_session_set_max_clients);

        engine::lobby_host_task_search_fn = reinterpret_cast<engine::lobby_host_task_search_t>(engine::base() + engine::lobby_host_task_search);

        utils::hook::attach(reinterpret_cast<void**>(&engine::lobby_host_task_search_fn), hk_lobby_host_task_search);

        engine::live_can_host_server_fn = reinterpret_cast<engine::live_can_host_server_t>(engine::base() + engine::live_can_host_server);

        utils::hook::attach(reinterpret_cast<void**>(&engine::live_can_host_server_fn), hk_live_can_host_server);

        engine::bd_byte_buffer_write_uint32_fn = reinterpret_cast<engine::bd_byte_buffer_write_uint32_t>(engine::base() + engine::bd_byte_buffer_write_uint32);

        engine::bd_session_id_serialize_fn = reinterpret_cast<engine::bd_session_id_serialize_t>(engine::base() + engine::bd_session_id_serialize);

        utils::hook::attach(reinterpret_cast<void**>(&engine::bd_session_id_serialize_fn), hk_bd_session_id_serialize);

        uint8_t dedicated = 0x0C;

        utils::mem::patch(reinterpret_cast<void*>(engine::base() + engine::dedicated_session_flag), &dedicated, 1);

        T7_LOG(cx("matchmaking: join handlers + lobby advertise installed."));
    }
}
