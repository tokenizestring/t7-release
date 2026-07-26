#include "callvote.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"
#include "../../features/overlay/overlay.hpp"

#include <cstring>

namespace callvote
{
    static constexpr size_t max_vote_length = 128;

    static void strip_markup(char* dst, const char* src, size_t dst_size)
    {
        size_t w = 0;

        for (size_t r = 0; src[r] != 0 && w < dst_size - 1; r++)
        {
            if (src[r] == 0x5E && src[r + 1] != 0)
            {
                char code = src[r + 1];

                if (code >= 0x30 && code <= 0x39)
                {
                    if (w + 2 < dst_size)
                    {
                        dst[w++] = src[r];

                        dst[w++] = src[r + 1];
                    }

                    r++;

                    continue;
                }

                r++;

                continue;
            }

            if (src[r] == 0x24 && src[r + 1] == 0x28)
            {
                while (src[r] != 0 && src[r] != 0x29)
                {
                    r++;
                }

                continue;
            }

            dst[w++] = src[r];
        }

        dst[w] = 0;
    }

    static void __fastcall hk_vote_update(int client, const char* vote_string)
    {
        size_t length = vote_string != nullptr ? std::strlen(vote_string) : 0;

        T7_LOG(std::string(cx("callvote: vote string '")) + std::string(vote_string != nullptr ? vote_string : "", length < 200 ? length : 200) + cx("' len ") + std::to_string(length) + cx("."));

        if (engine::protection.vote_strings && length >= max_vote_length)
        {
            T7_LOG(cx("callvote: oversized vote string dropped."));

            features::overlay::notify(cx("blocked crash attempt."), features::overlay::level::bad);

            return;
        }

        char clean[256] = {};

        strip_markup(clean, vote_string, sizeof(clean)); 

        engine::vote_update(client, clean);
    }

    void initialize()
    {
        engine::vote_update = reinterpret_cast<engine::vote_update_t>(engine::base() + engine::vote_string_update);

        utils::hook::attach(reinterpret_cast<void**>(&engine::vote_update), hk_vote_update);

        T7_LOG(cx("callvote: crash protection installed."));
    }
}
