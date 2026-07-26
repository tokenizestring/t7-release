#include "movement.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <cstdint>

namespace movement
{
    static int64_t __fastcall hk_pmove(int64_t pmove)
    {
        if (!engine::movement_tick_enabled)
        {
            return engine::pmove_fn(pmove);
        }

        int64_t ps = *reinterpret_cast<int64_t*>(pmove);

        if (ps == 0)
        {
            return engine::pmove_fn(pmove);
        }

        int client_num = *reinterpret_cast<int*>(ps);

        if (client_num != 0)
        {
            return engine::pmove_fn(pmove);
        }

        int* server_time = reinterpret_cast<int*>(pmove + 8);

        int command_time = *reinterpret_cast<int*>(ps + 4);

        int real = *server_time;

        int elapsed = real - command_time;

        if (elapsed <= 0 || elapsed > 200)
        {
            return engine::pmove_fn(pmove);
        }

        int tick = engine::movement_tick_ms;

        if (tick < 1)
        {
            tick = 1;
        }

        int steps = elapsed / tick;

        int quantized = command_time + steps * tick;

        *server_time = quantized;

        int64_t result = engine::pmove_fn(pmove);

        *server_time = real;

        return result;
    }

    void initialize()
    {
        engine::pmove_fn = reinterpret_cast<engine::pmove_t>(engine::base() + engine::pmove);

        utils::hook::attach(reinterpret_cast<void**>(&engine::pmove_fn), hk_pmove);

        T7_LOG(cx("movement: fixed-tick accumulator installed (default off)."));
    }
}
