#include "lde.hpp"

namespace utils::hook::lde
{
    namespace
    {
        enum imm : uint8_t
        {
            imm_none = 0,
            imm_ib = 1,
            imm_iw = 2,
            imm_id = 3,
            imm_iz = 4,
            imm_iv = 5,
            imm_rel8 = 6,
            imm_rel32 = 7,
            imm_moffs = 8,
            imm_grp3 = 9,
            imm_enter = 10,
        };

        constexpr uint8_t has_modrm = 0x10;

        constexpr uint8_t invalid = 0x80;

        uint8_t one_byte[256];

        uint8_t two_byte[256];

        bool ready = false;

        void build()
        {
            for (int i = 0; i < 256; i++)
            {
                one_byte[i] = imm_none;

                two_byte[i] = has_modrm;
            }

            for (int g = 0; g < 8; g++)
            {
                int b = g * 8;

                one_byte[b + 0] = has_modrm;

                one_byte[b + 1] = has_modrm;

                one_byte[b + 2] = has_modrm;

                one_byte[b + 3] = has_modrm;

                one_byte[b + 4] = imm_ib;

                one_byte[b + 5] = imm_iz;
            }

            one_byte[0x63] = has_modrm;

            one_byte[0x68] = imm_iz;

            one_byte[0x69] = has_modrm | imm_iz;

            one_byte[0x6A] = imm_ib;

            one_byte[0x6B] = has_modrm | imm_ib;

            for (int i = 0x70; i <= 0x7F; i++)
            {
                one_byte[i] = imm_rel8;
            }

            one_byte[0x80] = has_modrm | imm_ib;

            one_byte[0x81] = has_modrm | imm_iz;

            one_byte[0x82] = invalid;

            one_byte[0x83] = has_modrm | imm_ib;

            for (int i = 0x84; i <= 0x8F; i++)
            {
                one_byte[i] = has_modrm;
            }

            one_byte[0x9A] = invalid;

            for (int i = 0xA0; i <= 0xA3; i++)
            {
                one_byte[i] = imm_moffs;
            }

            one_byte[0xA8] = imm_ib;

            one_byte[0xA9] = imm_iz;

            for (int i = 0xB0; i <= 0xB7; i++)
            {
                one_byte[i] = imm_ib;
            }

            for (int i = 0xB8; i <= 0xBF; i++)
            {
                one_byte[i] = imm_iv;
            }

            one_byte[0xC0] = has_modrm | imm_ib;

            one_byte[0xC1] = has_modrm | imm_ib;

            one_byte[0xC2] = imm_iw;

            one_byte[0xC3] = imm_none;

            one_byte[0xC4] = invalid;

            one_byte[0xC5] = invalid;

            one_byte[0xC6] = has_modrm | imm_ib;

            one_byte[0xC7] = has_modrm | imm_iz;

            one_byte[0xC8] = imm_enter;

            one_byte[0xCA] = imm_iw;

            one_byte[0xCD] = imm_ib;

            one_byte[0xCE] = invalid;

            one_byte[0xD0] = has_modrm;

            one_byte[0xD1] = has_modrm;

            one_byte[0xD2] = has_modrm;

            one_byte[0xD3] = has_modrm;

            one_byte[0xD4] = invalid;

            one_byte[0xD5] = invalid;

            one_byte[0xD6] = invalid;

            for (int i = 0xD8; i <= 0xDF; i++)
            {
                one_byte[i] = has_modrm;
            }

            for (int i = 0xE0; i <= 0xE3; i++)
            {
                one_byte[i] = imm_rel8;
            }

            one_byte[0xE4] = imm_ib;

            one_byte[0xE5] = imm_ib;

            one_byte[0xE6] = imm_ib;

            one_byte[0xE7] = imm_ib;

            one_byte[0xE8] = imm_rel32;

            one_byte[0xE9] = imm_rel32;

            one_byte[0xEA] = invalid;

            one_byte[0xEB] = imm_rel8;

            one_byte[0xF6] = has_modrm | imm_grp3;

            one_byte[0xF7] = has_modrm | imm_grp3;

            one_byte[0xFE] = has_modrm;

            one_byte[0xFF] = has_modrm;

            int invalid_ops[] = { 0x06, 0x07, 0x0E, 0x16, 0x17, 0x1E, 0x1F, 0x27, 0x2F, 0x37, 0x3F, 0x60, 0x61, 0x62 };

            for (int x : invalid_ops)
            {
                one_byte[x] = invalid;
            }

            int no_modrm[] = { 0x05, 0x06, 0x07, 0x08, 0x09, 0x0B, 0x0E, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x77, 0xA0, 0xA1, 0xA2, 0xA8, 0xA9, 0xAA };

            for (int x : no_modrm)
            {
                two_byte[x] = imm_none;
            }

            for (int i = 0xC8; i <= 0xCF; i++)
            {
                two_byte[i] = imm_none;
            }

            for (int i = 0x80; i <= 0x8F; i++)
            {
                two_byte[i] = imm_rel32;
            }

            int modrm_ib[] = { 0x70, 0x71, 0x72, 0x73, 0xA4, 0xAC, 0xBA, 0xC2, 0xC4, 0xC5, 0xC6, 0x0F };

            for (int x : modrm_ib)
            {
                two_byte[x] = has_modrm | imm_ib;
            }

            ready = true;
        }
    }

