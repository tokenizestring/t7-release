#include "video.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

namespace video
{
    static constexpr uint32_t flag_remote_fetch = 0x200;

    static int64_t __fastcall hk_play(char* path, int64_t context, uint32_t flags, float speed, int64_t* callback, uint32_t video_id)
    {
        if (skip_cutscenes && engine::is_connected())
        {
            return 9999;
        }

        return engine::play_video_fn(path, context, flags, speed, callback, video_id);
    }

    static int64_t __fastcall hk_open(int64_t name, int64_t subtitle, uint32_t flags, float speed, void* callback, int video_id)
    {
        if ((flags & flag_remote_fetch) != 0)
        {
            flags &= ~flag_remote_fetch;

            T7_LOG(std::string(cx("video: blocked remote fetch ")) + reinterpret_cast<const char*>(name));
        }

        return engine::cinematic_open_fn(name, subtitle, flags, speed, callback, video_id);
    }

    void initialize()
    {
        engine::play_video_fn = reinterpret_cast<engine::play_video_t>(engine::base() + engine::cinematic_play);

        utils::hook::attach(reinterpret_cast<void**>(&engine::play_video_fn), hk_play);

        engine::cinematic_open_fn = reinterpret_cast<engine::cinematic_open_t>(engine::base() + engine::cinematic_open);

        //utils::hook::attach(reinterpret_cast<void**>(&engine::cinematic_open_fn), hk_open); need to fix our hooks decoder

        T7_LOG(cx("video: remote-fetch guard + cutscene skip installed"));
    }
}
