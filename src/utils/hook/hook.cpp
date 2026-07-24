#include "hook.hpp"
#include "../../../lib/detours/src/detours.h"

namespace utils::hook
{
    bool attach(void** original, void* detour)
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(original, detour);

        LONG result = DetourTransactionCommit();

        return result == NO_ERROR;
    }
}
