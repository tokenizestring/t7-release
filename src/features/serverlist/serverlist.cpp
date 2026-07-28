#include "serverlist.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>

namespace serverlist
{
    static constexpr int mm_stride = 0x1C0;

    static constexpr int mm_count = 750;

    static constexpr int off_host_addr = 0x28;

    static constexpr int off_max_players = 0x130;

    static constexpr int off_num_players = 0x134;

    static constexpr int off_session_id = 0x138;

    static constexpr int off_key_exchange = 0x140;

    static constexpr int off_server_type = 0x154;

    static constexpr int off_xuid = 0x158;

    static constexpr int off_xn_public = 0x1E;

    static constexpr int off_xn_port = 0x22;

    static bool g_active = false;

    static bool g_alloced = false;

    static bool g_inited = false;

    static CRITICAL_SECTION g_lock;

    static uint8_t g_results[mm_count * mm_stride] = {};

    static std::vector<entry> g_entries;

    static bool is_dedicated(uint32_t server_type)
    {
        return server_type == 2000;
    }

    static int find_by_xuid(uint64_t xuid)
    {
        for (int i = 0; i < static_cast<int>(g_entries.size()); i++)
        {
            if (g_entries[i].xuid == xuid)
            {
                return i;
            }
        }

        return -1;
    }

    static void __fastcall hk_find_sessions(uintptr_t matchmaking, void* remote_task, int query_id, uint32_t start_index, uint32_t max_num_results, void* query, int64_t results)
    {
        engine::bd_find_sessions_fn(matchmaking, remote_task, query_id, start_index, mm_count, query, reinterpret_cast<int64_t>(g_results));
    }

    static void alloc_results()
    {
        if (g_alloced || engine::allocate_structs_fn == nullptr)
        {
            return;
        }

        uint8_t* base = reinterpret_cast<uint8_t*>(engine::base());

        engine::allocate_structs_fn(g_results, mm_stride, mm_count, base + engine::matchmaking_info_ctor, base + engine::matchmaking_thing_ctor);

        g_alloced = true;
    }

    static void fetch_batch()
    {
        void* session = engine::lobby_session_get_fn(engine::lobby_type_game);

        if (session == nullptr)
        {
            return;
        }

        engine::lobby_search_session_fn(session);

        EnterCriticalSection(&g_lock);

        for (int i = 0; i < mm_count; i++)
        {
            uint8_t* e = g_results + i * mm_stride;

            uint64_t xuid = *reinterpret_cast<uint64_t*>(e + off_xuid);

            if (xuid == 0)
            {
                continue;
            }

            uint32_t num_players = *reinterpret_cast<uint32_t*>(e + off_num_players);

            uint32_t max_players = *reinterpret_cast<uint32_t*>(e + off_max_players);

            uint32_t server_type = *reinterpret_cast<uint32_t*>(e + off_server_type);

            int index = find_by_xuid(xuid);

            if (index != -1)
            {
                g_entries[index].num_players = num_players;

                g_entries[index].max_players = max_players;

                continue;
            }

            if (num_players == 0)
            {
                continue;
            }

            uint8_t* xn = e + off_host_addr;

            uint8_t* ip = xn + off_xn_public;

            uint16_t port_raw = *reinterpret_cast<uint16_t*>(xn + off_xn_port);

            entry se = {};

            se.xuid = xuid;

            snprintf(se.ip, sizeof(se.ip), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);

            se.port = static_cast<uint16_t>((port_raw >> 8) | (port_raw << 8));

            se.num_players = num_players;

            se.max_players = max_players;

            se.server_type = server_type;

            se.dedicated = is_dedicated(server_type);

            memcpy(se.xnaddr, xn, sizeof(se.xnaddr));

            memcpy(se.sec_id, e + off_session_id, sizeof(se.sec_id));

            memcpy(se.sec_key, e + off_key_exchange, sizeof(se.sec_key));

            g_entries.push_back(se);
        }

        g_entries.erase(std::remove_if(g_entries.begin(), g_entries.end(), [](const entry& x) { return x.num_players == 0; }), g_entries.end());

        std::sort(g_entries.begin(), g_entries.end(), [](const entry& a, const entry& b) { return a.num_players > b.num_players; });

        LeaveCriticalSection(&g_lock);
    }

