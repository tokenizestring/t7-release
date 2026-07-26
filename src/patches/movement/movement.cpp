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

        // pmove is shared by client prediction AND the server sim of every player.
        // only quantize our own player (clientNum 0 at ps+0) so remote players
        // stay simulated vanilla and never rubber-band. on the host both the
        // predict and server paths are clientNum 0, so they quantize identically.
        int client_num = *reinterpret_cast<int*>(ps);

        if (client_num != 0)
        {
            return engine::pmove_fn(pmove);
        }

        int* server_time = reinterpret_cast<int*>(pmove + 8);

        int command_time = *reinterpret_cast<int*>(ps + 4);

        int real = *server_time;

        int elapsed = real - command_time;

        // big gap (spawn/teleport/level load) or nothing to do: let the engine
        // run its normal catch-up path, don't fold it into fixed ticks.
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

        // fixed-tick accumulator: simulate movement only in whole ~125Hz quanta
        // and carry the sub-tick remainder to the next frame. this makes wall-run,
        // step-up and collision behave identically at any fps (fixes wall-running
        // dropping and getting stuck on invisible barriers at high fps, where the
        // tiny per-frame frametime let fixed trace/collision epsilons dominate).
        int quantized = command_time + steps * tick;

        *server_time = quantized;

        int64_t result = engine::pmove_fn(pmove);

        // restore the real command time so the caller's scratch pmove is intact
        // and next frame's elapsed correctly includes the held remainder.
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
