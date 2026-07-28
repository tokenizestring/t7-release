#include "menu.hpp"
#include "../engine/engine.hpp"
#include "../features/overlay/renderer.hpp"
#include "../features/overlay/overlay.hpp"
#include "../features/menulogo/menulogo.hpp"
#include "../features/recents/recents.hpp"
#include "../features/serverlist/serverlist.hpp"
#include "../patches/video/video.hpp"
#include "../patches/matchmaking/matchmaking.hpp"
#include "../features/send/send.hpp"
#include "../utils/hook/hook.hpp"
#include "../utils/log/log.hpp"
#include "../utils/crypt/crypt.hpp"

#include <cstdio>
#include <cstdint>
#include <cmath>

using namespace overlay;

namespace menu
{
    struct item
    {
        const char* label;

        bool* toggle;

        void (*action)();
    };

    static void action_disconnect()
    {
        engine::cl_disconnect();
    }

    static void action_create_session()
    {
        matchmaking::create_session();
    }

    static item general_items[] =
    {
        { "notifications", &engine::notifications_enabled, nullptr },
        { "skip cutscenes", &video::skip_cutscenes, nullptr },
        { "anti lod pop-in", &engine::lod_enabled, nullptr },
        { "faster texture streaming", &engine::texstream_enabled, nullptr },
        { "fps-safe movement", &engine::movement_tick_enabled, nullptr },
        { "no mismatch kick", &engine::clientfield_fix, nullptr },
        { "matchmaking fix", &engine::matchmaking_fix, nullptr },
        { "advertise lobby", &engine::lobby_advertise, nullptr },
        { "create session", nullptr, action_create_session },
        { "auto probe servers", &serverlist::auto_probe, nullptr },
        { "block steam p2p", &engine::block_p2p, nullptr },
        { "block raw udp", &engine::block_netchan, nullptr },
        { "disconnect", nullptr, action_disconnect },
    };

    static item protection_items[] =
    {
        { "server commands", &engine::protection.server_commands, nullptr },
        { "connectionless (oob)", &engine::protection.connectionless, nullptr },
        { "lobby messages", &engine::protection.lobby_messages, nullptr },
        { "demonware", &engine::protection.demonware, nullptr },
        { "netchan guards", &engine::protection.netchan_guards, nullptr },
        { "markup text", &engine::protection.markup_text, nullptr },
        { "side-load", &engine::protection.side_load, nullptr },
        { "vote strings", &engine::protection.vote_strings, nullptr },
        { "player presence", &engine::protection.player_presence, nullptr },
        { "paragon icons", &engine::protection.paragon_icons, nullptr },
        { "notetrack anim", &engine::protection.notetrack, nullptr },
        { "p2p userdata", &engine::protection.p2p_userdata, nullptr },
        { "info leak", &engine::protection.info_leak, nullptr },
        { "host-forced mods", &engine::protection.workshop_mods, nullptr },
    };

    static void action_toxic_host()
    {
        send::target_info ti = {};

        send::send_join_overflow(ti);
    }

    static item toxic_items[] =
    {
        { "join overflow -> host", nullptr, action_toxic_host },
    };

    static const char* about_lines[] =
    {
        "reworked",
        "black ops III protection + quality of life",
        "hook-based, zero dependencies",
        "reworked.cc",
        "press insert to open or close.",
    };

    static constexpr int page_general = 0;

    static constexpr int page_protection = 1;

    static constexpr int page_clients = 2;

    static constexpr int page_recents = 3;

    static constexpr int page_toxic = 4;

    static constexpr int page_about = 5;

    static constexpr int page_servers = 6;

    static constexpr int page_exit = 7;

    static constexpr int root_count = 8;

    static bool g_open = false;

    static int g_view = -1;

    static int g_root_sel = 0;

    static int g_sub_sel = 0;

    static int g_player_view = -1;

    static int g_recents_page = 0;

    static int g_servers_page = 0;

    struct lobby_player
    {
        uint64_t xuid;

        char name[36];

        char clan[8];

        int rank;

        int prestige;

        int paragon;

        bool host;

        bool leader;

        bool local;

        bool talking;

        bool connected;

        bool has_ip;

        unsigned char ip[4];

        uint16_t port;

        uint32_t nat;

        bool has_netadr;

        engine::netadr_s netadr;
    };

    static lobby_player g_players[18];

    static int gather_players()
    {
        uint8_t* base = reinterpret_cast<uint8_t*>(engine::base());

        int mode = reinterpret_cast<engine::mode_index_t>(base + engine::current_mode_index)();

        if (mode < 0 || mode > 2)
        {
            mode = 0;
        }

        int best = -1;

        int best_count = 0;

        for (int s = 0; s < 2; s++)
        {
            uint8_t* session = base + engine::lobby_session + engine::lobby_session_stride * s;

            int occupied = 0;

            for (int i = 0; i < 18; i++)
            {
                if (*reinterpret_cast<uint64_t*>(session + engine::lobby_member_array + engine::lobby_member_stride * i) != 0)
                {
                    occupied++;
                }
            }

            if (occupied > best_count)
            {
                best_count = occupied;

                best = s;
            }
        }

        if (best < 0)
        {
            return 0;
        }

        uint8_t* session = base + engine::lobby_session + engine::lobby_session_stride * best;

        uint64_t host_xuid = *reinterpret_cast<uint64_t*>(session + engine::lobby_session_host_xuid);

        uint64_t leader_xuid = *reinterpret_cast<uint64_t*>(session + engine::lobby_session_leader_xuid);

        int found = 0;

        for (int i = 0; i < 18; i++)
        {
            uint8_t* member = session + engine::lobby_member_array + engine::lobby_member_stride * i;

            if (*reinterpret_cast<uint64_t*>(member) == 0)
            {
                continue;
            }

            uint8_t* client = *reinterpret_cast<uint8_t**>(member + engine::lobby_member_client);

            if (reinterpret_cast<uintptr_t>(client) < 0x10000)
            {
                continue;
            }

            lobby_player& p = g_players[found];

            p.xuid = *reinterpret_cast<uint64_t*>(client + engine::lobby_client_xuid);

            const char* src = reinterpret_cast<const char*>(client + engine::lobby_client_gamertag);

            int k = 0;

            while (k < 32 && src[k] != 0)
            {
                p.name[k] = src[k];

                k++;
            }

            p.name[k] = 0;

            const char* tag = reinterpret_cast<const char*>(client + engine::lobby_client_clantag);

            int t = 0;

            while (t < 4 && tag[t] != 0)
            {
                p.clan[t] = tag[t];

                t++;
            }

            p.clan[t] = 0;

            uint8_t* stats = client + engine::lobby_client_rank + engine::lobby_client_mode_stride * mode;

            p.rank = stats[0];

            p.prestige = stats[1];

            p.paragon = *reinterpret_cast<uint16_t*>(stats + 2);

            p.host = p.xuid == host_xuid;

            p.leader = p.xuid == leader_xuid;

            p.local = reinterpret_cast<engine::is_local_t>(base + engine::is_local_player)(p.xuid) != 0;

            p.talking = reinterpret_cast<engine::voip_status_t>(base + engine::voip_status)(client, 0) != 0;

            uint8_t* conn = *reinterpret_cast<uint8_t**>(client + engine::lobby_client_conn);

            if (reinterpret_cast<uintptr_t>(conn) >= 0x10000)
            {
                p.ip[0] = conn[engine::conn_ip];

                p.ip[1] = conn[engine::conn_ip + 1];

                p.ip[2] = conn[engine::conn_ip + 2];

                p.ip[3] = conn[engine::conn_ip + 3];

                p.port = *reinterpret_cast<uint16_t*>(conn + engine::conn_port);

                p.nat = *reinterpret_cast<uint32_t*>(conn + engine::conn_nat);

                p.has_ip = true;

                p.connected = true;
            }
            else
            {
                p.nat = 0;

                p.has_ip = false;

                p.connected = false;
            }

            p.has_netadr = false;

            for (int slot = 0; slot < 2; slot++)
            {
                uint8_t* entry = client + 32 * (slot + 39);

                if (*entry)
                {
                    memcpy(&p.netadr, entry + 4, sizeof(engine::netadr_s));

                    p.has_netadr = true;

                    break;
                }
            }

            found++;
        }

        return found;
    }

