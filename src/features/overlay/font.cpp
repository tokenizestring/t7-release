#include "font.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <vector>

namespace overlay::font
{
    static constexpr char first_char = 32;

    static constexpr char last_char = 126;

    static constexpr uint32_t glyph_count = last_char - first_char + 1;

    static constexpr int font_height = 20;

    static constexpr int font_weight = 600;

    static constexpr uint32_t pad = 1;

    static constexpr uint32_t atlas_w = 512;

    static uint32_t g_atlas_h = 0;

    static std::vector<uint8_t> g_coverage;

    static glyph g_glyphs[glyph_count];

    static glyph g_white;

    static float g_line_height = 0.0f;

    static bool g_ready = false;

    const uint8_t* atlas_pixels()
    {
        return g_coverage.data();
    }

    uint32_t atlas_width()
    {
        return atlas_w;
    }

    uint32_t atlas_height()
    {
        return g_atlas_h;
    }

    const glyph& get(char c)
    {
        if (c < first_char || c > last_char)
        {
            return g_glyphs[0];
        }

        return g_glyphs[c - first_char];
    }

    const glyph& white_texel()
    {
        return g_white;
    }

    float line_height()
    {
        return g_line_height;
    }

    float text_width(const char* text)
    {
        float width = 0.0f;

        for (const char* p = text; *p != '\0'; p++)
        {
            width += get(*p).advance;
        }

        return width;
    }

    bool build()
    {
        if (g_ready)
        {
            return true;
        }

        HDC screen = GetDC(nullptr);

        HDC dc = CreateCompatibleDC(screen);

        ReleaseDC(nullptr, screen);

        if (dc == nullptr)
        {
            return false;
        }

        std::string face = cx("Segoe UI");

        HFONT hfont = CreateFontA(-font_height, 0, 0, 0, font_weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face.c_str());

        if (hfont == nullptr)
        {
            DeleteDC(dc);

            return false;
        }

        HGDIOBJ old_font = SelectObject(dc, hfont);

        TEXTMETRICA tm = {};

        GetTextMetricsA(dc, &tm);

        uint32_t row_h = tm.tmHeight;

        g_line_height = static_cast<float>(row_h);

        int widths[glyph_count] = {};

        uint32_t cursor_x = 2;

        uint32_t cursor_y = 0;

        struct cell { uint32_t x; uint32_t y; uint32_t w; };

        cell cells[glyph_count] = {};

        for (uint32_t i = 0; i < glyph_count; i++)
        {
            char ch = static_cast<char>(first_char + i);

            SIZE size = {};

            GetTextExtentPoint32A(dc, &ch, 1, &size);

            uint32_t cw = size.cx > 0 ? static_cast<uint32_t>(size.cx) : 1;

            widths[i] = cw;

            if (cursor_x + cw + pad > atlas_w)
            {
                cursor_x = 0;

                cursor_y += row_h + pad;
            }

            cells[i].x = cursor_x;

            cells[i].y = cursor_y;

            cells[i].w = cw;

            cursor_x += cw + pad;
        }

        g_atlas_h = cursor_y + row_h + pad;

        void* bits = nullptr;

        BITMAPINFO bmi = {};

        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

        bmi.bmiHeader.biWidth = static_cast<LONG>(atlas_w);

        bmi.bmiHeader.biHeight = -static_cast<LONG>(g_atlas_h);

        bmi.bmiHeader.biPlanes = 1;

        bmi.bmiHeader.biBitCount = 32;

        bmi.bmiHeader.biCompression = BI_RGB;

        HBITMAP dib = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);

        if (dib == nullptr || bits == nullptr)
        {
            SelectObject(dc, old_font);

            DeleteObject(hfont);

            DeleteDC(dc);

            return false;
        }

        HGDIOBJ old_bmp = SelectObject(dc, dib);

        SetBkMode(dc, TRANSPARENT);

        SetTextColor(dc, RGB(255, 255, 255));

        for (uint32_t i = 0; i < glyph_count; i++)
        {
            char ch = static_cast<char>(first_char + i);

            TextOutA(dc, static_cast<int>(cells[i].x), static_cast<int>(cells[i].y), &ch, 1);
        }

        GdiFlush();

        g_coverage.assign(static_cast<size_t>(atlas_w) * g_atlas_h, 0);

        const uint8_t* src = static_cast<const uint8_t*>(bits);

        for (uint32_t y = 0; y < g_atlas_h; y++)
        {
            for (uint32_t x = 0; x < atlas_w; x++)
            {
                g_coverage[static_cast<size_t>(y) * atlas_w + x] = src[(static_cast<size_t>(y) * atlas_w + x) * 4];
            }
        }

        g_coverage[0] = 255;

        g_coverage[1] = 255;

        g_coverage[atlas_w] = 255;

        g_coverage[atlas_w + 1] = 255;

        float inv_w = 1.0f / static_cast<float>(atlas_w);

        float inv_h = 1.0f / static_cast<float>(g_atlas_h);

        for (uint32_t i = 0; i < glyph_count; i++)
        {
            glyph& g = g_glyphs[i];

            g.u0 = cells[i].x * inv_w;

            g.v0 = cells[i].y * inv_h;

            g.u1 = (cells[i].x + cells[i].w) * inv_w;

            g.v1 = (cells[i].y + row_h) * inv_h;

            g.width = static_cast<float>(cells[i].w);

            g.height = static_cast<float>(row_h);

            g.advance = static_cast<float>(widths[i]);
        }

        g_white.u0 = 0.5f * inv_w;

        g_white.v0 = 0.5f * inv_h;

        g_white.u1 = 0.5f * inv_w;

        g_white.v1 = 0.5f * inv_h;

        g_white.width = 1.0f;

        g_white.height = 1.0f;

        g_white.advance = 1.0f;

        SelectObject(dc, old_bmp);

        SelectObject(dc, old_font);

        DeleteObject(dib);

        DeleteObject(hfont);

        DeleteDC(dc);

        g_ready = true;

        T7_LOG(std::string(cx("overlay/font: atlas built ")) + std::to_string(atlas_w) + "x" + std::to_string(g_atlas_h) + cx("."));

        return true;
    }
}
