#include "lobbymsg.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"
#include "../../features/overlay/overlay.hpp"
#include "../../features/serverlist/serverlist.hpp"

#include <cstdint>
#include <cstring>
#include <string>

namespace lobbymsg
{
    static auto& array_cap()
    {
        struct array_state
        {
            int count;

            int limit;

            const char* key;
        };

        static thread_local array_state state = {};

        return state;
    }

    static bool __fastcall hk_array_start(engine::lobby_msg_s* lobby_msg, const char* key)
    {
        if (key != nullptr)
        {
            auto& cap = array_cap();

            cap.count = 0;

            cap.key = key;

            if (strcmp(key, "votes") == 0)
            {
                cap.limit = 216;
            }
            else if (strcmp(key, "nomineelist") == 0 || strcmp(key, "clientlist") == 0 || strcmp(key, "members") == 0)
            {
                cap.limit = 18;
            }
            else
            {
                cap.limit = 256;
            }
        }

        return engine::lobby_package_array_start(lobby_msg, key);
    }

    static char __fastcall hk_array_element(engine::lobby_msg_s* lobby_msg, char add_element)
    {
        char present = engine::lobby_package_element(lobby_msg, add_element);

        auto& cap = array_cap();

        if (engine::protection.lobby_messages && present != 0 && lobby_msg != nullptr && lobby_msg->mode == 2 && ++cap.count > cap.limit)
        {
            if (cap.count == cap.limit + 1)
            {
                T7_LOG(std::string(cx("lobbymsg: array overflow (")) + (cap.key != nullptr ? cap.key : "?") + cx(" capped at ") + std::to_string(cap.limit) + cx("), dropped."));

                features::overlay::notify(cx("blocked crash attempt."), features::overlay::level::bad);
            }

            return 0;
        }

        return present;
    }

    static bool lobbytype_ok(int32_t lobby_type)
    {
        if (static_cast<uint32_t>(lobby_type) > 1)
        {
            T7_LOG(std::string(cx("lobbymsg: bad lobbytype ")) + std::to_string(lobby_type) + cx(", dropped."));

            return false;
        }

        return true;
    }

    static bool lobby_lobbytype_valid(engine::lobby_msg_s* lobby_msg)
    {
        int32_t lobby_type = -1;

        engine::lobby_package_int(lobby_msg, "lobbytype", &lobby_type);

        return lobbytype_ok(lobby_type);
    }

    static bool lobby_join_active()
    {
        return *reinterpret_cast<const int32_t*>(engine::base() + engine::lobby_join_state) != 0;
    }

    static bool lobby_xuid_is_sender(engine::lobby_msg_s* lobby_msg, uint64_t sender_xuid)
    {
        uint64_t claimed_xuid = 0;

        engine::lobby_package_xuid(lobby_msg, "xuid", &claimed_xuid);

        if (claimed_xuid != sender_xuid)
        {
            T7_LOG(std::string(cx("lobbymsg: spoofed sender ")) + std::to_string(sender_xuid) + cx(" claimed ") + std::to_string(claimed_xuid) + cx(", dropped."));

            features::overlay::notify(cx("blocked spoofed lobby packet."), features::overlay::level::bad);

            return false;
        }

        return true;
    }

    static bool lobby_voice_packet(engine::lobby_msg_s* lobby_msg)
    {
        constexpr int voice_data_max = 1198;

        uint8_t talker = 0;

        int32_t relay_bits = 0;

        uint16_t voice_size = 0;

        uint8_t packet_count = 0;

        uint8_t voice_data[voice_data_max] = {};

        engine::lobby_package_uchar(lobby_msg, "talker", &talker);

        engine::lobby_package_int(lobby_msg, "relaybits", &relay_bits);

        engine::lobby_package_ushort(lobby_msg, "sizeofvoicedata", &voice_size);

        engine::lobby_package_uchar(lobby_msg, "numpackets", &packet_count);

        engine::lobby_package_data(lobby_msg, "voicedata", voice_data, voice_size, voice_data_max);

        if (voice_size > voice_data_max)
        {
            T7_LOG(std::string(cx("lobbymsg: voice_packet voicesize ")) + std::to_string(voice_size) + cx(", dropped."));

            return false;
        }

        uint32_t offset = 0;

        for (uint32_t packet = 0; packet < packet_count; packet++)
        {
            if (offset >= voice_size)
            {
                T7_LOG(std::string(cx("lobbymsg: voice_packet oob offset ")) + std::to_string(offset) + cx(" size ") + std::to_string(voice_size) + cx(", dropped."));

                features::overlay::notify(cx("blocked crash attempt."), features::overlay::level::bad);

                return false;
            }

            uint32_t length = voice_data[offset];

            if (offset + length > voice_size)
            {
                T7_LOG(std::string(cx("lobbymsg: voice_packet oob end ")) + std::to_string(offset + length) + cx(" size ") + std::to_string(voice_size) + cx(", dropped."));

                features::overlay::notify(cx("blocked crash attempt."), features::overlay::level::bad);

                return false;
            }

            offset += length;
        }

        return true;
    }

