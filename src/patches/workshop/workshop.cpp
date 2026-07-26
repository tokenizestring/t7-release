#include "workshop.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"
#include "../../features/overlay/overlay.hpp"

#include <cstring>

namespace workshop
{
    static bool toggle_latch = false;

    static int64_t __fastcall hk_apply_game_state(uint32_t a1, int64_t a2, int64_t a3, int64_t message)
    {
        if (engine::protection.workshop_mods && message != 0)
        {
            engine::lobby_state_msg_s* state = reinterpret_cast<engine::lobby_state_msg_s*>(message);

            if (state->ugc_name[0] != 0)
            {
                char name[33] = {};

                memcpy(name, state->ugc_name, sizeof(state->ugc_name));

                T7_LOG(std::string(cx("workshop: host-forced ugc '")) + name + cx("', dropped."));

                features::overlay::notify(cx("blocked forced workshop mod."), features::overlay::level::bad);

                memset(state->ugc_name, 0, sizeof(state->ugc_name));

                state->ugc_version = 0;
            }
        }

        return engine::apply_game_state_fn(a1, a2, a3, message);
    }

    void initialize()
    {
        engine::apply_game_state_fn = reinterpret_cast<engine::apply_game_state_t>(engine::base() + engine::lobby_apply_game_state);

        utils::hook::attach(reinterpret_cast<void**>(&engine::apply_game_state_fn), hk_apply_game_state);

        T7_LOG(cx("workshop: host-forced ugc protection installed (F6 toggles)."));
    }

    void tick()
    {
        bool down = (GetAsyncKeyState(VK_F6) & 0x8000) != 0;

        if (down && !toggle_latch)
        {
            toggle_latch = true;

            engine::protection.workshop_mods = !engine::protection.workshop_mods;

            if (engine::protection.workshop_mods) T7_LOG(cx("workshop: enabled - host-forced ugc blocked."));
            else T7_LOG(cx("workshop: disabled - host-forced ugc allowed."));
        }
        else if (!down)
        {
            toggle_latch = false;
        }
    }
}
