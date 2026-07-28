#include "steamqol.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <mutex>

namespace steamqol
{
    static bool __fastcall hk_is_dlc_installed(uint32_t app_id)
    {
        return true;
    }

    static bool __fastcall hk_is_app_installed(uint32_t app_id)
    {
        return true;
    }

    static char __fastcall hk_friend_list_rebuild(uint32_t controller)
    {
        if (!engine::friend_rebuild_throttle.ready())
        {
            return 1;
        }

        return engine::steam_friend_list_rebuild_fn(controller);
    }

    static double __fastcall hk_dlc_progress(uint32_t index)
    {
        if (index >= engine::dlc_progress_slots)
        {
            return engine::steam_dlc_progress_fn(index);
        }

        std::lock_guard<std::mutex> lock(engine::dlc_progress_mutex);

        uint64_t now = GetTickCount64();

        if (engine::dlc_progress[index].next != 0 && now < engine::dlc_progress[index].next)
        {
            return engine::dlc_progress[index].value;
        }

        engine::dlc_progress[index].value = engine::steam_dlc_progress_fn(index);

        engine::dlc_progress[index].next = now + engine::dlc_progress_interval_ms;

        return engine::dlc_progress[index].value;
    }

    static int64_t __fastcall hk_validate_auth(int64_t self, uint32_t* response)
    {
        if (engine::allow_all_auth && response != nullptr)
        {
            response[2] = 0;
        }

        return engine::validate_auth_response_fn(self, response);
    }

    void initialize()
    {
        engine::steam_is_dlc_installed_fn = reinterpret_cast<engine::steam_is_installed_t>(engine::base() + engine::steam_is_dlc_installed);

        utils::hook::attach(reinterpret_cast<void**>(&engine::steam_is_dlc_installed_fn), hk_is_dlc_installed);

        engine::steam_is_app_installed_fn = reinterpret_cast<engine::steam_is_installed_t>(engine::base() + engine::steam_is_app_installed);

        utils::hook::attach(reinterpret_cast<void**>(&engine::steam_is_app_installed_fn), hk_is_app_installed);

        engine::steam_friend_list_rebuild_fn = reinterpret_cast<engine::steam_friend_list_rebuild_t>(engine::base() + engine::steam_friend_list_rebuild);

        utils::hook::attach(reinterpret_cast<void**>(&engine::steam_friend_list_rebuild_fn), hk_friend_list_rebuild);

        engine::steam_dlc_progress_fn = reinterpret_cast<engine::steam_dlc_progress_t>(engine::base() + engine::steam_dlc_progress);

        utils::hook::attach(reinterpret_cast<void**>(&engine::steam_dlc_progress_fn), hk_dlc_progress);

        engine::validate_auth_response_fn = reinterpret_cast<engine::validate_auth_response_t>(engine::base() + engine::validate_auth_response);

       // utils::hook::attach(reinterpret_cast<void**>(&engine::validate_auth_response_fn), hk_validate_auth);

        T7_LOG(cx("steamqol: dlc/app + dlc-progress cache + friend-rebuild throttle installed (kills steam-ipc menu fps hitches)."));
    }
}
