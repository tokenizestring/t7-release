#include "clientfield.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"
#include "../../features/overlay/overlay.hpp"

#include <string>
#include <cstring>

namespace clientfield
{
    static char __fastcall hk_clientfield_checksum(int server_checksum)
    {
        int local_checksum = *reinterpret_cast<int*>(engine::base() + engine::clientfield_local_checksum);

        if (!engine::clientfield_fix || server_checksum == local_checksum)
        {
            return engine::clientfield_checksum_fn(server_checksum);
        }

        T7_LOG(cx("clientfield: local checksum forced to accept (client-side path)."));

        return engine::clientfield_checksum_fn(local_checksum);
    }

    static int64_t __fastcall hk_disconnect_reason(const char* reason)
    {
        if (reason != nullptr)
        {
            T7_LOG(std::string(cx("clientfield: disconnect reason = ")) + reason + cx("."));
        }

        if (engine::clientfield_fix && reason != nullptr && strcmp(reason, cx("EXE_CLIENT_FIELD_MISMATCH").c_str()) == 0)
        {
            T7_LOG(cx("clientfield: suppressed client/server index mismatch drop."));

            features::overlay::notify(cx("blocked client/server mismatch kick."), features::overlay::level::good);

            return 0;
        }

        return engine::disconnect_reason_fn(reason);
    }

    void initialize()
    {
        /*engine::clientfield_checksum_fn = reinterpret_cast<engine::clientfield_checksum_t>(engine::base() + engine::clientfield_checksum_validate);

        utils::hook::attach(reinterpret_cast<void**>(&engine::clientfield_checksum_fn), hk_clientfield_checksum);

        engine::disconnect_reason_fn = reinterpret_cast<engine::disconnect_reason_t>(engine::base() + engine::disconnect_reason);

        utils::hook::attach(reinterpret_cast<void**>(&engine::disconnect_reason_fn), hk_disconnect_reason);*/

        // testing not currently working T7_LOG(cx("clientfield: mismatch self-kick guard installed (checksum + disconnect-reason chokepoint)."));
    }
}
