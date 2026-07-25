#include "antiquit.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/mem/mem.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

namespace antiquit
{
    void initialize()
    {
        uintptr_t base = engine::base();

        if (base == 0)
        {
            return;
        }

        uint8_t nops[6] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

        utils::mem::patch(reinterpret_cast<void*>(base + engine::menu_blocked_jnz), nops, sizeof(nops));

        T7_LOG(cx("antiquit: patched menu-block branch."));
    }

    void tick() 
    {
        uintptr_t base = engine::base();

        if (base == 0)
        {
            return;
        }

        if (*reinterpret_cast<uint8_t*>(base + engine::client_active_flag) == 0)
        {
            return;
        }

        uintptr_t state = *reinterpret_cast<uintptr_t*>(base + engine::client_state_ptr);

        if (state == 0)
        {
            return;
        }

        int local = *reinterpret_cast<int*>(state);

        uintptr_t data_base = *reinterpret_cast<uintptr_t*>(base + engine::client_data_base);

        if (data_base == 0)
        {
            return;
        }

        engine::client_data_s* clients = reinterpret_cast<engine::client_data_s*>(data_base);

        volatile uint32_t* flags = reinterpret_cast<volatile uint32_t*>(&clients[local].match_flags);

        if (*flags != 0)
        {
            *flags = 0;
        }
    }
}
