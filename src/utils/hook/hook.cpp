#include "hook.hpp"
#include "lde.hpp"
#include "../log/log.hpp"
#include "../crypt/crypt.hpp"

#include <cstring>

namespace utils::hook
{
    struct hook_record
    {
        uintptr_t target;

        void* trampoline;

        uint8_t original[24];

        int length;

        bool active;
    };

    static hook_record records[64];

    static int record_count = 0;

    static void* alloc_near(uintptr_t target, size_t size)
    {
        SYSTEM_INFO si;

        GetSystemInfo(&si);

        uintptr_t gran = si.dwAllocationGranularity;

        uintptr_t low = target > 0x40000000 ? target - 0x40000000 : gran;

        uintptr_t high = target + 0x40000000;

        uintptr_t aligned = target & ~(gran - 1);

        for (uintptr_t addr = aligned; addr >= low; addr -= gran)
        {
            MEMORY_BASIC_INFORMATION mbi;

            if (!VirtualQuery(reinterpret_cast<void*>(addr), &mbi, sizeof(mbi))) { break; }

            if (mbi.State == MEM_FREE)
            {
                void* block = VirtualAlloc(reinterpret_cast<void*>(addr), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

                if (block != nullptr) { return block; }
            }
        }

        for (uintptr_t addr = aligned + gran; addr <= high; addr += gran)
        {
            MEMORY_BASIC_INFORMATION mbi;

            if (!VirtualQuery(reinterpret_cast<void*>(addr), &mbi, sizeof(mbi))) { break; }

            if (mbi.State == MEM_FREE)
            {
                void* block = VirtualAlloc(reinterpret_cast<void*>(addr), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

                if (block != nullptr) { return block; }
            }
        }

        return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    }

    static void write_abs_jmp(uint8_t* dst, uint64_t destination)
    {
        dst[0] = 0xFF;

        dst[1] = 0x25;

        *reinterpret_cast<int32_t*>(dst + 2) = 0;

        *reinterpret_cast<uint64_t*>(dst + 6) = destination;
    }

    static bool fits_int32(int64_t value)
    {
        return value >= -2147483648LL && value <= 2147483647LL;
    }

    bool attach(void** original, void* detour)
    {
        uintptr_t target = reinterpret_cast<uintptr_t>(*original);

        struct stolen { int offset; lde::instruction ins; };

        stolen list[16];

        int count = 0;

        int copied = 0;

        while (copied < 14)
        {
            lde::instruction ins;

            if (!lde::decode(reinterpret_cast<const uint8_t*>(target + copied), ins) || count >= 16)
            {
                uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));

                T7_LOG(std::string(cx("hook: undecodable prologue at +")) + std::to_string(target - base) + cx("."));

                return false;
            }

            list[count].offset = copied;

            list[count].ins = ins;

            count++;

            copied += ins.length;
        }

        if (copied > 24) { return false; }

        void* tramp = alloc_near(target, 128);

        if (tramp == nullptr) { return false; }

        memcpy(tramp, reinterpret_cast<const void*>(target), copied);

        int64_t shift = static_cast<int64_t>(target) - static_cast<int64_t>(reinterpret_cast<uintptr_t>(tramp));

        for (int i = 0; i < count; i++)
        {
            int offset = list[i].offset;

            lde::instruction& ins = list[i].ins;

            if (ins.riprel_disp_offset != 0)
            {
                int32_t* slot = reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(tramp) + offset + ins.riprel_disp_offset);

                int64_t fixed = static_cast<int64_t>(*slot) + shift;

                if (!fits_int32(fixed)) { VirtualFree(tramp, 0, MEM_RELEASE); return false; }

                *slot = static_cast<int32_t>(fixed);
            }

            if (ins.rel_size == 4)
            {
                int32_t* slot = reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(tramp) + offset + ins.rel_offset);

                int64_t fixed = static_cast<int64_t>(*slot) + shift;

                if (!fits_int32(fixed)) { VirtualFree(tramp, 0, MEM_RELEASE); return false; }

                *slot = static_cast<int32_t>(fixed);
            }
            else if (ins.rel_size == 1)
            {
                VirtualFree(tramp, 0, MEM_RELEASE);

                return false;
            }
        }

        write_abs_jmp(reinterpret_cast<uint8_t*>(tramp) + copied, static_cast<uint64_t>(target + copied));

        DWORD tramp_old;

        VirtualProtect(tramp, 128, PAGE_EXECUTE_READ, &tramp_old);

        FlushInstructionCache(GetCurrentProcess(), tramp, 128);

        uint8_t saved[24];

        memcpy(saved, reinterpret_cast<const void*>(target), copied);

        uint8_t patch[14];

        write_abs_jmp(patch, reinterpret_cast<uint64_t>(detour));

        DWORD old;

        if (!VirtualProtect(reinterpret_cast<void*>(target), copied, PAGE_EXECUTE_READWRITE, &old))
        {
            VirtualFree(tramp, 0, MEM_RELEASE);

            return false;
        }

        memcpy(reinterpret_cast<void*>(target), patch, 14);

        for (int i = 14; i < copied; i++)
        {
            reinterpret_cast<uint8_t*>(target)[i] = 0x90;
        }

        VirtualProtect(reinterpret_cast<void*>(target), copied, old, &old);

        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(target), copied);

        if (record_count < 64)
        {
            hook_record& r = records[record_count++];

            r.target = target;

            r.trampoline = tramp;

            r.length = copied;

            r.active = true;

            memcpy(r.original, saved, copied);
        }

        *original = tramp;

        return true;
    }

    bool detach(void* target)
    {
        uintptr_t addr = reinterpret_cast<uintptr_t>(target);

        for (int i = 0; i < record_count; i++)
        {
            hook_record& r = records[i];

            if (!r.active || r.target != addr) { continue; }

            DWORD old;

            if (!VirtualProtect(reinterpret_cast<void*>(r.target), r.length, PAGE_EXECUTE_READWRITE, &old)) { return false; }

            memcpy(reinterpret_cast<void*>(r.target), r.original, r.length);

            VirtualProtect(reinterpret_cast<void*>(r.target), r.length, old, &old);

            FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(r.target), r.length);

            VirtualFree(r.trampoline, 0, MEM_RELEASE);

            r.active = false;

            return true;
        }

        return false;
    }
}
