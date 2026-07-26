#include "menu.hpp"
#include "../engine/engine.hpp"
#include "../features/overlay/renderer.hpp"
#include "../patches/video/video.hpp"

using namespace overlay;

namespace menu
{
    struct item
    {
        const char* label;

        bool* toggle;

        void (*action)();
    };

    static void action_close();

    static void action_disconnect()
    {
        engine::cl_disconnect();
    }

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

    static item settings_items[] =
    {
        { "notifications", &engine::notifications_enabled, nullptr },
        { "skip cutscenes", &video::skip_cutscenes, nullptr },
        { "anti lod pop-in", &engine::lod_enabled, nullptr },
        { "faster texture streaming", &engine::texstream_enabled, nullptr },
        { "block steam p2p", &engine::block_p2p, nullptr },
        { "block raw udp (breaks dedis)", &engine::block_netchan, nullptr },
        { "fps-safe movement (125hz)", &engine::movement_tick_enabled, nullptr },
        { "disconnect", nullptr, action_disconnect },
    };

    static item exit_items[] =
    {
        { "close menu", nullptr, action_close },
    };

    static const char* about_lines[] =
    {
        "t7-release",
        "black ops III protection + quality of life",
        "hook-based, zero dependencies",
        "every protection is on by default,",
        "toggle anything off from the protection tab.",
        "press insert to open or close this menu.",
    };

    static const char* tab_names[] = { "protection", "settings", "about", "exit" };

    static constexpr int tab_count = 4;

    static bool g_open = false;

    static int g_tab = 0;

    static int g_selected = 0;

    static void action_close()
    {
        g_open = false;
    }

    static item* tab_items(int tab, int& count)
    {
        if (tab == 0)
        {
            count = sizeof(protection_items) / sizeof(protection_items[0]);

            return protection_items;
        }

        if (tab == 1)
        {
            count = sizeof(settings_items) / sizeof(settings_items[0]);

            return settings_items;
        }

        if (tab == 3)
        {
            count = sizeof(exit_items) / sizeof(exit_items[0]);

            return exit_items;
        }

        count = 0;

        return nullptr;
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

        static bool latch_left = false;

        static bool latch_right = false;

        static bool latch_up = false;

        static bool latch_down = false;

        static bool latch_enter = false;

        if (edge(VK_INSERT, latch_open))
        {
            g_open = !g_open;
        }

        if (!g_open)
        {
            return;
        }

        if (edge(VK_LEFT, latch_left))
        {
            g_tab = (g_tab - 1 + tab_count) % tab_count;

            g_selected = 0;
        }

        if (edge(VK_RIGHT, latch_right))
        {
            g_tab = (g_tab + 1) % tab_count;

            g_selected = 0;
        }

        int count = 0;

        item* items = tab_items(g_tab, count);

        if (count > 0)
        {
            if (edge(VK_UP, latch_up))
            {
                g_selected = (g_selected - 1 + count) % count;
            }

            if (edge(VK_DOWN, latch_down))
            {
                g_selected = (g_selected + 1) % count;
            }

            if (edge(VK_RETURN, latch_enter))
            {
                item& selected = items[g_selected];

                if (selected.toggle != nullptr)
                {
                    *selected.toggle = !*selected.toggle;
                }
                else if (selected.action != nullptr)
                {
                    selected.action();
                }
            }
        }
    }

    void tick(float screen_w, float screen_h)
    {
        handle_input();

        if (!g_open)
        {
            return;
        }

        uint32_t back = renderer::rgba(12, 12, 16, 236);

        uint32_t bar = renderer::rgba(120, 90, 220, 255);

        uint32_t title_bg = renderer::rgba(24, 24, 30, 255);

        uint32_t tab_bg = renderer::rgba(18, 18, 24, 255);

        uint32_t highlight = renderer::rgba(120, 90, 220, 70);

        uint32_t text = renderer::rgba(238, 238, 244, 255);

        uint32_t dim = renderer::rgba(150, 150, 162, 255);

        uint32_t shadow = renderer::rgba(0, 0, 0, 200);

        uint32_t on = renderer::rgba(80, 200, 120, 255);

        uint32_t off = renderer::rgba(210, 90, 90, 255);

        float line = renderer::line_height();

        float row_h = line + 8.0f;

        float title_h = line + 16.0f;

        float tab_h = line + 12.0f;

        float panel_w = 400.0f;

        float body_rows = 13.0f;

        float panel_h = title_h + tab_h + body_rows * row_h + 14.0f;

        float x = 60.0f;

        float y = (screen_h - panel_h) * 0.5f;

        renderer::draw_rect(x, y, panel_w, panel_h, back);

        renderer::draw_rect(x, y, panel_w, title_h, title_bg);

        renderer::draw_rect(x, y, 4.0f, panel_h, bar);

        renderer::draw_text_shadow(x + 16.0f, y + 9.0f, "t7-release", text, shadow);

        float tab_y = y + title_h;

        renderer::draw_rect(x, tab_y, panel_w, tab_h, tab_bg);

        float tab_x = x + 14.0f;

        for (int i = 0; i < tab_count; i++)
        {
            float w = renderer::text_width(tab_names[i]);

            if (i == g_tab)
            {
                renderer::draw_rect(tab_x - 6.0f, tab_y, w + 12.0f, tab_h, highlight);

                renderer::draw_rect(tab_x - 6.0f, tab_y + tab_h - 2.0f, w + 12.0f, 2.0f, bar);
            }

            renderer::draw_text(tab_x, tab_y + 6.0f, tab_names[i], i == g_tab ? text : dim);

            tab_x += w + 26.0f;
        }

        float body_y = tab_y + tab_h + 6.0f;

        if (g_tab == 2)
        {
            int n = sizeof(about_lines) / sizeof(about_lines[0]);

            for (int i = 0; i < n; i++)
            {
                renderer::draw_text(x + 18.0f, body_y + i * row_h + 4.0f, about_lines[i], i == 0 ? text : dim);
            }
        }
        else
        {
            int count = 0;

            item* items = tab_items(g_tab, count);

            for (int i = 0; i < count; i++)
            {
                float row_y = body_y + i * row_h;

                if (i == g_selected)
                {
                    renderer::draw_rect(x + 2.0f, row_y, panel_w - 4.0f, row_h, highlight);
                }

                renderer::draw_text(x + 18.0f, row_y + 4.0f, items[i].label, i == g_selected ? text : dim);

                if (items[i].toggle != nullptr)
                {
                    const char* state = *items[i].toggle ? "ON" : "OFF";

                    uint32_t color = *items[i].toggle ? on : off;

                    renderer::draw_text(x + panel_w - 20.0f - renderer::text_width(state), row_y + 4.0f, state, color);
                }
                else
                {
                    renderer::draw_text(x + panel_w - 20.0f - renderer::text_width(">"), row_y + 4.0f, ">", bar);
                }
            }
        }

        renderer::draw_text(x + 16.0f, y + panel_h + 8.0f, "insert close   left/right tabs   up/down move   enter select", dim);
    }
}