    static bool g_ready = false;

    static void probe_all()
    {
        int total = count();

        for (int i = 0; i < total; i++)
        {
            bool dedi = false;

            bool full = false;

            uint64_t lobby_id = 0;

            EnterCriticalSection(&g_lock);

            if (i < static_cast<int>(g_entries.size()))
            {
                entry& e = g_entries[i];

                dedi = e.dedicated;

                full = e.max_players != 0 && e.num_players >= e.max_players;

                lobby_id = e.steam_lobby_id;
            }

            LeaveCriticalSection(&g_lock);

            if (dedi && !full)
            {
                dispatch_probe(i);

                if (lobby_id != 0)
                {
                    join_steam_lobby(i);
                }
            }
        }
    }

    void tick()
    {
        if ((!g_active && !auto_probe) || engine::lobby_search_session_fn == nullptr || engine::demonware_fetching_done_fn == nullptr)
        {
            return;
        }

        uint64_t now = GetTickCount64();

        static uint64_t last_fetch = 0;

        if (last_fetch == 0 || now - last_fetch >= 1000)
        {
            last_fetch = now;

            g_ready = engine::demonware_fetching_done_fn(0) && engine::demonware_signed_in_fn(0);

            if (g_ready)
            {
                alloc_results();

                if (g_alloced)
                {
                    fetch_batch();

                    if (auto_probe)
                    {
                        probe_all();
                    }
                }
            }
        }
    }

    void dispatch_probe(int index)
    {
        if (engine::send_join_lobby_fn == nullptr)
        {
            return;
        }

        engine::netadr_s adr = {};

        uint64_t target_xuid = 0;

        EnterCriticalSection(&g_lock);

        if (index >= 0 && index < static_cast<int>(g_entries.size()))
        {
            entry& e = g_entries[index];

            target_xuid = e.xuid;

            engine::dw_register_sec_fn(e.sec_id, e.sec_key);

            engine::dw_common_to_netadr_fn(&adr, e.xnaddr, e.sec_id);
        }

        LeaveCriticalSection(&g_lock);

        if (target_xuid == 0)
        {
            return;
        }

        engine::msg_join_lobby_s msg = {};

        int channel = engine::lobby_netchan_global_channel_fn(0);

        uint64_t my_xuid = static_cast<uint64_t>(engine::user_xuid(0));

        msg.target_lobby = engine::lobby_type_game;

        msg.source_lobby = engine::lobby_type_game;

        msg.join_type = 0;

        msg.probed_xuid = target_xuid;

        msg.member_count = 1;

        msg.split_screen_clients = 0;

        msg.playlist_id = 1;

        msg.playlist_ver = engine::playlist_get_version_number_fn();

        msg.ffotd_ver = engine::live_storage_get_ffotd_version_fn();

        msg.network_mode = 2;

        msg.net_checksum = engine::net_field_checksum_fn();

        msg.protocol = 18532;

        msg.change_list = 13892626;

        msg.dlc_bits = engine::content_packs_fn(1);

        msg.join_nonce = 0;

        msg.is_starter_pack = false;

        msg.chunk[0] = 3;

        msg.chunk[1] = 3;

        msg.chunk[2] = 3;

        msg.members[0].xuid = my_xuid;

        msg.members[0].lobby_id = my_xuid;

        msg.members[0].skill_rating = 0.0f;

        msg.members[0].skill_variance = 1.0f;

        engine::send_join_lobby_fn(0, channel, engine::lobby_module_host, adr, target_xuid, &msg);

        T7_LOG(std::string(cx("serverlist: probe dispatched to ")) + std::to_string(target_xuid) + cx("."));
    }

