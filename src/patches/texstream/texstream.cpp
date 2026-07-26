#include "texstream.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

namespace texstream
{
    static int64_t __fastcall hk_stream_throttle(char a1, int64_t a2, int64_t a3)
    {
        if (engine::texstream_enabled)
        {
            *reinterpret_cast<int*>(engine::base() + engine::stream_frame_budget) = engine::stream_budget_boost;
        }

        return engine::stream_throttle_fn(a1, a2, a3);
    }

    void initialize()
    {
        engine::stream_throttle_fn = reinterpret_cast<engine::stream_throttle_t>(engine::base() + engine::stream_throttle);

        utils::hook::attach(reinterpret_cast<void**>(&engine::stream_throttle_fn), hk_stream_throttle);

        T7_LOG(cx("texstream: per-frame stream budget raised (textures load in faster, no pop-in)."));
    }
}