    static item* page_items(int page, int& count)
    {
        if (page == page_general)
        {
            count = sizeof(general_items) / sizeof(general_items[0]);

            return general_items;
        }

        if (page == page_protection)
        {
            count = sizeof(protection_items) / sizeof(protection_items[0]);

            return protection_items;
        }

        if (page == page_toxic)
        {
            count = sizeof(toxic_items) / sizeof(toxic_items[0]);

            return toxic_items;
        }

        count = 0;

        return nullptr;
    }

    static const char* root_label(int i)
    {
        if (i == page_general)
        {
            return "general";
        }

        if (i == page_protection)
        {
            return "protection";
        }

        if (i == page_toxic)
        {
            return "toxic";
        }

        if (i == page_about)
        {
            return "about";
        }

        if (i == page_servers)
        {
            return "servers";
        }

        if (i == page_exit)
        {
            return "exit";
        }

        return "";
    }

    static void format_addr(const recents::address_record& a, char* buf, int buf_size)
    {
        snprintf(buf, buf_size, "%d.%d.%d.%d:%d", a.ip[0], a.ip[1], a.ip[2], a.ip[3], a.port);
    }

    static void activate()
    {
        if (g_view == -1)
        {
            if (g_root_sel == page_exit)
            {
                g_open = false;
            }
            else
            {
                g_view = g_root_sel;

                g_sub_sel = 0;

                if (g_view == page_recents)
                {
                    g_recents_page = 0;
                }

                if (g_view == page_servers)
                {
                    g_servers_page = 0;
                }
            }

            return;
        }

        int count = 0;

        item* items = page_items(g_view, count);

        if (items != nullptr && g_sub_sel < count)
        {
            item& s = items[g_sub_sel];

            if (s.toggle != nullptr)
            {
                *s.toggle = !*s.toggle;
            }
            else if (s.action != nullptr)
            {
                s.action();
            }
        }
    }

    static bool local_is_host()
    {
        void* lobby = engine::active_lobby();

        if (lobby == nullptr)
        {
            return false;
        }

        uint64_t host = engine::lobby_host_xuid(lobby);

        if (host == 0)
        {
            return false;
        }

        return host == static_cast<uint64_t>(engine::user_xuid(0));
    }

    static void fire_player_action(int player_idx, int action_idx)
    {
        if (player_idx < 0 || player_idx >= 18)
        {
            features::overlay::notify(cx("invalid player index."), features::overlay::level::warn);

            return;
        }

        lobby_player& p = g_players[player_idx];

        if (!p.has_netadr)
        {
            features::overlay::notify(cx("no address for target."), features::overlay::level::warn);

            T7_LOG(cx("send: no netadr for ") + std::string(p.name));

            return;
        }

        send::target_info ti = {};

        ti.ip[0] = p.ip[0];

        ti.ip[1] = p.ip[1];

        ti.ip[2] = p.ip[2];

        ti.ip[3] = p.ip[3];

        ti.port = p.port;

        ti.xuid = p.xuid;

        ti.netadr = p.netadr;

        int c = 0;

        while (c < 35 && p.name[c] != 0)
        {
            ti.name[c] = p.name[c];

            c++;
        }

        ti.name[c] = 0;

        if (action_idx == 0)
        {
            send::send_relay_test(ti);

            if (local_is_host())
            {
                send::send_host_disconnect(ti);
            }

            send::send_migrate_test(ti);

            send::send_print_nested(ti);

            send::send_oob_at(ti);
        }
    }

    typedef uint32_t(__stdcall* xinput_get_state_t)(uint32_t index, void* state);

    static xinput_get_state_t g_xinput = nullptr;

    static bool g_xinput_tried = false;

    struct pad
    {
        bool up;

        bool down;

        bool left;

        bool right;

        bool accept;

        bool cancel;
    };

    static void read_pad(pad& out)
    {
        out = {};

        if (!g_xinput_tried)
        {
            g_xinput_tried = true;

            HMODULE lib = LoadLibraryA("xinput1_4.dll");

            if (lib == nullptr)
            {
                lib = LoadLibraryA("xinput1_3.dll");
            }

            if (lib == nullptr)
            {
                lib = LoadLibraryA("xinput9_1_0.dll");
            }

            if (lib != nullptr)
            {
                g_xinput = reinterpret_cast<xinput_get_state_t>(GetProcAddress(lib, "XInputGetState"));
            }
        }

        if (g_xinput == nullptr)
        {
            return;
        }

        struct { uint32_t packet; uint16_t buttons; uint8_t lt; uint8_t rt; int16_t lx; int16_t ly; int16_t rx; int16_t ry; } state = {};

        for (uint32_t i = 0; i < 4; i++)
        {
            if (g_xinput(i, &state) == 0)
            {
                out.up = (state.buttons & 0x0001) != 0 || state.ly > 16000;

                out.down = (state.buttons & 0x0002) != 0 || state.ly < -16000;

                out.left = (state.buttons & 0x0004) != 0 || state.lx < -16000;

                out.right = (state.buttons & 0x0008) != 0 || state.lx > 16000;

                out.accept = (state.buttons & 0x1000) != 0;

                out.cancel = (state.buttons & 0x2000) != 0;

                break;
            }
        }
    }

