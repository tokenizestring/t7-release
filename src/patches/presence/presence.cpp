#include "presence.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"
#include "../../features/overlay/overlay.hpp"

#include <cstring>

namespace presence
{
    static constexpr int max_players = 18;

    static bool hide_presence = true;

    static void __fastcall hk_presence_pack(engine::presence_data_s* presence, void* buffer, size_t buffer_size)
    {
        if (presence != nullptr && hide_presence)
        {
            memset(&presence->title, 0, sizeof(presence->title));

            memset(&presence->platform, 0, sizeof(presence->platform));
        }

        engine::presence_pack(presence, buffer, buffer_size);
    }

    static __int64 __fastcall hk_presence_serialize(void* presence, void* buffer)
    {
        if (presence == nullptr || buffer == nullptr)
        {
            return 0;
        }

        int* packed = *reinterpret_cast<int**>(reinterpret_cast<uint8_t*>(presence) + 16);

        if (packed != nullptr)
        {
            int count = (*packed >> 2) & 0x1F;

            if (engine::protection.player_presence && count > max_players)
            {
                *packed = (*packed & ~(0x1F << 2)) | (max_players << 2);

                T7_LOG(std::string(cx("presence: player count from ")) + std::to_string(count) + cx(" to ") + std::to_string(max_players) + cx(", ignored."));

                features::overlay::notify(cx("blocked crash attempt."), features::overlay::level::bad);
            }
        }

        return engine::presence_serialize(presence, buffer);
    }

    void initialize()
    {
        engine::presence_serialize = reinterpret_cast<engine::live_presence_serialize_t>(engine::base() + engine::live_presence_serialize);

        utils::hook::attach(reinterpret_cast<void**>(&engine::presence_serialize), hk_presence_serialize);

        engine::presence_pack = reinterpret_cast<engine::live_presence_pack_t>(engine::base() + engine::live_presence_pack);

        utils::hook::attach(reinterpret_cast<void**>(&engine::presence_pack), hk_presence_pack);

        T7_LOG(cx("presence: crash protection installed."));
    }
}
