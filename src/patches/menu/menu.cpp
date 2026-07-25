#include "menu.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"
#include "../../features/overlay/overlay.hpp"

namespace menu
{
    static uint64_t blocked = 0;

    static int64_t __fastcall hk_open_menu(uint32_t controller, int menu_id) // weird gsc crash people have been using lmao
    {
        if (engine::current_menu_id() == menu_id)
        {
            if (++blocked % 500 == 1)
            {
                T7_LOG(std::string(cx("menu: openmenu spam (x")) + std::to_string(blocked) + cx("), dropped."));

                features::overlay::notify(cx("blocked menu spam."), features::overlay::level::bad);
            }

            return 0;
        }

        return engine::open_menu_fn(controller, menu_id);
    }

    void initialize()
    {
        engine::open_menu_fn = reinterpret_cast<engine::open_menu_t>(engine::base() + engine::open_menu);

        utils::hook::attach(reinterpret_cast<void**>(&engine::open_menu_fn), hk_open_menu);

        T7_LOG(cx("menu: openmenu spam protection installed."));
    }
}
