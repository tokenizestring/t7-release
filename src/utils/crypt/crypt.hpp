#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace utils::crypt
{
    namespace detail
    {
        struct rc4_state
        {
            uint8_t s[256];
            uint8_t i;
            uint8_t j;
        };

        static constexpr rc4_state rc4_init(uint64_t seed)
        {
            rc4_state state = {};

            uint8_t key[8] = {};
            for (int k = 0; k < 8; k++)
            {
                key[k] = static_cast<uint8_t>(seed >> (k * 8));
            }

            for (int n = 0; n < 256; n++)
            {
                state.s[n] = static_cast<uint8_t>(n);
            }

            uint8_t j = 0;
            for (int n = 0; n < 256; n++)
            {
                j = j + state.s[n] + key[n % 8];
                uint8_t tmp = state.s[n];
                state.s[n] = state.s[j];
                state.s[j] = tmp;
            }

            state.i = 0;
            state.j = 0;
            return state;
        }

        static constexpr uint8_t rc4_byte(rc4_state& state)
        {
            state.i = state.i + 1;
            state.j = state.j + state.s[state.i];
            uint8_t tmp = state.s[state.i];
            state.s[state.i] = state.s[state.j];
            state.s[state.j] = tmp;
            return state.s[static_cast<uint8_t>(state.s[state.i] + state.s[state.j])];
        }

        static constexpr uint64_t make_seed(uint64_t counter)
        {
            uint64_t h = counter ^ 0x517cc1b727220a95ULL;
            h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ULL;
            h = (h ^ (h >> 27)) * 0x94d049bb133111ebULL;
            h = h ^ (h >> 31);
            return h;
        }
    }

    template <size_t N, uint64_t Seed>
    struct encrypted_string
    {
        std::array<uint8_t, N> data;
        static constexpr uint64_t seed = Seed;

        constexpr encrypted_string(const char(&input)[N]) : data{}
        {
            detail::rc4_state state = detail::rc4_init(seed);

            for (size_t drop = 0; drop < 256; drop++)
            {
                detail::rc4_byte(state);
            }

            for (size_t i = 0; i < N; i++)
            {
                data[i] = static_cast<uint8_t>(input[i]) ^ detail::rc4_byte(state);
            }
        }

        void reveal(char* out) const
        {
            detail::rc4_state state = detail::rc4_init(seed);

            for (size_t drop = 0; drop < 256; drop++)
            {
                detail::rc4_byte(state);
            }

            for (size_t i = 0; i < N; i++)
            {
                out[i] = static_cast<char>(data[i] ^ detail::rc4_byte(state));
            }
        }

        static constexpr size_t size()
        {
            return N;
        }
    };
}

#define cx(str) ([]() -> std::string { \
    static constexpr ::utils::crypt::encrypted_string<sizeof(str), ::utils::crypt::detail::make_seed(__COUNTER__)> e(str); \
    char buf[sizeof(str)]; \
    e.reveal(buf); \
    std::string r(buf, sizeof(str) - 1); \
    volatile char* p = buf; \
    for (size_t i = 0; i < sizeof(str); i++) p[i] = 0; \
    return r; \
}())