    static bool pad_edge(bool down, bool& latch)
    {
        if (down && !latch)
        {
            latch = true;

            return true;
        }

        if (!down)
        {
            latch = false;
        }

        return false;
    }

    static bool edge(int key, bool& latch)
    {
        bool down = (GetAsyncKeyState(key) & 0x8000) != 0;

        if (down && !latch)
        {
            latch = true;

            return true;
        }

        if (!down)
        {
            latch = false;
        }

        return false;
    }

    static void handle_input()
    {
        static bool latch_open = false;

        static bool latch_up = false;

        static bool latch_down = false;

        static bool latch_enter = false;

        static bool latch_back = false;

        static bool latch_left = false;

        static bool latch_right = false;

        static bool latch_join = false;

        if (edge(VK_INSERT, latch_open))
        {
            g_open = !g_open;
        }

        if (!g_open)
        {
            return;
        }

        static bool pad_latch_up = false;

        static bool pad_latch_down = false;

        static bool pad_latch_left = false;

        static bool pad_latch_right = false;

        static bool pad_latch_accept = false;

        static bool pad_latch_cancel = false;

        pad gp;

        read_pad(gp);

        bool up = edge(VK_UP, latch_up) || pad_edge(gp.up, pad_latch_up);

        bool down = edge(VK_DOWN, latch_down) || pad_edge(gp.down, pad_latch_down);

        bool enter = edge(VK_RETURN, latch_enter) || pad_edge(gp.accept, pad_latch_accept);

        bool back = edge(VK_BACK, latch_back) || pad_edge(gp.cancel, pad_latch_cancel);

        bool left = edge(VK_LEFT, latch_left) || pad_edge(gp.left, pad_latch_left);

        bool right = edge(VK_RIGHT, latch_right) || pad_edge(gp.right, pad_latch_right);

        if (g_view == page_recents && right)
        {
            if (g_recents_page < recents::page_count() - 1)
            {
                g_recents_page++;

                g_sub_sel = 0;
            }
        }

        if (g_view == page_recents && left)
        {
            if (g_recents_page > 0)
            {
                g_recents_page--;

                g_sub_sel = 0;

                left = false;
            }
        }

        if (g_view == page_servers && right)
        {
            if (g_servers_page < serverlist::page_count() - 1)
            {
                g_servers_page++;

                g_sub_sel = 0;
            }
        }

        if (g_view == page_servers && left)
        {
            if (g_servers_page > 0)
            {
                g_servers_page--;

                g_sub_sel = 0;

                left = false;
            }
        }

        if (g_view == page_servers && edge('J', latch_join))
        {
            serverlist::join_steam_lobby(g_servers_page * serverlist::entries_per_page + g_sub_sel);
        }

        if (g_view == -1)
        {
            if (left)
            {
                g_open = false;

                return;
            }

            if (up)
            {
                g_root_sel = (g_root_sel - 1 + root_count) % root_count;
            }

            if (down)
            {
                g_root_sel = (g_root_sel + 1) % root_count;
            }

            if (enter)
            {
                activate();
            }

            return;
        }

        if (back || left)
        {
            if (g_view == page_clients && g_player_view >= 0)
            {
                g_player_view = -1;

                g_sub_sel = 0;

                return;
            }

            g_view = -1;

            g_sub_sel = 0;

            g_player_view = -1;

            g_recents_page = 0;

            return;
        }

        int count = 0;

        page_items(g_view, count);

        if (g_view == page_clients)
        {
            count = g_player_view >= 0 ? 1 : gather_players();
        }

        if (g_view == page_recents)
        {
            int rc = 0;

            int start = g_recents_page * recents::entries_per_page;

            int total = recents::count();

            rc = total - start;

            if (rc > recents::entries_per_page)
            {
                rc = recents::entries_per_page;
            }

            if (rc < 0)
            {
                rc = 0;
            }

            count = rc;
        }

        if (g_view == page_servers)
        {
            if (g_servers_page >= serverlist::page_count())
            {
                g_servers_page = serverlist::page_count() - 1;
            }

            int start = g_servers_page * serverlist::entries_per_page;

            int page_n = serverlist::count() - start;

            if (page_n > serverlist::entries_per_page)
            {
                page_n = serverlist::entries_per_page;
            }

            if (page_n < 0)
            {
                page_n = 0;
            }

            count = page_n;
        }

        if (count <= 0)
        {
            return;
        }

        if (g_sub_sel >= count)
        {
            g_sub_sel = count - 1;
        }

        if (up)
        {
            g_sub_sel = (g_sub_sel - 1 + count) % count;
        }

        if (down)
        {
            g_sub_sel = (g_sub_sel + 1) % count;
        }

        if (enter)
        {
            if (g_view == page_clients && g_player_view == -1)
            {
                g_player_view = g_sub_sel;

                g_sub_sel = 0;
            }
            else if (g_view == page_clients && g_player_view >= 0)
            {
                fire_player_action(g_player_view, g_sub_sel);
            }
            else if (g_view == page_servers)
            {
                serverlist::dispatch_probe(g_servers_page * serverlist::entries_per_page + g_sub_sel);
            }
            else
            {
                activate();
            }
        }
    }

    static void __fastcall hk_key_event(uint32_t local_client, uint32_t key)
    {
        if (g_open)
        {
            return;
        }

        engine::key_event_fn(local_client, key);
    }

    static int64_t __fastcall hk_mouse_move(uint32_t local_client, uint32_t a2, uint32_t a3, int dx, int dy)
    {
        if (g_open)
        {
            return 0;
        }

        return engine::mouse_move_fn(local_client, a2, a3, dx, dy);
    }

    static void block_game_input(bool open)
    {
        static bool was_open = false;

        uint8_t* base = reinterpret_cast<uint8_t*>(engine::base());

        if (open && !was_open)
        {
            reinterpret_cast<engine::clear_input_t>(base + engine::clear_held_input)(0);
        }

        if (open)
        {
            *reinterpret_cast<int*>(base + engine::cursor_visible_flag) = 1;
        }

        was_open = open;
    }

    static void mouse_pos(float screen_w, float screen_h, float& mx, float& my)
    {
        POINT pt = {};

        GetCursorPos(&pt);

        HWND hwnd = GetForegroundWindow();

        if (hwnd != nullptr)
        {
            ScreenToClient(hwnd, &pt);
        }

        mx = static_cast<float>(pt.x);

        my = static_cast<float>(pt.y);

        RECT rc = {};

        if (hwnd != nullptr && GetClientRect(hwnd, &rc) && rc.right > 0 && rc.bottom > 0)
        {
            mx *= screen_w / static_cast<float>(rc.right);

            my *= screen_h / static_cast<float>(rc.bottom);
        }
    }