    static bool lobby_client_content(engine::lobby_msg_s* lobby_msg)
    {
        uint32_t datamask = 0;

        int32_t lobby_type = -1;

        engine::lobby_package_uint(lobby_msg, "datamask", &datamask);

        engine::lobby_package_int(lobby_msg, "lobbytype", &lobby_type);

        return lobbytype_ok(lobby_type);
    }

    static bool lobby_client_reliable_data(engine::lobby_msg_s* lobby_msg, uint64_t sender_xuid)
    {
        constexpr uint32_t datamask_newleader = 0x2;

        constexpr uint32_t datamask_discclient = 0x4;

        constexpr uint32_t datamask_platformsession = 0x800;

        uint32_t datamask = 0;

        int32_t lobby_type = -1;

        engine::lobby_package_uint(lobby_msg, "datamask", &datamask);

        engine::lobby_package_int(lobby_msg, "lobbytype", &lobby_type);

        if (!lobbytype_ok(lobby_type))
        {
            return false;
        }

        uint64_t xuid_scratch = 0;

        int32_t int_scratch = 0;

        if ((datamask & datamask_newleader) != 0)
        {
            engine::lobby_package_xuid(lobby_msg, "newleader", &xuid_scratch);
        }

        if ((datamask & datamask_platformsession) != 0)
        {
            engine::lobby_package_xuid(lobby_msg, "platformsession", &xuid_scratch);
        }

        if ((datamask & datamask_discclient) != 0)
        {
            uint64_t disc_xuid = 0;

            engine::lobby_package_xuid(lobby_msg, "discclientxuid", &disc_xuid);

            engine::lobby_package_int(lobby_msg, "discclient", &int_scratch);

            if (disc_xuid != sender_xuid)
            {
                T7_LOG(std::string(cx("lobbymsg: reliable_data ")) + std::to_string(sender_xuid) + cx(" spoofed ") + std::to_string(disc_xuid) + cx(", dropped."));

                features::overlay::notify(cx("blocked spoofed lobby packet."), features::overlay::level::bad);

                return false;
            }
        }

        return true;
    }

    static void parse_probe_state(engine::lobby_msg_s* lobby_msg, uint64_t host_xuid)
    {
        static uint8_t state[0x8000];

        if (engine::msg_lobby_state_package_fn == nullptr)
        {
            return;
        }

        memset(state, 0, sizeof(state));

        if (!engine::msg_lobby_state_package_fn(state, lobby_msg))
        {
            return;
        }

        serverlist::player players[18] = {};

        int found = 0;

        for (int i = 0; i < 18; i++)
        {
            uint8_t* c = state + 104 + 1168 * i;

            uint64_t client_xuid = *reinterpret_cast<uint64_t*>(c);

            if (client_xuid == 0)
            {
                continue;
            }

            players[found].xuid = client_xuid;

            players[found].client_num = *reinterpret_cast<uint8_t*>(c + 8);

            strncpy_s(players[found].name, sizeof(players[found].name), reinterpret_cast<const char*>(c + 9), 32);

            strncpy_s(players[found].clan, sizeof(players[found].clan), reinterpret_cast<const char*>(c + 72), 4);

            found++;
        }

        uint64_t steam_lobby_id = *reinterpret_cast<uint64_t*>(state + 80);

        serverlist::set_players(host_xuid, players, found, steam_lobby_id);
    }

    static uint64_t parse_join_response_key(engine::lobby_msg_s* lobby_msg)
    {
        int response = 0;

        char name[36] = {};

        uint32_t server_location = 0;

        int lobby_type_field = 0;

        int network_mode = 0;

        int main_mode = 0;

        uint64_t reservation_key = 0;

        engine::lobby_package_int(lobby_msg, "response", &response);

        engine::lobby_package_string(lobby_msg, "name", name, 32);

        engine::lobby_package_uint(lobby_msg, "serverlocation", &server_location);

        engine::lobby_package_int(lobby_msg, "lobbytype", &lobby_type_field);

        engine::lobby_package_int(lobby_msg, "networkmode", &network_mode);

        if (engine::lobby_package_int(lobby_msg, "mainmode", &main_mode))
        {
            engine::lobby_package_uint64(lobby_msg, "reservationkey", &reservation_key);
        }

        return reservation_key;
    }

