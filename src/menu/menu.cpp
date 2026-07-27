#include "menu.hpp"
#include "../engine/engine.hpp"
#include "../features/overlay/renderer.hpp"
#include "../features/menulogo/menulogo.hpp"
#include "../patches/video/video.hpp"
#include "../utils/hook/hook.hpp"

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

    static item general_items[] =
    {
        { "notifications", &engine::notifications_enabled, nullptr },
        { "skip cutscenes", &video::skip_cutscenes, nullptr },
        { "anti lod pop-in", &engine::lod_enabled, nullptr },
        { "faster texture streaming", &engine::texstream_enabled, nullptr },
        { "fps-safe movement", &engine::movement_tick_enabled, nullptr },
        { "no mismatch kick", &engine::clientfield_fix, nullptr },
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
        { "p2p userdata", &engine::protection.p2p_userdata, nullptr },
        { "info leak", &engine::protection.info_leak, nullptr },
        { "host-forced mods", &engine::protection.workshop_mods, nullptr },
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

    static constexpr int page_about = 3;

    static constexpr int page_exit = 4;

    static constexpr int root_count = 5;

    static bool g_open = false;

    static int g_view = -1;

    static int g_root_sel = 0;

    static int g_sub_sel = 0;

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

        int ping_band;

        bool has_ip;

        unsigned char ip[4];

        uint32_t nat;
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

            p.ping_band = *reinterpret_cast<uint8_t*>(client + engine::lobby_client_ping_band);

            uint8_t* conn = *reinterpret_cast<uint8_t**>(client + engine::lobby_client_conn);

            if (reinterpret_cast<uintptr_t>(conn) >= 0x10000)
            {
                p.ip[0] = conn[engine::conn_ip];

                p.ip[1] = conn[engine::conn_ip + 1];

                p.ip[2] = conn[engine::conn_ip + 2];

                p.ip[3] = conn[engine::conn_ip + 3];

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

        if (i == page_about)
        {
            return "about";
        }

        if (i == page_exit)
        {
            return "exit";
        }

        return "";
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

    typedef uint32_t(__stdcall* xinput_get_state_t)(uint32_t index, void* state);

    static xinput_get_state_t g_xinput = nullptr;

    static bool g_xinput_tried = false;

    struct pad
    {
        bool up;

        bool down;

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

        static bool pad_latch_accept = false;

        static bool pad_latch_cancel = false;

        pad gp;

        read_pad(gp);

        bool up = edge(VK_UP, latch_up) || pad_edge(gp.up, pad_latch_up);

        bool down = edge(VK_DOWN, latch_down) || pad_edge(gp.down, pad_latch_down);

        bool enter = edge(VK_RETURN, latch_enter) || pad_edge(gp.accept, pad_latch_accept);

        bool back = edge(VK_BACK, latch_back) || pad_edge(gp.cancel, pad_latch_cancel);

        bool left = edge(VK_LEFT, latch_left);

        if (g_view == -1)
        {
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
            g_view = -1;

            g_sub_sel = 0;

            return;
        }

        int count = 0;

        page_items(g_view, count);

        if (g_view == page_clients)
        {
            count = gather_players();
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
            activate();
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

        if (g_view == page_about)
        {
            body_rows = sizeof(about_lines) / sizeof(about_lines[0]);
        }
        else if (g_view == page_clients)
        {
            body_rows = 1 + (n > 0 ? n : 1);
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
            hover_rows = n > 12 ? 12 : n;

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

                if (g_view != page_clients)
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
                g_view = -1;

                g_sub_sel = 0;
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
                else
                {
                    int lbars = lp.ping_band < 0 ? 0 : (lp.ping_band > 4 ? 4 : lp.ping_band);

                    float px = x + panel_w - pad - 4.0f * 5.0f;

                    for (int b = 0; b < 4; b++)
                    {
                        uint32_t bc = b < lbars ? green : renderer::rgba(60, 58, 74, 255);

                        float pbh = 2.0f + b * 2.5f;

                        renderer::draw_rect(px + b * 5.0f, row_y + row_h * 0.5f + 4.0f - pbh, 3.0f, pbh, bc);
                    }
                }
            }

            if (n > 0)
            {
                lobby_player& p = g_players[g_sub_sel];

                float bw = 270.0f;

                float bx = x + panel_w + 12.0f;

                float bh = header_h + 11.0f * row_h + pad;

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

                renderer::draw_text(bx + pad, dy, "ping", dim);

                int bars = p.ping_band < 0 ? 0 : (p.ping_band > 4 ? 4 : p.ping_band);

                float bar_x = bx + bw - pad - 4.0f * 7.0f;

                for (int b = 0; b < 4; b++)
                {
                    uint32_t bc = b < bars ? green : renderer::rgba(60, 58, 74, 255);

                    float bh2 = 3.0f + b * 3.0f;

                    renderer::draw_rect(bar_x + b * 7.0f, dy + line - bh2, 5.0f, bh2, bc);
                }

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