    static uint8_t* get_our_client(uint8_t** out_session, int* out_type)
    {
        if (engine::lobby_session_get_fn == nullptr)
        {
            return nullptr;
        }

        int types[2] = { engine::lobby_type_game, 0 };

        for (int ti = 0; ti < 2; ti++)
        {
            uint8_t* s = reinterpret_cast<uint8_t*>(engine::lobby_session_get_fn(types[ti]));

            if (reinterpret_cast<uintptr_t>(s) < 0x10000)
            {
                continue;
            }

            uint8_t* member0 = s + engine::lobby_member_array;

            if (*reinterpret_cast<uint64_t*>(member0) == 0)
            {
                continue;
            }

            uint8_t* ac = *reinterpret_cast<uint8_t**>(member0 + engine::lobby_member_client);

            if (reinterpret_cast<uintptr_t>(ac) >= 0x10000)
            {
                *out_session = s;

                *out_type = types[ti];

                return ac;
            }
        }

        return nullptr;
    }

    void send_member_info(uint64_t host_xuid, void* adr, uint64_t reservation_key)
    {
        if (engine::send_member_info_fn == nullptr || adr == nullptr)
        {
            return;
        }

        uint8_t* session = nullptr;

        int type = 0;

        uint8_t* ac = get_our_client(&session, &type);

        if (ac == nullptr)
        {
            return;
        }

        engine::msg_join_member_info_s info = {};

        memcpy(info.mutable_client_info_msg.data, ac, 1040);

        memcpy(info.fixed_client_info_msg.data, ac + 1040, 208);

        info.reservation_key = reservation_key;

        info.target_lobby = engine::lobby_type_game;

        info.serialized_adr.valid = 1;

        if (engine::dw_netadr_to_common_fn != nullptr)
        {
            engine::netadr_s my_netadr = {};

            memcpy(&my_netadr, ac + 1248 + 32 * type + 4, sizeof(my_netadr));

            engine::dw_netadr_to_common_fn(my_netadr, info.serialized_adr.xnaddr, sizeof(info.serialized_adr.xnaddr), nullptr);
        }

        int channel = engine::lobby_netchan_global_channel_fn(0);

        engine::netadr_s dest = *reinterpret_cast<engine::netadr_s*>(adr);

        engine::send_member_info_fn(0, channel, engine::lobby_module_host, dest, host_xuid, &info);

        T7_LOG(std::string(cx("serverlist: member info sent to ")) + std::to_string(host_xuid) + cx("."));
    }

    bool wants(uint64_t host_xuid)
    {
        EnterCriticalSection(&g_lock);

        bool found = find_by_xuid(host_xuid) != -1;

        LeaveCriticalSection(&g_lock);

        return found;
    }

    void set_players(uint64_t host_xuid, const player* list, int list_count, uint64_t steam_lobby_id)
    {
        EnterCriticalSection(&g_lock);

        int index = find_by_xuid(host_xuid);

        if (index != -1)
        {
            int n = list_count > 18 ? 18 : (list_count < 0 ? 0 : list_count);

            g_entries[index].player_count = n;

            for (int i = 0; i < n; i++)
            {
                g_entries[index].players[i] = list[i];
            }

            g_entries[index].probed = true;

            g_entries[index].last_state_ms = GetTickCount64();

            if (steam_lobby_id != 0)
            {
                g_entries[index].steam_lobby_id = steam_lobby_id;
            }
        }

        LeaveCriticalSection(&g_lock);
    }

    void join_steam_lobby(int index)
    {
        uint64_t steam_lobby_id = 0;

        EnterCriticalSection(&g_lock);

        if (index >= 0 && index < static_cast<int>(g_entries.size()))
        {
            steam_lobby_id = g_entries[index].steam_lobby_id;
        }

        LeaveCriticalSection(&g_lock);

        if (steam_lobby_id == 0)
        {
            return;
        }

        void* matchmaking = *reinterpret_cast<void**>(engine::base() + engine::steam_matchmaking);

        if (matchmaking == nullptr)
        {
            return;
        }

        void** vtable = *reinterpret_cast<void***>(matchmaking);

        typedef void(__fastcall* join_lobby_t)(void* self, uint64_t steam_id);

        reinterpret_cast<join_lobby_t>(vtable[14])(matchmaking, steam_lobby_id);

        T7_LOG(std::string(cx("serverlist: joining steam lobby ")) + std::to_string(steam_lobby_id) + cx("."));
    }