    static int64_t __fastcall hk_handle_packet_internal(uint32_t controller_index, void* adr, uint64_t xuid, int64_t lobby_type, int role, void* msg)
    {
        if (!engine::protection.lobby_messages)
        {
            return engine::handle_packet_internal(controller_index, adr, xuid, lobby_type, role, msg);
        }

        engine::lobby_msg_s lobby_msg = {};

        engine::lobby_prep_read_msg(&lobby_msg, msg);

        engine::message_type type = lobby_msg.type;

        const char* name = type < engine::message_type_count ? engine::lobby_type_name(type) : nullptr;

        T7_LOG(std::string(cx("lobbymsg: ")) + (name != nullptr ? name : cx("unknown")) + " (" + std::to_string(static_cast<uint32_t>(type)) + cx(") from ") + std::to_string(xuid) + cx("."));

        switch (type)
        {
            case engine::message_type_lobby_client_disconnect:
            case engine::message_type_lobby_host_disconnect:
            {
                if (!lobby_lobbytype_valid(&lobby_msg) || !lobby_xuid_is_sender(&lobby_msg, xuid))
                {
                    return 0;
                }
            }
            break;

            case engine::message_type_voice_packet:
            case engine::message_type_voice_relay_packet:
            {
                if (!lobby_lobbytype_valid(&lobby_msg) || !lobby_voice_packet(&lobby_msg))
                {
                    return 0;
                }
            }
            break;

            case engine::message_type_lobby_migrate_announce_host:
            case engine::message_type_ingame_migrate_to:
            case engine::message_type_ingame_migrate_new_host:
            case engine::message_type_lobby_migrate_test:
            case engine::message_type_lobby_migrate_start:
            case engine::message_type_lobby_host_disconnect_client:
            case engine::message_type_lobby_host_leave_with_party:
            {
                if (!lobby_lobbytype_valid(&lobby_msg))
                {
                    return 0;
                }
            }
            break;

            case engine::message_type_lobby_client_reliable_data:
            {
                if (!lobby_client_reliable_data(&lobby_msg, xuid))
                {
                    return 0;
                }
            }
            break;

            case engine::message_type_lobby_client_content:
            {
                if (!lobby_client_content(&lobby_msg))
                {
                    return 0;
                }
            }
            break;

            case engine::message_type_join_response:
            {
                if (serverlist::wants(xuid))
                {
                    uint64_t reservation_key = parse_join_response_key(&lobby_msg);

                    serverlist::send_member_info(xuid, adr, reservation_key);

                    return 0;
                }
            }
            break;

            case engine::message_type_lobby_state_game:
            case engine::message_type_lobby_state_gamepublic:
            case engine::message_type_lobby_state_gamecustom:
            case engine::message_type_lobby_state_gametheater:
            {
                if (serverlist::wants(xuid))
                {
                    parse_probe_state(&lobby_msg, xuid);

                    return 0;
                }
            }
            break;

            case engine::message_type_join_agreement_request:
            case engine::message_type_join_complete:
            {
                bool hosting = engine::lobby_host_data_get_session_fn != nullptr && engine::lobby_host_data_get_session_fn(engine::lobby_type_game) != nullptr;

                if (!hosting && !lobby_join_active())
                {
                    T7_LOG(std::string(cx("lobbymsg: pull not joining from ")) + std::to_string(xuid) + cx(", dropped."));

                    features::overlay::notify(cx("blocked pull attempt."), features::overlay::level::bad);

                    return 0;
                }
            }
            break;
        }

        return engine::handle_packet_internal(controller_index, adr, xuid, lobby_type, role, msg);
    }

    static int64_t __fastcall hk_lobby_print_debug(int64_t msg)
    {
        if (engine::protection.lobby_messages && engine::lobby_print_depth >= engine::lobby_print_max_depth)
        {
            if (msg != 0)
            {
                *reinterpret_cast<uint32_t*>(msg) = 1;
            }

            if (engine::lobby_print_blocked++ % 64 == 0)
            {
                T7_LOG(cx("lobbymsg: deep-nested print recursion dropped."));

                features::overlay::notify(cx("blocked crash attempt."), features::overlay::level::bad);
            }

            return 16;
        }

        engine::lobby_print_depth++;

        int64_t result = engine::lobby_print_debug_fn(msg);

        engine::lobby_print_depth--;

        return result;
    }

