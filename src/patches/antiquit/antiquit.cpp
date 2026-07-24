#include "antiquit.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/mem/mem.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

static constexpr size_t client_data_stride = 0x342720;

static constexpr size_t matchflags_offset = 0x18;

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

        T7_LOG(cx("antiquit: patched menu-block branch"));
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

        uintptr_t client_data = data_base + client_data_stride * static_cast<size_t>(local);

        volatile uint32_t* flags = reinterpret_cast<volatile uint32_t*>(client_data + matchflags_offset);

        if (*flags != 0)
        {
            *flags = 0;
        }
    }
}