    bool decode(const uint8_t* start, instruction& out)
    {
        if (!ready) { build(); }

        out.length = 0;

        out.riprel_disp_offset = 0;

        out.rel_offset = 0;

        out.rel_size = 0;

        const uint8_t* p = start;

        bool opsize = false;

        bool addrsize = false;

        bool rexw = false;

        for (;;)
        {
            uint8_t b = *p;

            if (b == 0x66) { opsize = true; p++; continue; }

            if (b == 0x67) { addrsize = true; p++; continue; }

            if (b == 0xF0 || b == 0xF2 || b == 0xF3) { p++; continue; }

            if (b == 0x2E || b == 0x36 || b == 0x3E || b == 0x26 || b == 0x64 || b == 0x65) { p++; continue; }

            break;
        }

        if (*p >= 0x40 && *p <= 0x4F)
        {
            if (*p & 0x08) { rexw = true; }

            p++;
        }

        uint8_t opcode = *p++;

        uint8_t flags;

        if (opcode == 0x0F)
        {
            uint8_t second = *p++;

            if (second == 0x38) { p++; flags = has_modrm; }
            else if (second == 0x3A) { p++; flags = has_modrm | imm_ib; }
            else { flags = two_byte[second]; }
        }
        else
        {
            flags = one_byte[opcode];
        }

        if (flags & invalid) { return false; }

        int code = flags & 0x0F;

        if (flags & has_modrm)
        {
            uint8_t modrm = *p++;

            uint8_t mod = modrm >> 6;

            uint8_t rm = modrm & 7;

            bool sib = mod != 3 && rm == 4;

            uint8_t sib_base = 0;

            if (sib) { sib_base = *p & 7; p++; }

            int disp = 0;

            if (mod == 0)
            {
                if (!sib && rm == 5) { disp = 4; out.riprel_disp_offset = static_cast<int>(p - start); }
                else if (sib && sib_base == 5) { disp = 4; }
            }
            else if (mod == 1) { disp = 1; }
            else if (mod == 2) { disp = 4; }

            p += disp;

            if (code == imm_grp3)
            {
                uint8_t reg = (modrm >> 3) & 7;

                if (reg == 0 || reg == 1)
                {
                    p += opcode == 0xF6 ? 1 : (opsize ? 2 : 4);
                }
            }
        }

        switch (code)
        {
            case imm_ib: p += 1; break;

            case imm_iw: p += 2; break;

            case imm_id: p += 4; break;

            case imm_iz: p += opsize ? 2 : 4; break;

            case imm_iv: p += rexw ? 8 : (opsize ? 2 : 4); break;

            case imm_rel8: out.rel_offset = static_cast<int>(p - start); out.rel_size = 1; p += 1; break;

            case imm_rel32: out.rel_offset = static_cast<int>(p - start); out.rel_size = 4; p += 4; break;

            case imm_moffs: p += addrsize ? 4 : 8; break;

            case imm_enter: p += 3; break;

            default: break;
        }

        out.length = static_cast<int>(p - start);

        return out.length > 0 && out.length <= 15;
    }
}
