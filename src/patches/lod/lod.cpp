#include "lod.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

namespace lod
{
    static void __fastcall hk_lod_params(void* view, int* cam_pos, int* flags, void* out_params)
    {
        engine::lod_params_reader_fn(view, cam_pos, flags, out_params);

        if (out_params != nullptr)
        {
            char* params = reinterpret_cast<char*>(out_params);

            *reinterpret_cast<float*>(params + engine::lod_params_cull_offset) = engine::lod_cull_disabled;
        }
    }

    void initialize()
    {
        engine::lod_params_reader_fn = reinterpret_cast<engine::lod_params_reader_t>(engine::base() + engine::lod_params_reader);

        utils::hook::attach(reinterpret_cast<void**>(&engine::lod_params_reader_fn), hk_lod_params);

        T7_LOG(cx("lod: auto-cull disabled + lod scale boosted (no pop-in)."));
    }
}
