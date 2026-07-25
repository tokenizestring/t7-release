#include "perf.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

namespace perf
{
    static int64_t __fastcall hk_fps_cap(uint32_t local_client)
    {
        return 0;
    }

    void initialize()
    {
        engine::fps_cap_fn = reinterpret_cast<engine::fps_cap_t>(engine::base() + engine::frame_fps_cap);

        utils::hook::attach(reinterpret_cast<void**>(&engine::fps_cap_fn), hk_fps_cap);

        T7_LOG(cx("perf: secondary fps cap removed (uncaps loading screen)."));
    }
}
