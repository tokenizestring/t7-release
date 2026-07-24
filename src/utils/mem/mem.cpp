#include "mem.hpp"

namespace utils::mem
{
    uintptr_t get_process_base()
    {
        return reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    }

    void patch(void* address, const uint8_t* bytes, size_t size)
    {
        DWORD old_protect;

        if (VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &old_protect))
        {
            for (size_t i = 0; i < size; i++)
            {
                static_cast<uint8_t*>(address)[i] = bytes[i];
            }

            FlushInstructionCache(GetCurrentProcess(), address, size);

            DWORD dummy;

            VirtualProtect(address, size, old_protect, &dummy);
        }
    }
}
