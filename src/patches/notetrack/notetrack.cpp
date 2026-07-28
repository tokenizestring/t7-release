#include "notetrack.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"
#include "../../features/overlay/overlay.hpp"

static uint64_t blocked_count = 0;

static float __fastcall hk_notetrack_lookup(int64_t anim_table, int anim_index, int notetrack_hash)
{
    if (!engine::protection.notetrack)
    {
        return engine::notetrack_anim_lookup_fn(anim_table, anim_index, notetrack_hash);
    }

    uint64_t entry = *reinterpret_cast<uint64_t*>(anim_table + 24 * static_cast<int64_t>(anim_index) + 56);

    if (entry < engine::min_asset_pointer)
    {
        blocked_count++;

        if (blocked_count == 1 || blocked_count % 128 == 0)
        {
            T7_LOG(std::string(cx("notetrack: bad xanim entry (x")) + std::to_string(blocked_count) + cx("), dropped."));

            features::overlay::notify(cx("blocked crash attempt."), features::overlay::level::bad);
        }

        return -1.0f;
    }

    return engine::notetrack_anim_lookup_fn(anim_table, anim_index, notetrack_hash);
}

namespace notetrack
{
    void initialize()
    {
        engine::notetrack_anim_lookup_fn = reinterpret_cast<engine::notetrack_anim_lookup_t>(engine::base() + engine::notetrack_anim_lookup);

        utils::hook::attach(reinterpret_cast<void**>(&engine::notetrack_anim_lookup_fn), hk_notetrack_lookup);

        T7_LOG(cx("notetrack: xanim notetrack crash protection installed."));
    }
}
