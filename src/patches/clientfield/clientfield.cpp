#include "clientfield.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

namespace clientfield
{
    static bool logged_once = false;

    static char __fastcall hk_clientfield_checksum(int server_checksum)
    {
        int local_checksum = *reinterpret_cast<int*>(engine::base() + engine::clientfield_local_checksum);

        if (server_checksum != local_checksum && !logged_once)
        {
            logged_once = true;

            T7_LOG(cx("clientfield: registration checksum differed, staying in game instead of dropping."));
        }

        return engine::clientfield_checksum_fn(local_checksum);
    }

    void initialize()
    {
        engine::clientfield_checksum_fn = reinterpret_cast<engine::clientfield_checksum_t>(engine::base() + engine::clientfield_checksum_validate);

        utils::hook::attach(reinterpret_cast<void**>(&engine::clientfield_checksum_fn), hk_clientfield_checksum);

        T7_LOG(cx("clientfield: index mismatch self-kick fixed."));
    }
}
