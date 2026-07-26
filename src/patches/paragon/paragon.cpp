#include "paragon.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"
#include "../../features/overlay/overlay.hpp"

static engine::paragon_icon_entry safe_entry = {};

static char empty_name[1] = {};

static uint64_t blocked_count = 0;

static char* __fastcall hk_paragon_icon_name(uint32_t mode, int icon_id)
{
    char* result = engine::paragon_icon_name_fn(mode, icon_id);

    if (!engine::protection.paragon_icons || engine::paragon_icon_in_table(result))
    {
        return result;
    }

    blocked_count++;

    if (blocked_count == 1 || blocked_count % 300 == 0)
    {
        T7_LOG(std::string(cx("paragon: malformed paragon-icon lookup (x")) + std::to_string(blocked_count) + cx("), dropped."));

        features::overlay::notify(cx("blocked scoreboard crash."), features::overlay::level::bad);
    }

    return reinterpret_cast<char*>(&safe_entry);
}

namespace paragon
{
    void initialize()
    {
        safe_entry.name = empty_name;

        engine::paragon_icon_name_fn = reinterpret_cast<engine::paragon_icon_name_t>(engine::base() + engine::paragon_icon_name);

        utils::hook::attach(reinterpret_cast<void**>(&engine::paragon_icon_name_fn), hk_paragon_icon_name);

        T7_LOG(cx("paragon: paragon-icon crash protection installed."));
    }
}
