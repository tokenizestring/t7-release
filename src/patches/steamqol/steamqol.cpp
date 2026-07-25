#include "steamqol.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <unordered_map>
#include <mutex>

namespace steamqol
{
    struct install_cache
    {
        std::mutex mutex;

        std::unordered_map<uint32_t, bool> entries;

        bool query(engine::steam_is_installed_t original, uint32_t app_id)
        {
            std::lock_guard<std::mutex> lock(mutex);

            auto entry = entries.find(app_id);

            if (entry != entries.end())
            {
                return entry->second;
            }

            bool result = original(app_id);

            entries[app_id] = result;

            return result;
        }
    };

    struct frame_throttle
    {
        uint64_t interval_ms = 0;

        uint64_t last = 0;

        bool ready()
        {
            uint64_t now = GetTickCount64();

            if (last != 0 && now - last < interval_ms)
            {
                return false;
            }

            last = now;

            return true;
        }
    };

    static install_cache dlc_cache;

    static install_cache app_cache;

    static frame_throttle friend_rebuild_throttle{ 20000 };

    static bool __fastcall hk_is_dlc_installed(uint32_t app_id)
    {
        return dlc_cache.query(engine::steam_is_dlc_installed_fn, app_id);
    }

    static bool __fastcall hk_is_app_installed(uint32_t app_id)
    {
        return app_cache.query(engine::steam_is_app_installed_fn, app_id);
    }

    static char __fastcall hk_friend_list_rebuild(uint32_t controller)
    {
        if (!friend_rebuild_throttle.ready())
        {
            return 1;
        }

        return engine::steam_friend_list_rebuild_fn(controller);
    }

    void initialize()
    {
        engine::steam_is_dlc_installed_fn = reinterpret_cast<engine::steam_is_installed_t>(engine::base() + engine::steam_is_dlc_installed);

        utils::hook::attach(reinterpret_cast<void**>(&engine::steam_is_dlc_installed_fn), hk_is_dlc_installed);

        engine::steam_is_app_installed_fn = reinterpret_cast<engine::steam_is_installed_t>(engine::base() + engine::steam_is_app_installed);

        utils::hook::attach(reinterpret_cast<void**>(&engine::steam_is_app_installed_fn), hk_is_app_installed);

        engine::steam_friend_list_rebuild_fn = reinterpret_cast<engine::steam_friend_list_rebuild_t>(engine::base() + engine::steam_friend_list_rebuild);

        utils::hook::attach(reinterpret_cast<void**>(&engine::steam_friend_list_rebuild_fn), hk_friend_list_rebuild);

        T7_LOG(cx("steamqol: dlc/app cache + friend-rebuild throttle installed (kills steam-ipc menu fps hitches)."));
    }
}