    void initialize()
    {
        engine::key_event_fn = reinterpret_cast<engine::key_event_t>(engine::base() + engine::key_event);

        engine::mouse_move_fn = reinterpret_cast<engine::mouse_move_t>(engine::base() + engine::mouse_move);

        utils::hook::attach(reinterpret_cast<void**>(&engine::key_event_fn), hk_key_event);

        utils::hook::attach(reinterpret_cast<void**>(&engine::mouse_move_fn), hk_mouse_move);

        send::initialize();
    }

    void tick(float screen_w, float screen_h)
    {
        static bool hooks_ready = false;

        if (!hooks_ready)
        {
            initialize();

            hooks_ready = true;
        }

        handle_input();

        block_game_input(g_open);

        serverlist::set_active(g_open && g_view == page_servers);

        serverlist::tick();

        if (!g_open)
        {
            return;
        }

        uint32_t back = renderer::rgba(11, 11, 15, 240);

        uint32_t bar = renderer::rgba(154, 122, 250, 255);

        uint32_t sep = renderer::rgba(30, 28, 40, 255);

        uint32_t highlight = renderer::rgba(154, 122, 250, 60);

        uint32_t text = renderer::rgba(238, 238, 244, 255);

        uint32_t dim = renderer::rgba(140, 140, 154, 255);

        uint32_t shadow = renderer::rgba(0, 0, 0, 200);

        uint32_t green = renderer::rgba(94, 201, 139, 255);

        uint32_t red = renderer::rgba(210, 90, 90, 255);

        float line = renderer::line_height();

        float pad = 18.0f;

        float row_h = line + 12.0f;

        float text_dy = (row_h - line) * 0.5f;

        float logo_w = 190.0f;

        float logo_h = 66.0f;

        if (features::menulogo::current_srv() != nullptr && features::menulogo::height() != 0)
        {
            float aspect = static_cast<float>(features::menulogo::width()) / static_cast<float>(features::menulogo::height());

            logo_h = logo_w / aspect;
        }

        float header_h = logo_h + pad * 1.4f;

        int n = gather_players();

        int body_rows = root_count;

        int rc = recents::count();

        if (g_view == page_about)
        {
            body_rows = sizeof(about_lines) / sizeof(about_lines[0]);
        }
        else if (g_view == page_clients)
        {
            body_rows = g_player_view >= 0 ? 2 : 1 + (n > 0 ? n : 1);
        }
        else if (g_view == page_recents)
        {
            int page_start = g_recents_page * recents::entries_per_page;

            int page_n = rc - page_start;

            if (page_n > recents::entries_per_page)
            {
                page_n = recents::entries_per_page;
            }

            if (page_n < 0)
            {
                page_n = 0;
            }

            body_rows = 1 + (page_n > 0 ? page_n : 1);
        }
        else if (g_view == page_servers)
        {
            int start = g_servers_page * serverlist::entries_per_page;

            int page_n = serverlist::count() - start;

            if (page_n > serverlist::entries_per_page)
            {
                page_n = serverlist::entries_per_page;
            }

            if (page_n < 0)
            {
                page_n = 0;
            }

            body_rows = 1 + (page_n > 0 ? page_n : 1);
        }
        else if (g_view >= 0)
        {
            int c = 0;

            page_items(g_view, c);

            body_rows = c;
        }

        float footer_h = 8.0f;

        float panel_w = 360.0f;

        float panel_h = header_h + body_rows * row_h + footer_h + pad;

        float x = 60.0f;

        float y = (screen_h - panel_h) * 0.5f;

        float body_y = y + header_h + 4.0f;

        float lx = x + pad + 4.0f;

        float mx = 0.0f;

        float my = 0.0f;

        mouse_pos(screen_w, screen_h, mx, my);

        static float last_mx = -1.0f;

        static float last_my = -1.0f;

        bool moved = std::fabs(mx - last_mx) > 1.0f || std::fabs(my - last_my) > 1.0f;

        last_mx = mx;

        last_my = my;

        static bool lb_latch = false;

        bool lb_down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        bool click = lb_down && !lb_latch;

        lb_latch = lb_down;

        int hover_rows = 0;

        float hover_base = body_y;

        if (g_view == -1)
        {
            hover_rows = root_count;
        }
        else if (g_view == page_clients)
        {
            if (g_player_view >= 0)
            {
                hover_rows = 2;
            }
            else
            {
                hover_rows = n > 12 ? 12 : n;

                hover_base = body_y + row_h;
            }
        }
        else if (g_view == page_recents)
        {
            int page_start = g_recents_page * recents::entries_per_page;

            int page_n = rc - page_start;

            if (page_n > recents::entries_per_page)
            {
                page_n = recents::entries_per_page;
            }

            if (page_n < 0)
            {
                page_n = 0;
            }

            hover_rows = page_n;

            hover_base = body_y + row_h;
        }
        else if (g_view == page_servers)
        {
            int start = g_servers_page * serverlist::entries_per_page;

            int page_n = serverlist::count() - start;

            if (page_n > serverlist::entries_per_page)
            {
                page_n = serverlist::entries_per_page;
            }

            if (page_n < 0)
            {
                page_n = 0;
            }

            hover_rows = page_n;

            hover_base = body_y + row_h;
        }
        else if (g_view >= 0 && g_view != page_about)
        {
            page_items(g_view, hover_rows);
        }

        int hover = -1;

        if (mx >= x && mx <= x + panel_w)
        {
            for (int i = 0; i < hover_rows; i++)
            {
                float row_y = hover_base + i * row_h;

                if (my >= row_y && my < row_y + row_h)
                {
                    hover = i;

                    break;
                }
            }
        }

        if (hover >= 0)
        {
            if (moved)
            {
                if (g_view == -1)
                {
                    g_root_sel = hover;
                }
                else
                {
                    g_sub_sel = hover;
                }
            }

            if (click)
            {
                if (g_view == -1)
                {
                    g_root_sel = hover;
                }
                else
                {
                    g_sub_sel = hover;
                }

                if (g_view == page_clients && g_player_view == -1)
                {
                    g_player_view = hover;

                    g_sub_sel = 0;
                }
                else if (g_view == page_clients && g_player_view >= 0)
                {
                    fire_player_action(g_player_view, g_sub_sel);
                }
                else if (g_view == page_recents)
                {
                }
                else if (g_view == page_servers)
                {
                    serverlist::dispatch_probe(g_servers_page * serverlist::entries_per_page + g_sub_sel);
                }
                else if (g_view != page_clients)
                {
                    activate();
                }
            }
        }

        renderer::draw_rect(x, y, panel_w, panel_h, back);

        renderer::draw_rect(x, y, 4.0f, panel_h, bar);

        ID3D11ShaderResourceView* logo = features::menulogo::current_srv();

        if (logo != nullptr)
        {
            renderer::draw_image(x + pad, y + pad * 0.7f, logo_w, logo_h, logo);
        }
        else
        {
            renderer::draw_text_shadow(x + pad, y + pad, "reworked", text, shadow);
        }

        renderer::draw_rect(x + pad, y + header_h - 1.0f, panel_w - pad * 2.0f, 1.0f, sep);

        if (g_view != -1)
        {
            float back_w = renderer::text_width("< back");

            float back_x = x + panel_w - pad - back_w;

            float back_y = y + pad * 0.9f;

            bool back_hover = mx >= back_x - 6.0f && mx <= back_x + back_w + 6.0f && my >= back_y - 4.0f && my <= back_y + line + 4.0f;

            renderer::draw_text(back_x, back_y, "< back", back_hover ? text : dim);

            if (back_hover && click)
            {
                if (g_view == page_clients && g_player_view >= 0)
                {
                    g_player_view = -1;

                    g_sub_sel = 0;
                }
                else
                {
                    g_view = -1;

                    g_sub_sel = 0;

                    g_player_view = -1;

                    g_recents_page = 0;
                }
            }
        }

        if (g_view == -1)
        {
            for (int i = 0; i < root_count; i++)
            {
                float row_y = body_y + i * row_h;

                bool sel = i == g_root_sel;

                if (sel)
                {
                    renderer::draw_rect(x + 2.0f, row_y, panel_w - 4.0f, row_h, highlight);
                }

                if (i == page_clients)
                {
                    char num[8];

                    sprintf_s(num, sizeof(num), "%d", n);

                    renderer::draw_text(lx, row_y + text_dy, num, green);

                    renderer::draw_text(lx + renderer::text_width(num) + 5.0f, row_y + text_dy, "clients", sel ? text : dim);
                }
                else if (i == page_recents)
                {
                    char num[8];

                    sprintf_s(num, sizeof(num), "%d", rc);

                    renderer::draw_text(lx, row_y + text_dy, num, green);

                    renderer::draw_text(lx + renderer::text_width(num) + 5.0f, row_y + text_dy, "recents", sel ? text : dim);
                }
                else if (i == page_servers)
                {
                    char num[8];

                    sprintf_s(num, sizeof(num), "%d", serverlist::count());

                    renderer::draw_text(lx, row_y + text_dy, num, green);

                    renderer::draw_text(lx + renderer::text_width(num) + 5.0f, row_y + text_dy, "servers", sel ? text : dim);
                }
                else
                {
                    renderer::draw_text(lx, row_y + text_dy, root_label(i), sel ? text : dim);
                }

                if (i != page_exit)
                {
                    renderer::draw_text(x + panel_w - pad - renderer::text_width(">"), row_y + text_dy, ">", bar);
                }
            }
        }
        else if (g_view == page_about)
        {
            int lines = sizeof(about_lines) / sizeof(about_lines[0]);

            for (int i = 0; i < lines; i++)
            {
                renderer::draw_text(lx, body_y + i * row_h + text_dy, about_lines[i], i == 0 ? text : dim);
            }
        }
        else if (g_view == page_clients)
        {
            if (g_player_view >= 0)
            {
                const char* actions[] = { "crash" };

                for (int i = 0; i < 1; i++)
                {
                    float row_y = body_y + i * row_h;

                    bool sel = g_sub_sel == i;

                    if (sel)
                    {
                        renderer::draw_rect(x + 2.0f, row_y, panel_w - 4.0f, row_h, highlight);
                    }

                    renderer::draw_text(lx, row_y + text_dy, actions[i], sel ? text : dim);

                    renderer::draw_text(x + panel_w - pad - renderer::text_width(">"), row_y + text_dy, ">", bar);
                }
            }
            else
            {
                char num[8];

                sprintf_s(num, sizeof(num), "%d", n);

                renderer::draw_text(lx, body_y + text_dy, num, green);

                renderer::draw_text(lx + renderer::text_width(num) + 5.0f, body_y + text_dy, "clients connected", dim);

                if (n == 0)
                {
                    renderer::draw_text(lx, body_y + row_h + text_dy, "no one connected", dim);
                }

                int shown = n > 12 ? 12 : n;

                for (int i = 0; i < shown; i++)
                {
                    float row_y = body_y + (i + 1) * row_h;

                    bool sel = g_sub_sel == i;

                    if (sel)
                    {
                        renderer::draw_rect(x + 2.0f, row_y, panel_w - 4.0f, row_h, highlight);
                    }

                    lobby_player& lp = g_players[i];

                    if (lp.talking)
                    {
                        renderer::draw_rect(lx + 2.0f, row_y + row_h * 0.5f - 3.0f, 6.0f, 6.0f, green);
                    }

                    renderer::draw_text(lx + 16.0f, row_y + text_dy, lp.name, sel ? text : dim);

                    const char* tag = nullptr;

                    if (lp.local && lp.host)
                    {
                        tag = "you, host";
                    }
                    else if (lp.local)
                    {
                        tag = "you";
                    }
                    else if (lp.host)
                    {
                        tag = "host";
                    }

                    if (tag != nullptr)
                    {
                        renderer::draw_text(x + panel_w - pad - renderer::text_width(tag), row_y + text_dy, tag, bar);
                    }
                }
            }

            int detail_idx = g_player_view >= 0 ? g_player_view : g_sub_sel;

            if (n > 0 && detail_idx < n)
            {
                lobby_player& p = g_players[detail_idx];

                const char* c_badge = p.host ? "host" : (p.leader ? "leader" : "");

                char c_level[16];

                sprintf_s(c_level, sizeof(c_level), "%d", p.rank + 1);

                char c_prest[16];

                sprintf_s(c_prest, sizeof(c_prest), "%d", p.prestige);

                char c_para[16];

                sprintf_s(c_para, sizeof(c_para), p.paragon > 0 ? "%d" : "-", p.paragon);

                const char* c_clan = p.clan[0] != 0 ? p.clan : "-";

                const char* c_status = p.connected ? "connected" : "connecting";

                const char* c_nat = p.nat == 1 ? "open" : (p.nat == 2 ? "moderate" : (p.nat == 3 ? "strict" : "unknown"));

                char c_ip[32];

                sprintf_s(c_ip, sizeof(c_ip), p.has_ip ? "%d.%d.%d.%d" : "-", p.ip[0], p.ip[1], p.ip[2], p.ip[3]);

                char c_xuid[24];

                sprintf_s(c_xuid, sizeof(c_xuid), "%llu", static_cast<unsigned long long>(p.xuid));

                float c_gap = 30.0f;

                float c_content = renderer::text_width(p.name) + (c_badge[0] != 0 ? c_gap + renderer::text_width(c_badge) : 0.0f);

                c_content = fmaxf(c_content, renderer::text_width("level") + c_gap + renderer::text_width(c_level));

                c_content = fmaxf(c_content, renderer::text_width("prestige") + c_gap + renderer::text_width(c_prest));

                c_content = fmaxf(c_content, renderer::text_width("paragon") + c_gap + renderer::text_width(c_para));

                c_content = fmaxf(c_content, renderer::text_width("clan") + c_gap + renderer::text_width(c_clan));

                c_content = fmaxf(c_content, renderer::text_width("status") + c_gap + renderer::text_width(c_status));

                c_content = fmaxf(c_content, renderer::text_width("nat type") + c_gap + renderer::text_width(c_nat));

                c_content = fmaxf(c_content, renderer::text_width("address") + c_gap + renderer::text_width(c_ip));

                c_content = fmaxf(c_content, renderer::text_width(c_xuid));

                float bw = c_content + pad * 2.0f;

                float bx = x + panel_w + 12.0f;

                float bh = header_h + 10.0f * row_h + pad;

                renderer::draw_rect(bx, y, bw, bh, back);

                renderer::draw_rect(bx, y, 4.0f, bh, bar);

                renderer::draw_text_shadow(bx + pad, y + pad * 0.9f, p.name, text, shadow);

                if (p.host)
                {
                    renderer::draw_text(bx + bw - pad - renderer::text_width("host"), y + pad * 0.9f, "host", green);
                }
                else if (p.leader)
                {
                    renderer::draw_text(bx + bw - pad - renderer::text_width("leader"), y + pad * 0.9f, "leader", green);
                }

                renderer::draw_rect(bx + pad, y + header_h - 1.0f, bw - pad * 2.0f, 1.0f, sep);

                float dy = y + header_h + 6.0f;

                char buf[32];

                renderer::draw_text(bx + pad, dy, "level", dim);

                sprintf_s(buf, sizeof(buf), "%d", p.rank + 1);

                renderer::draw_text(bx + bw - pad - renderer::text_width(buf), dy, buf, green);

                dy += row_h;

                renderer::draw_text(bx + pad, dy, "prestige", dim);

                sprintf_s(buf, sizeof(buf), "%d", p.prestige);

                renderer::draw_text(bx + bw - pad - renderer::text_width(buf), dy, buf, text);

                dy += row_h;

                renderer::draw_text(bx + pad, dy, "paragon", dim);

                if (p.paragon > 0)
                {
                    sprintf_s(buf, sizeof(buf), "%d", p.paragon);

                    renderer::draw_text(bx + bw - pad - renderer::text_width(buf), dy, buf, text);
                }
                else
                {
                    renderer::draw_text(bx + bw - pad - renderer::text_width("-"), dy, "-", dim);
                }

                dy += row_h;

                renderer::draw_text(bx + pad, dy, "clan", dim);

                renderer::draw_text(bx + bw - pad - renderer::text_width(p.clan[0] != 0 ? p.clan : "-"), dy, p.clan[0] != 0 ? p.clan : "-", text);

                dy += row_h * 1.3f;

                renderer::draw_text(bx + pad, dy, "status", dim);

                const char* status = p.connected ? "connected" : "connecting";

                uint32_t status_col = p.connected ? green : renderer::rgba(230, 180, 60, 255);

                renderer::draw_text(bx + bw - pad - renderer::text_width(status), dy, status, status_col);

                dy += row_h;

                renderer::draw_text(bx + pad, dy, "nat type", dim);

                const char* nat = "unknown";

                if (p.nat == 1)
                {
                    nat = "open";
                }
                else if (p.nat == 2)
                {
                    nat = "moderate";
                }
                else if (p.nat == 3)
                {
                    nat = "strict";
                }

                renderer::draw_text(bx + bw - pad - renderer::text_width(nat), dy, nat, text);

                dy += row_h;

                renderer::draw_text(bx + pad, dy, "address", dim);

                if (p.has_ip)
                {
                    char ip_text[32];

                    sprintf_s(ip_text, sizeof(ip_text), "%d.%d.%d.%d", p.ip[0], p.ip[1], p.ip[2], p.ip[3]);

                    float ip_w = renderer::text_width(ip_text);

                    float ip_x = bx + bw - pad - ip_w;

                    bool ip_hover = mx >= ip_x - 3.0f && mx <= ip_x + ip_w + 3.0f && my >= dy - 3.0f && my <= dy + line + 3.0f;

                    if (ip_hover)
                    {
                        renderer::draw_text(ip_x, dy, ip_text, green);
                    }
                    else
                    {
                        renderer::draw_rect(ip_x, dy + 1.0f, ip_w, line - 2.0f, renderer::rgba(46, 44, 60, 255));
                    }
                }
                else
                {
                    renderer::draw_text(bx + bw - pad - renderer::text_width("-"), dy, "-", dim);
                }

                dy += row_h * 1.4f;

                renderer::draw_text(bx + pad, dy, "xuid", dim);

                char xuid_text[24];

                sprintf_s(xuid_text, sizeof(xuid_text), "%llu", static_cast<unsigned long long>(p.xuid));

                renderer::draw_text(bx + pad, dy + row_h, xuid_text, green);
            }
        }
        else if (g_view == page_recents)
        {
            int total_pages = recents::page_count();

            int page_start = g_recents_page * recents::entries_per_page;

            int page_n = rc - page_start;

            if (page_n > recents::entries_per_page)
            {
                page_n = recents::entries_per_page;
            }

            if (page_n < 0)
            {
                page_n = 0;
            }

            char header[48];

            sprintf_s(header, sizeof(header), "page %d/%d", g_recents_page + 1, total_pages);

            char rcount[8];

            sprintf_s(rcount, sizeof(rcount), "%d", rc);

            renderer::draw_text(lx, body_y + text_dy, rcount, green);

            renderer::draw_text(lx + renderer::text_width(rcount) + 5.0f, body_y + text_dy, "recents", dim);

            renderer::draw_text(x + panel_w - pad - renderer::text_width(header), body_y + text_dy, header, dim);

            if (page_n == 0)
            {
                renderer::draw_text(lx, body_y + row_h + text_dy, "no players found", dim);
            }

            for (int i = 0; i < page_n; i++)
            {
                float row_y = body_y + (i + 1) * row_h;

                bool sel = g_sub_sel == i;

                if (sel)
                {
                    renderer::draw_rect(x + 2.0f, row_y, panel_w - 4.0f, row_h, highlight);
                }

                const recents::entry* re = recents::get(page_start + i);

                if (re == nullptr)
                {
                    continue;
                }

                renderer::draw_text(lx, row_y + text_dy, re->name, sel ? text : dim);

                char ago[24];

                recents::format_ago(re->last_seen, ago, sizeof(ago));

                renderer::draw_text(x + panel_w - pad - renderer::text_width(ago), row_y + text_dy, ago, dim);
            }

            if (page_n > 0 && g_sub_sel < page_n)
            {
                const recents::entry* re = recents::get(page_start + g_sub_sel);

                if (re != nullptr)
                {
                    char r_addr[32];

                    sprintf_s(r_addr, sizeof(r_addr), re->has_addr ? "%d.%d.%d.%d" : "-", re->current.ip[0], re->current.ip[1], re->current.ip[2], re->current.ip[3]);

                    char r_ago[24];

                    recents::format_ago(re->last_seen, r_ago, sizeof(r_ago));

                    char r_xuid[24];

                    sprintf_s(r_xuid, sizeof(r_xuid), "%llu", static_cast<unsigned long long>(re->xuid));

                    float r_gap = 30.0f;

                    float r_content = renderer::text_width(re->name);

                    r_content = fmaxf(r_content, renderer::text_width("address") + r_gap + renderer::text_width(r_addr));

                    r_content = fmaxf(r_content, renderer::text_width("last seen") + r_gap + renderer::text_width(r_ago));

                    r_content = fmaxf(r_content, renderer::text_width(r_xuid));

                    r_content = fmaxf(r_content, renderer::text_width("previous addresses"));

                    for (int a = 0; a < re->addr_count; a++)
                    {
                        char r_hist[32];

                        format_addr(re->history[a], r_hist, sizeof(r_hist));

                        r_content = fmaxf(r_content, 8.0f + renderer::text_width(r_hist));
                    }

                    float bw = r_content + pad * 2.0f;

                    float bx = x + panel_w + 12.0f;

                    int detail_rows = 4 + (re->has_addr ? 1 : 0) + re->addr_count;

                    float bh = header_h + detail_rows * row_h + pad;

                    renderer::draw_rect(bx, y, bw, bh, back);

                    renderer::draw_rect(bx, y, 4.0f, bh, bar);

                    renderer::draw_text_shadow(bx + pad, y + pad * 0.9f, re->name, text, shadow);

                    renderer::draw_rect(bx + pad, y + header_h - 1.0f, bw - pad * 2.0f, 1.0f, sep);

                    float dy = y + header_h + 6.0f;

                    renderer::draw_text(bx + pad, dy, "address", dim);

                    if (re->has_addr)
                    {
                        char addr_buf[32];

                        sprintf_s(addr_buf, sizeof(addr_buf), "%d.%d.%d.%d", re->current.ip[0], re->current.ip[1], re->current.ip[2], re->current.ip[3]);

                        float ip_w = renderer::text_width(addr_buf);

                        float ip_x = bx + bw - pad - ip_w;

                        bool ip_hover = mx >= ip_x - 3.0f && mx <= ip_x + ip_w + 3.0f && my >= dy - 3.0f && my <= dy + line + 3.0f;

                        if (ip_hover)
                        {
                            renderer::draw_text(ip_x, dy, addr_buf, green);
                        }
                        else
                        {
                            renderer::draw_rect(ip_x, dy + 1.0f, ip_w, line - 2.0f, renderer::rgba(46, 44, 60, 255));
                        }
                    }
                    else
                    {
                        renderer::draw_text(bx + bw - pad - renderer::text_width("-"), dy, "-", dim);
                    }

                    dy += row_h;

                    renderer::draw_text(bx + pad, dy, "last seen", dim);

                    char ago[24];

                    recents::format_ago(re->last_seen, ago, sizeof(ago));

                    renderer::draw_text(bx + bw - pad - renderer::text_width(ago), dy, ago, green);

                    dy += row_h * 1.3f;

                    renderer::draw_text(bx + pad, dy, "xuid", dim);

                    char xuid_buf[24];

                    sprintf_s(xuid_buf, sizeof(xuid_buf), "%llu", static_cast<unsigned long long>(re->xuid));

                    renderer::draw_text(bx + pad, dy + row_h, xuid_buf, green);

                    dy += row_h * 2.0f;

                    if (re->addr_count > 0)
                    {
                        renderer::draw_text(bx + pad, dy, "previous addresses", dim);

                        dy += row_h;

                        for (int a = 0; a < re->addr_count; a++)
                        {
                            char abuf[32];

                            format_addr(re->history[a], abuf, sizeof(abuf));

                            renderer::draw_text(bx + pad + 8.0f, dy, abuf, dim);

                            dy += row_h;
                        }
                    }
                }
            }
        }
        else if (g_view == page_servers)
        {
            int sc = serverlist::count();

            int total_pages = serverlist::page_count();

            int page_start = g_servers_page * serverlist::entries_per_page;

            int page_n = sc - page_start;

            if (page_n > serverlist::entries_per_page)
            {
                page_n = serverlist::entries_per_page;
            }

            if (page_n < 0)
            {
                page_n = 0;
            }

            char scount[8];

            sprintf_s(scount, sizeof(scount), "%d", sc);

            renderer::draw_text(lx, body_y + text_dy, scount, green);

            renderer::draw_text(lx + renderer::text_width(scount) + 5.0f, body_y + text_dy, "servers", dim);

            char pgtxt[24];

            sprintf_s(pgtxt, sizeof(pgtxt), "page %d/%d", g_servers_page + 1, total_pages);

            renderer::draw_text(x + panel_w - pad - renderer::text_width(pgtxt), body_y + text_dy, pgtxt, dim);

            if (page_n == 0)
            {
                renderer::draw_text(lx, body_y + row_h + text_dy, "searching...", dim);
            }

            for (int i = 0; i < page_n; i++)
            {
                float row_y = body_y + (i + 1) * row_h;

                bool sel = g_sub_sel == i;

                if (sel)
                {
                    renderer::draw_rect(x + 2.0f, row_y, panel_w - 4.0f, row_h, highlight);
                }

                const serverlist::entry* se = serverlist::get(page_start + i);

                if (se == nullptr)
                {
                    continue;
                }

                if (se->last_state_ms != 0 && GetTickCount64() - se->last_state_ms < 5000)
                {
                    renderer::draw_rect(x + 8.0f, row_y + row_h * 0.5f - 3.0f, 6.0f, 6.0f, green);
                }

                char addr[32];

                sprintf_s(addr, sizeof(addr), "%s:%u", se->ip, se->port);

                renderer::draw_text(lx, row_y + text_dy, addr, sel ? text : dim);

                char info[16];

                sprintf_s(info, sizeof(info), "%u/%u", se->num_players, se->max_players);

                renderer::draw_text(x + panel_w - pad - renderer::text_width(info), row_y + text_dy, info, se->dedicated ? green : bar);
            }

            if (page_n > 0 && g_sub_sel < page_n)
            {
                serverlist::lock();

                const serverlist::entry* se = serverlist::get(page_start + g_sub_sel);

                if (se != nullptr)
                {
                    const char* head_text = se->dedicated ? "dedicated" : "peer to peer";

                    const char* type_text = se->dedicated ? "dedicated" : "p2p";

                    char addr_buf[32];

                    sprintf_s(addr_buf, sizeof(addr_buf), "%s:%u", se->ip, se->port);

                    char pl_buf[16];

                    sprintf_s(pl_buf, sizeof(pl_buf), "%u/%u", se->num_players, se->max_players);

                    char xuid_buf[24];

                    sprintf_s(xuid_buf, sizeof(xuid_buf), "%llu", static_cast<unsigned long long>(se->xuid));

                    float gap = 30.0f;

                    float content_w = renderer::text_width(head_text);

                    content_w = fmaxf(content_w, renderer::text_width("address") + gap + renderer::text_width(addr_buf));

                    content_w = fmaxf(content_w, renderer::text_width("players") + gap + renderer::text_width(pl_buf));

                    content_w = fmaxf(content_w, renderer::text_width("type") + gap + renderer::text_width(type_text));

                    content_w = fmaxf(content_w, renderer::text_width("host xuid"));

                    content_w = fmaxf(content_w, renderer::text_width(xuid_buf));

                    content_w = fmaxf(content_w, renderer::text_width("[enter] probe   [j] steam lobby"));

                    float bw = content_w + pad * 2.0f;

                    float bx = x + panel_w + 12.0f;

                    float bh = header_h + 7.0f * row_h + pad;

                    renderer::draw_rect(bx, y, bw, bh, back);

                    renderer::draw_rect(bx, y, 4.0f, bh, bar);

                    renderer::draw_text_shadow(bx + pad, y + pad * 0.9f, head_text, text, shadow);

                    renderer::draw_rect(bx + pad, y + header_h - 1.0f, bw - pad * 2.0f, 1.0f, sep);

                    float dy = y + header_h + 6.0f;

                    renderer::draw_text(bx + pad, dy, "address", dim);

                    renderer::draw_text(bx + bw - pad - renderer::text_width(addr_buf), dy, addr_buf, green);

                    dy += row_h;

                    renderer::draw_text(bx + pad, dy, "players", dim);

                    renderer::draw_text(bx + bw - pad - renderer::text_width(pl_buf), dy, pl_buf, text);

                    dy += row_h;

                    renderer::draw_text(bx + pad, dy, "type", dim);

                    renderer::draw_text(bx + bw - pad - renderer::text_width(type_text), dy, type_text, se->dedicated ? green : bar);

                    dy += row_h * 1.3f;

                    renderer::draw_text(bx + pad, dy, "host xuid", dim);

                    renderer::draw_text(bx + pad, dy + row_h, xuid_buf, green);

                    dy += row_h * 2.2f;

                    renderer::draw_text(bx + pad, dy, "[enter] probe   [j] steam lobby", dim);

                    int prows = se->probed ? se->player_count : 0;

                    char plabel[24];

                    sprintf_s(plabel, sizeof(plabel), "players (%d)", se->player_count);

                    float pcontent = renderer::text_width(plabel);

                    char rows[18][80];

                    char nums[18][8];

                    for (int pi = 0; pi < prows; pi++)
                    {
                        const serverlist::player& lp = se->players[pi];

                        if (lp.clan[0] != 0)
                        {
                            sprintf_s(rows[pi], sizeof(rows[pi]), "[%s] %s", lp.clan, lp.name);
                        }
                        else
                        {
                            sprintf_s(rows[pi], sizeof(rows[pi]), "%s", lp.name);
                        }

                        sprintf_s(nums[pi], sizeof(nums[pi]), "#%d", lp.client_num);

                        pcontent = fmaxf(pcontent, renderer::text_width(rows[pi]) + gap + renderer::text_width(nums[pi]));
                    }

                    if (!se->probed)
                    {
                        pcontent = fmaxf(pcontent, renderer::text_width("press enter to probe"));
                    }

                    float pbw = pcontent + pad * 2.0f;

                    float pbx = bx + bw + 12.0f;

                    int extra = se->probed ? (prows > 0 ? prows : 1) : 1;

                    float pbh = header_h + (extra + 0.5f) * row_h + pad;

                    renderer::draw_rect(pbx, y, pbw, pbh, back);

                    renderer::draw_rect(pbx, y, 4.0f, pbh, bar);

                    renderer::draw_text_shadow(pbx + pad, y + pad * 0.9f, plabel, text, shadow);

                    renderer::draw_rect(pbx + pad, y + header_h - 1.0f, pbw - pad * 2.0f, 1.0f, sep);

                    float pdy = y + header_h + 6.0f;

                    if (!se->probed)
                    {
                        renderer::draw_text(pbx + pad, pdy, "press enter to probe", dim);
                    }
                    else if (prows == 0)
                    {
                        renderer::draw_text(pbx + pad, pdy, "no players", dim);
                    }
                    else
                    {
                        for (int pi = 0; pi < prows; pi++)
                        {
                            renderer::draw_text(pbx + pad, pdy, rows[pi], text);

                            renderer::draw_text(pbx + pbw - pad - renderer::text_width(nums[pi]), pdy, nums[pi], green);

                            pdy += row_h;
                        }
                    }
                }

                serverlist::unlock();
            }
        }
        else
        {
            int c = 0;

            item* items = page_items(g_view, c);

            for (int i = 0; i < c; i++)
            {
                float row_y = body_y + i * row_h;

                bool sel = g_sub_sel == i;

                if (sel)
                {
                    renderer::draw_rect(x + 2.0f, row_y, panel_w - 4.0f, row_h, highlight);
                }

                renderer::draw_text(lx, row_y + text_dy, items[i].label, sel ? text : dim);

                if (items[i].toggle != nullptr)
                {
                    const char* state = *items[i].toggle ? "on" : "off";

                    uint32_t color = *items[i].toggle ? green : red;

                    renderer::draw_text(x + panel_w - pad - renderer::text_width(state), row_y + text_dy, state, color);
                }
                else
                {
                    renderer::draw_text(x + panel_w - pad - renderer::text_width(">"), row_y + text_dy, ">", bar);
                }
            }
        }

        uint32_t cursor_fill = renderer::rgba(240, 240, 246, 255);

        uint32_t cursor_edge = renderer::rgba(0, 0, 0, 190);

        for (int i = 0; i < 13; i++)
        {
            float w = i * 0.62f;

            renderer::draw_rect(mx, my + i, w + 3.0f, 1.0f, cursor_edge);
        }

        for (int i = 1; i < 12; i++)
        {
            float w = i * 0.62f;

            renderer::draw_rect(mx + 1.0f, my + i, w, 1.0f, cursor_fill);
        }
    }
}
