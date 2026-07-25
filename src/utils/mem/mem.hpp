#pragma once

#include "../../stdafx.hpp"
#include "../log/log.hpp"
#include "../resource/resource.hpp"

#include <array>
#include <string>

namespace utils::mem
{
    uintptr_t get_process_base();

    void patch(void* address, const uint8_t* bytes, size_t size);

    template <size_t N>
    struct obfuscated_patch
    {
        std::array<uint8_t, N> data;

        static constexpr uint8_t key = 0x69;

        constexpr obfuscated_patch(const uint8_t(&input)[N]) : data{}
        {
            for (size_t i = 0; i < N; ++i)
            {
                data[i] = input[i] ^ key;
            }
        }

        void apply(void* address) const
        {
            uint8_t decoded[N];

            for (size_t i = 0; i < N; ++i)
            {
                decoded[i] = data[i] ^ key;
            }

            patch(address, decoded, N);

            memset(decoded, 0, N);
        }
    };

    template <size_t N>
    void apply_resource_patches(int resource_id, const char* name, const obfuscated_patch<N>& p)
    {
        const void* data = nullptr;

        size_t size = 0;

        if (utils::resource::load(resource_id, data, size))
        {
            auto offsets = static_cast<const uintptr_t*>(data);

            size_t count = size / sizeof(uintptr_t);

            uintptr_t base = get_process_base();

            T7_LOG("crc: patching " + std::string(name) + " (" + std::to_string(count) + " addresses).");

            for (size_t i = 0; i < count; i++)
            {
                p.apply(reinterpret_cast<void*>(base + offsets[i]));
            }
        }
    }
}