    void send_steam_chat(int index, const char* message)
    {
        uint64_t steam_lobby_id = 0;

        EnterCriticalSection(&g_lock);

        if (index >= 0 && index < static_cast<int>(g_entries.size()))
        {
            steam_lobby_id = g_entries[index].steam_lobby_id;
        }

        LeaveCriticalSection(&g_lock);

        if (steam_lobby_id == 0 || message == nullptr)
        {
            return;
        }

        void* matchmaking = *reinterpret_cast<void**>(engine::base() + engine::steam_matchmaking);

        if (matchmaking == nullptr)
        {
            return;
        }

        void** vtable = *reinterpret_cast<void***>(matchmaking);

        std::string body = std::string(cx("0 ")) + message;

        typedef bool(__fastcall* send_chat_t)(void* self, uint64_t steam_id, void* data, int size);

        reinterpret_cast<send_chat_t>(vtable[26])(matchmaking, steam_lobby_id, const_cast<char*>(body.data()), static_cast<int>(body.size()) + 1);
    }

    void set_active(bool value)
    {
        g_active = value;
    }

    void clear()
    {
        EnterCriticalSection(&g_lock);

        g_entries.clear();

        LeaveCriticalSection(&g_lock);
    }

    int count()
    {
        return static_cast<int>(g_entries.size());
    }

    int page_count()
    {
        int pages = (static_cast<int>(g_entries.size()) + entries_per_page - 1) / entries_per_page;

        return pages < 1 ? 1 : pages;
    }

    const entry* get(int index)
    {
        if (index < 0 || index >= static_cast<int>(g_entries.size()))
        {
            return nullptr;
        }

        return &g_entries[index];
    }

    void lock()
    {
        EnterCriticalSection(&g_lock);
    }

    void unlock()
    {
        LeaveCriticalSection(&g_lock);
    }

    void initialize()
    {
        if (g_inited)
        {
            return;
        }

        g_inited = true;

        InitializeCriticalSection(&g_lock);

        uint8_t* base = reinterpret_cast<uint8_t*>(engine::base());

        engine::allocate_structs_fn = reinterpret_cast<engine::allocate_structs_t>(base + engine::allocate_structs);

        engine::lobby_search_session_fn = reinterpret_cast<engine::lobby_search_session_t>(base + engine::lobby_search_session);

        engine::demonware_fetching_done_fn = reinterpret_cast<engine::demonware_gate_t>(base + engine::demonware_fetching_done);

        engine::demonware_signed_in_fn = reinterpret_cast<engine::demonware_gate_t>(base + engine::demonware_signed_in);

        engine::lobby_session_get_fn = reinterpret_cast<engine::lobby_session_get_t>(base + engine::lobby_session_getter);

        engine::bd_find_sessions_fn = reinterpret_cast<engine::bd_find_sessions_t>(base + engine::bd_matchmaking_find_sessions);

        engine::send_join_lobby_fn = reinterpret_cast<engine::send_join_lobby_t>(base + engine::send_join_lobby_to_adr);

        engine::content_packs_fn = reinterpret_cast<engine::content_packs_t>(base + engine::content_get_enabled_content_packs);

        engine::net_field_checksum_fn = reinterpret_cast<engine::net_field_checksum_t>(base + engine::msg_crc_net_field_checksum);

        engine::dw_register_sec_fn = reinterpret_cast<engine::dw_register_sec_t>(base + engine::dw_register_sec_id_and_key);

        engine::dw_common_to_netadr_fn = reinterpret_cast<engine::dw_common_to_netadr_t>(base + engine::dw_common_addr_to_netadr);

        engine::send_member_info_fn = reinterpret_cast<engine::send_member_info_t>(base + engine::send_join_member_info_to_adr);

        engine::dw_netadr_to_common_fn = reinterpret_cast<engine::dw_netadr_to_common_t>(base + engine::dw_netadr_to_common_addr);

        utils::hook::attach(reinterpret_cast<void**>(&engine::bd_find_sessions_fn), hk_find_sessions);

        T7_LOG(cx("serverlist: initialized."));
    }
}
