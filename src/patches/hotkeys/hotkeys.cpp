#include "hotkeys.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

namespace hotkeys
{
    static bool disconnect_latch = false;

    static bool disable_cutscenes = false;

    static bool disable_cutscenes_latch = false;

    static void* __fastcall hk_frame_callback()
    {
        void* result = engine::steam_frame_callback_fn();

        bool disconnect_down = (GetAsyncKeyState(VK_F2) & 0x8000) != 0;

        if (disconnect_down && !disconnect_latch)
        {
            disconnect_latch = true;

            engine::cl_disconnect();

            T7_LOG(cx("hotkeys: F2 disconnect"));
        }
        else if (!disconnect_down)
        {
            disconnect_latch = false;
        }

        bool disable_down = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;

        if (disable_down && !disable_cutscenes_latch)
        {
            disable_cutscenes_latch = true;

            disable_cutscenes = !disable_cutscenes;

            if (disable_cutscenes) T7_LOG(cx("hotkeys: F1 cutscenes DISABLED"));
            else T7_LOG(cx("hotkeys: F1 cutscenes enabled"));
        }
        else if (!disable_down)
        {
            disable_cutscenes_latch = false;
        }

        if (disable_cutscenes && engine::is_connected() && engine::cinematic_playing())
        {
            engine::cinematic_stop_all();
        }

        return result;
    }

    void initialize()
    {
        engine::steam_frame_callback_fn = reinterpret_cast<engine::steam_frame_callback_t>(engine::base() + engine::steam_frame_callback);

        utils::hook::attach(reinterpret_cast<void**>(&engine::steam_frame_callback_fn), hk_frame_callback);

        T7_LOG(cx("hotkeys: F1 toggle disable cutscenes, F2 disconnect (main-thread frame hook)"));
    }
}