    static char __fastcall hk_lobby_print_message(int64_t msg, char force)
    {
        engine::lobby_print_depth = 0;

        return engine::lobby_print_message_fn(msg, force);
    }

    static char __fastcall hk_lobby_print_array(int64_t msg, int indent)
    {
        if (engine::protection.lobby_messages && engine::lobby_print_depth >= engine::lobby_print_max_depth)
        {
            if (msg != 0)
            {
                *reinterpret_cast<uint32_t*>(msg) = 1;
            }

            if (engine::lobby_print_blocked++ % 64 == 0)
            {
                T7_LOG(cx("lobbymsg: deep-nested print array dropped."));

                features::overlay::notify(cx("blocked crash attempt."), features::overlay::level::bad);
            }

            return 14;
        }

        engine::lobby_print_depth++;

        char result = engine::lobby_print_array_fn(msg, indent);

        engine::lobby_print_depth--;

        return result;
    }

    void initialize()
    {
        engine::lobby_prep_read_msg = reinterpret_cast<engine::lobby_prep_read_msg_t>(engine::base() + engine::lobby_prep_read);

        engine::msg_lobby_state_package_fn = reinterpret_cast<engine::msg_lobby_state_package_t>(engine::base() + engine::msg_lobby_state_package);

        engine::lobby_type_name = reinterpret_cast<engine::lobby_type_name_t>(engine::base() + engine::lobby_type_name_fn);

        engine::lobby_package_int = reinterpret_cast<engine::lobby_package_int_t>(engine::base() + engine::lobby_pkg_int);

        engine::lobby_package_xuid = reinterpret_cast<engine::lobby_package_xuid_t>(engine::base() + engine::lobby_pkg_xuid);

        engine::lobby_package_array_start = reinterpret_cast<engine::lobby_package_array_start_t>(engine::base() + engine::lobby_pkg_array_start);

        utils::hook::attach(reinterpret_cast<void**>(&engine::lobby_package_array_start), hk_array_start);

        engine::lobby_package_element = reinterpret_cast<engine::lobby_package_element_t>(engine::base() + engine::lobby_pkg_element);

        utils::hook::attach(reinterpret_cast<void**>(&engine::lobby_package_element), hk_array_element);

        engine::lobby_package_short = reinterpret_cast<engine::lobby_package_short_t>(engine::base() + engine::lobby_pkg_short);

        engine::lobby_package_uint = reinterpret_cast<engine::lobby_package_uint_t>(engine::base() + engine::lobby_pkg_uint);

        engine::lobby_package_uint64 = reinterpret_cast<engine::lobby_package_uint64_t>(engine::base() + engine::lobby_pkg_uint64);

        engine::lobby_package_uchar = reinterpret_cast<engine::lobby_package_uchar_t>(engine::base() + engine::lobby_pkg_uchar);

        engine::lobby_package_bool = reinterpret_cast<engine::lobby_package_bool_t>(engine::base() + engine::lobby_pkg_bool);

        engine::lobby_package_string = reinterpret_cast<engine::lobby_package_string_t>(engine::base() + engine::lobby_pkg_string);

        engine::lobby_package_ushort = reinterpret_cast<engine::lobby_package_ushort_t>(engine::base() + engine::lobby_pkg_ushort);

        engine::lobby_package_data = reinterpret_cast<engine::lobby_package_data_t>(engine::base() + engine::lobby_pkg_data);

        engine::handle_packet_internal = reinterpret_cast<engine::handle_packet_internal_t>(engine::base() + engine::lobby_handle_packet_internal);

        utils::hook::attach(reinterpret_cast<void**>(&engine::handle_packet_internal), hk_handle_packet_internal);

        engine::lobby_print_debug_fn = reinterpret_cast<engine::lobby_print_debug_t>(engine::base() + engine::lobby_print_debug);

        utils::hook::attach(reinterpret_cast<void**>(&engine::lobby_print_debug_fn), hk_lobby_print_debug);

        engine::lobby_print_array_fn = reinterpret_cast<engine::lobby_print_array_t>(engine::base() + engine::lobby_print_array);

        utils::hook::attach(reinterpret_cast<void**>(&engine::lobby_print_array_fn), hk_lobby_print_array);

        engine::lobby_print_message_fn = reinterpret_cast<engine::lobby_print_message_t>(engine::base() + engine::lobby_print_message);

        utils::hook::attach(reinterpret_cast<void**>(&engine::lobby_print_message_fn), hk_lobby_print_message);

        T7_LOG(cx("lobbymsg: handle_packet_internal + print-recursion protections installed."));
    }
}
