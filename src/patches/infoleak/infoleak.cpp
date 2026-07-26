#include "infoleak.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace infoleak
{
    static bool enforce = true;

    static bool netadr_equal(const engine::netadr_s* a, const engine::netadr_s* b)
    {
        return a->type == b->type && a->port == b->port && memcmp(a->ip, b->ip, 4) == 0;
    }

    static const engine::netadr_s* session_host(void* lobby, uint64_t self_xuid)
    {
        if (lobby == nullptr)
        {
            return nullptr;
        }

        uint64_t host_xuid = engine::lobby_host_xuid(lobby);

        if (host_xuid == 0 || host_xuid == self_xuid)
        {
            return nullptr;
        }

        return engine::lobby_host_netadr(lobby);
    }

    static void* __fastcall hk_steam_gs_pump(void* server)
    {
        if (enforce)
        {
            return nullptr;
        }

        return engine::steam_gs_pump_fn(server);
    }

    static void __fastcall hk_rich_presence_null(uint64_t steam_id)
    {
    }

    static bool __fastcall hk_net_send_packet(uint32_t sock, uint32_t length, const void* data, const void* to)
    {
        const engine::netadr_s* dest = static_cast<const engine::netadr_s*>(to);

        if (engine::protection.info_leak && dest != nullptr && dest->type == engine::netadr_type_raw_udp)
        {
            uint64_t self = static_cast<uint64_t>(engine::user_xuid(0));

            const engine::netadr_s* host = session_host(engine::lobby_game(0), self);

            if (host == nullptr)
            {
                host = session_host(engine::lobby_party(0), self);
            }

            if (host != nullptr && !netadr_equal(dest, host))
            {
                char endpoint[48];

                snprintf(endpoint, sizeof(endpoint), "%u.%u.%u.%u:%u", dest->ip[0], dest->ip[1], dest->ip[2], dest->ip[3], dest->port);

                T7_LOG(std::string(cx("infoleak: raw-udp to non-host ")) + endpoint + (enforce ? cx(" dropped") : cx(" (log only)")) + cx("."));

                if (enforce)
                {
                    return false;
                }
            }
        }

        return engine::net_send_packet(sock, length, data, to);
    }

    void initialize()
    {
        engine::net_send_packet = reinterpret_cast<engine::net_send_packet_t>(engine::base() + engine::net_send);

        utils::hook::attach(reinterpret_cast<void**>(&engine::net_send_packet), hk_net_send_packet);

        engine::steam_gs_pump_fn = reinterpret_cast<engine::steam_gs_pump_t>(engine::base() + engine::steam_gs_pump);

        utils::hook::attach(reinterpret_cast<void**>(&engine::steam_gs_pump_fn), hk_steam_gs_pump);

        engine::steam_rich_presence_friendadd_fn = reinterpret_cast<engine::steam_rich_presence_t>(engine::base() + engine::steam_rich_presence_friendadd);

        utils::hook::attach(reinterpret_cast<void**>(&engine::steam_rich_presence_friendadd_fn), hk_rich_presence_null);

        engine::steam_rich_presence_steamid_fn = reinterpret_cast<engine::steam_rich_presence_t>(engine::base() + engine::steam_rich_presence_steamid);

        utils::hook::attach(reinterpret_cast<void**>(&engine::steam_rich_presence_steamid_fn), hk_rich_presence_null);

        T7_LOG(cx("infoleak: ip leak protection installed (+ steam rich-presence nulled)."));
    }
}
