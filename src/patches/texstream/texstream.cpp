#include "texstream.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

namespace texstream
{
    static int64_t __fastcall hk_resident_mip(int64_t a1, int a2, int64_t a3)
    {
        if (a2 >= 0 && a2 < engine::resident_mip_floor)
        {
            a2 = engine::resident_mip_floor;
        }

        return engine::resident_mip_request_fn(a1, a2, a3);
    }

    void initialize()
    {
        engine::resident_mip_request_fn = reinterpret_cast<engine::resident_mip_request_t>(engine::base() + engine::resident_mip_request);

        // utils::hook::attach(reinterpret_cast<void**>(&engine::resident_mip_request_fn), hk_resident_mip); needs rework, kinda beaming textures lmao

        T7_LOG(cx("texstream: resident-mip floor raised (less texture pop-in)."));
    }
}
