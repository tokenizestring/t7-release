#include "hotkeys.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"
#include "../video/video.hpp"

namespace hotkeys
{
    static bool disconnect_latch = false;

    static bool disable_cutscenes_latch = false;

    static bool launch_latch = false;

    static void* __fastcall hk_frame_callback()
    {
        void* result = engine::steam_frame_callback_fn();

        bool disconnect_down = (GetAsyncKeyState(VK_F2) & 0x8000) != 0;

        if (disconnect_down && !disconnect_latch)
        {
            disconnect_latch = true;

            engine::cl_disconnect();

            T7_LOG(cx("hotkeys: F2 disconnect."));
        }
        else if (!disconnect_down)
        {
            disconnect_latch = false;
        }

        bool disable_down = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;

        if (disable_down && !disable_cutscenes_latch)
        {
            disable_cutscenes_latch = true;

            video::skip_cutscenes = !video::skip_cutscenes;

            if (video::skip_cutscenes) T7_LOG(cx("hotkeys: F1 cutscenes DISABLED."));
            else T7_LOG(cx("hotkeys: F1 cutscenes enabled."));
        }
        else if (!disable_down)
        {
            disable_cutscenes_latch = false;
        }

        bool launch_down = (GetAsyncKeyState(VK_F5) & 0x8000) != 0;

        if (launch_down && !launch_latch)
        {
            launch_latch = true;

            if (engine::cbuf_add_text_fn != nullptr)
            {
                engine::cbuf_add_text_fn(0, cx("launchGame; party_minplayers 1; lobby_setmap mp_nuketown_x; launchGame;").c_str());

                T7_LOG(cx("hotkeys: F5 launch host match."));
            }
        }
        else if (!launch_down)
        {
            launch_latch = false;
        }

        return result;
    }

    void initialize()
    {
        engine::steam_frame_callback_fn = reinterpret_cast<engine::steam_frame_callback_t>(engine::base() + engine::steam_frame_callback);

        utils::hook::attach(reinterpret_cast<void**>(&engine::steam_frame_callback_fn), hk_frame_callback);

        engine::cbuf_add_text_fn = reinterpret_cast<engine::cbuf_add_text_t>(engine::base() + engine::cbuf_add_text);

        T7_LOG(cx("hotkeys: F1 cutscenes, F2 disconnect, F5 launch (main-thread frame hook)."));
    }
}
