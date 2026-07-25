#include "p2p.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"
#include "../../features/overlay/overlay.hpp"

#include <string>

static bool block_enabled = false;

static bool toggle_latch = false;

static char __fastcall hk_send(void* ctx, uint64_t steam_id, void* data, uint32_t size)
{
    uint8_t type = data != nullptr ? *reinterpret_cast<uint8_t*>(data) : 0;

    T7_LOG(std::string(cx("p2p: send ")) + std::to_string(size) + cx("b to ") + std::to_string(steam_id) + cx(" type ") + std::to_string(type) + (block_enabled ? cx(" dropped.") : cx(".")));

    if (block_enabled)
    {
        return 1;
    }

    return engine::steam_p2p_send_fn(ctx, steam_id, data, size);
}

static int64_t __fastcall hk_dispatch(uint64_t sender, uint32_t type, void* data, int size)
{
    T7_LOG(std::string(cx("p2p: recv ")) + std::to_string(size) + cx("b from ") + std::to_string(sender) + cx(" type ") + std::to_string(type) + (block_enabled ? cx(" dropped.") : cx(".")));

    if (block_enabled)
    {
        return 1;
    }

    if (type == engine::live_userdata_type && data != nullptr)
    {
        uint32_t claimed = *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(data) + 4);

        if (claimed > engine::max_userdata_size || static_cast<int>(claimed) + 8 > size)
        {
            T7_LOG(std::string(cx("p2p: oversized userdata ")) + std::to_string(claimed) + cx(" bytes from ") + std::to_string(sender) + cx(", dropped."));

            features::overlay::notify(cx("blocked crash attempt."), features::overlay::level::bad);

            return 1;
        }
    }

    return engine::steam_p2p_dispatch_fn(sender, type, data, size);
}

static char __fastcall hk_accept(void* ctx, uint64_t* steam_id)
{
    uint64_t id = steam_id != nullptr ? *steam_id : 0;

    if (block_enabled)
    {
        T7_LOG(std::string(cx("p2p: session request from ")) + std::to_string(id) + cx(" dropped."));

        return 0;
    }

   /* if (id != 0 && !engine::is_friend(0, static_cast<int64_t>(id)))
    {
        T7_LOG("p2p: session request from non-friend " + std::to_string(id) + " rejected (ip-leak guard).");

        return 0;
    }*/ // kinda impossible if you wanted to play p2p matches

    return engine::steam_p2p_accept_fn(ctx, steam_id);
}

namespace p2p 
{
    void initialize()
    {
        engine::steam_p2p_send_fn = reinterpret_cast<engine::steam_p2p_send_t>(engine::base() + engine::steam_p2p_send);

        utils::hook::attach(reinterpret_cast<void**>(&engine::steam_p2p_send_fn), hk_send);

        engine::steam_p2p_dispatch_fn = reinterpret_cast<engine::steam_p2p_dispatch_t>(engine::base() + engine::steam_p2p_recv_dispatch);

        utils::hook::attach(reinterpret_cast<void**>(&engine::steam_p2p_dispatch_fn), hk_dispatch);

        engine::steam_p2p_accept_fn = reinterpret_cast<engine::steam_p2p_accept_t>(engine::base() + engine::steam_p2p_accept);

        utils::hook::attach(reinterpret_cast<void**>(&engine::steam_p2p_accept_fn), hk_accept);

        T7_LOG(cx("p2p: steam p2p hooks installed (F8 toggles, default off, dedis unaffected)."));
    }

    void tick()
    {
        bool down = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;

        if (down && !toggle_latch)
        {
            toggle_latch = true;

            block_enabled = !block_enabled;

            if (block_enabled) T7_LOG(cx("p2p: enabled - steam p2p blocked (raw-udp dedis still work)."));
            else T7_LOG(cx("p2p: disabled - steam p2p restored."));
        }
        else if (!down)
        {
            toggle_latch = false;
        }
    }
}
