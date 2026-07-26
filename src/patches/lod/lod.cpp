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

        if (!engine::lod_enabled || out_params == nullptr || flags == nullptr)
        {
            return;
        }

        if ((*flags & engine::lod_shadow_reflection_mask) != 0)
        {
            return;
        }

        float* cull = reinterpret_cast<float*>(reinterpret_cast<char*>(out_params) + engine::lod_params_cull_offset);

        if (*cull > 0.0f)
        {
            *cull *= engine::lod_cull_multiplier;
        }
    }

    void initialize()
    {
        engine::lod_params_reader_fn = reinterpret_cast<engine::lod_params_reader_t>(engine::base() + engine::lod_params_reader);

        utils::hook::attach(reinterpret_cast<void**>(&engine::lod_params_reader_fn), hk_lod_params);

        T7_LOG(cx("lod: cull radius extended (less pop-in)."));
    }
}
