#include "markup.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"
#include "../../features/overlay/overlay.hpp"

#include <cstdint>
#include <cctype>

namespace markup
{
    static bool enforce = true;

    static char safe_null = 0;

    static void notify_malicious_text()
    {
        features::overlay::notify(cx("blocked malicious text."), features::overlay::level::bad);
    }

    static void notify_crash()
    {
        features::overlay::notify(cx("blocked crash attempt."), features::overlay::level::bad);
    }

    static bool is_safe_embed_char(char c)
    {
        unsigned char u = static_cast<unsigned char>(c);

        return u >= 0x20 && u < 0x7F;
    }

    static constexpr int interp_max_key = 63;

    static bool defuse_interp(char* p, bool& hit)
    {
        int key_len = 0;

        bool closed = false;

        while (key_len < 0x100 && p[2 + key_len] != 0)
        {
            if (p[2 + key_len] == 0x29)
            {
                closed = true;

                break;
            }

            key_len++;
        }

        if (!closed || key_len == 0 || key_len > interp_max_key)
        {
            p[0] = 0x20;

            hit = true;
        }

        return true;
    }

    static bool defuse(char* p)
    {
        if (p == nullptr)
        {
            return false;
        }

        bool hit = false;

        int guard = 0;

        while (*p != 0 && guard < 0x4000)
        {
            guard++;

            if (*p == 0x24 && p[1] == 0x28)
            {
                defuse_interp(p, hit);

                p++;

                continue;
            }

            if (*p != 0x5E)
            {
                p++;

                continue;
            }

            char letter = p[1];

            if (letter == 0)
            {
                return hit;
            }

            if (letter == 0x48 || letter == 0x49)
            {
                if (p[2] == 0 || p[3] == 0 || p[4] == 0)
                {
                    p[1] = 0x37;

                    hit = true;

                    p += 2;

                    continue;
                }

                uint8_t length = static_cast<uint8_t>(p[4]);

                if (length >= 0x80 || length == 0)
                {
                    p[1] = 0x37;

                    hit = true;

                    p += 2;

                    continue;
                }

                uint8_t available = 0;

                bool clean = true;

                while (available < length && p[5 + available] != 0)
                {
                    if (!is_safe_embed_char(p[5 + available]))
                    {
                        clean = false;
                    }

                    available++;
                }

                if (available < length || !clean)
                {
                    p[1] = 0x37;

                    hit = true;
                }

                p += 2;

                continue;
            }

            if (letter == 0x42)
            {
                bool closed = false;

                for (int n = 0; n < 63; n++)
                {
                    char c = p[2 + n];

                    if (c == 0)
                    {
                        break;
                    }

                    if (c == 0x5E)
                    {
                        closed = true;

                        break;
                    }
                }

                if (!closed)
                {
                    p[1] = 0x37;

                    hit = true;
                }

                p += 2;

                continue;
            }

            p += 2;
        }

        return hit;
    }

    static bool guarded_defuse(char* p)
    {
        __try
        {
            return defuse(p);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    static char __fastcall hk_parse_a(char** cursor, int64_t a2, void* a3, void* a4, int* a5, int* a6)
    {
        if (engine::protection.markup_text && cursor != nullptr)
        {
            if (guarded_defuse(*cursor))
            {
                notify_malicious_text();
            }
        }

        __try
        {
            return engine::markup_parse_a_fn(cursor, a2, a3, a4, a5, a6);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            if (cursor != nullptr)
            {
                *cursor = &safe_null;
            }

            notify_crash();

            return 0;
        }
    }

    static char __fastcall hk_parse_b(uint32_t a1, int64_t cursor, void* a3, int64_t a4, int64_t a5, void* a6, int64_t* a7, int* a8)
    {
        if (engine::protection.markup_text && cursor != 0)
        {
            if (guarded_defuse(*reinterpret_cast<char**>(cursor)))
            {
                notify_malicious_text();
            }
        }

        __try
        {
            return engine::markup_parse_b_fn(a1, cursor, a3, a4, a5, a6, a7, a8);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            if (cursor != 0)
            {
                *reinterpret_cast<char**>(cursor) = &safe_null;
            }

            notify_crash();

            return 0;
        }
    }

    static char __fastcall hk_interp(uint32_t a1, int64_t a2, void* a3, int64_t out, uint32_t out_size)
    {
        __try
        {
            return engine::markup_interp_fn(a1, a2, a3, out, out_size);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            if (out != 0 && out_size != 0)
            {
                *reinterpret_cast<char*>(out) = 0;
            }

            notify_crash();

            return 0;
        }
    }

    void initialize()
    {
        engine::markup_parse_a_fn = reinterpret_cast<engine::markup_parse_a_t>(engine::base() + engine::markup_parse_a);

        utils::hook::attach(reinterpret_cast<void**>(&engine::markup_parse_a_fn), hk_parse_a);

        engine::markup_parse_b_fn = reinterpret_cast<engine::markup_parse_b_t>(engine::base() + engine::markup_parse_b);

        utils::hook::attach(reinterpret_cast<void**>(&engine::markup_parse_b_fn), hk_parse_b);

        engine::markup_interp_fn = reinterpret_cast<engine::markup_interp_t>(engine::base() + engine::markup_interp);

        utils::hook::attach(reinterpret_cast<void**>(&engine::markup_interp_fn), hk_interp);

        T7_LOG(cx("markup: text crash protection installed."));
    }
}
