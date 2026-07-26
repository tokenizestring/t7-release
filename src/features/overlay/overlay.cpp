#include "overlay.hpp"
#include "renderer.hpp"
#include "../../menu/menu.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <d3d11.h>
#include <mutex>
#include <vector>

#pragma comment(lib, "gdi32.lib")

using namespace overlay;

namespace features::overlay
{
    using present_t = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);

    using resize_t = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    using create_device_t = HRESULT(WINAPI*)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT, const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

    static present_t g_present_original = nullptr;

    static resize_t g_resize_original = nullptr;

    static ID3D11Device* g_device = nullptr;

    static ID3D11DeviceContext* g_context = nullptr;

    static ID3D11RenderTargetView* g_rtv = nullptr;

    static bool g_initialized = false;

    struct toast
    {
        std::string text;

        uint32_t accent;

        uint64_t spawn;

        uint32_t lifetime;
    };

    static std::mutex g_mutex;

    static std::vector<toast> g_toasts;

    static constexpr uint32_t max_toasts = 8;

    static constexpr uint32_t toast_lifetime = 4500;

    static constexpr float fade_in = 220.0f;

    static constexpr float fade_out = 340.0f;

    static uint32_t level_accent(level lvl)
    {
        if (lvl == level::good)
        {
            return renderer::rgba(80, 200, 120, 255);
        }

        if (lvl == level::warn)
        {
            return renderer::rgba(230, 180, 60, 255);
        }

        if (lvl == level::bad)
        {
            return renderer::rgba(225, 70, 70, 255);
        }

        return renderer::rgba(120, 90, 220, 255);
    }

    static uint32_t scale_alpha(uint32_t color, float a)
    {
        uint32_t alpha = (color >> 24) & 0xFF;

        alpha = static_cast<uint32_t>(alpha * a);

        return (color & 0x00FFFFFF) | (alpha << 24);
    }

    static float smoothstep(float t)
    {
        if (t < 0.0f)
        {
            t = 0.0f;
        }

        if (t > 1.0f)
        {
            t = 1.0f;
        }

        return t * t * (3.0f - 2.0f * t);
    }

    void notify(const std::string& text, level lvl)
    {
        if (!engine::notifications_enabled)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(g_mutex);

        uint64_t now = GetTickCount64();

        for (toast& existing : g_toasts)
        {
            if (existing.text == text)
            {
                existing.spawn = now;

                existing.accent = level_accent(lvl);

                return;
            }
        }

        toast entry;

        entry.text = text;

        entry.accent = level_accent(lvl);

        entry.spawn = now;

        entry.lifetime = toast_lifetime;

        g_toasts.insert(g_toasts.begin(), entry);

        if (g_toasts.size() > max_toasts)
        {
            g_toasts.pop_back();
        }
    }

    static void draw_toasts(float screen_w)
    {
        uint64_t now = GetTickCount64();

        std::vector<toast> snapshot;

        {
            std::lock_guard<std::mutex> lock(g_mutex);

            for (size_t i = 0; i < g_toasts.size();)
            {
                if (now - g_toasts[i].spawn >= g_toasts[i].lifetime)
                {
                    g_toasts.erase(g_toasts.begin() + i);
                }
                else
                {
                    i++;
                }
            }

            snapshot = g_toasts;
        }

        float margin = 20.0f;

        float gap = 8.0f;

        float pad_x = 12.0f;

        float pad_y = 8.0f;

        float bar = 4.0f;

        float y = margin;

        for (const toast& entry : snapshot)
        {
            uint64_t age = now - entry.spawn;

            float alpha = 1.0f;

            float slide = 0.0f;

            float text_w = renderer::text_width(entry.text.c_str());

            float height = renderer::line_height() + pad_y * 2.0f;

            float width = text_w + pad_x * 2.0f + bar;

            if (static_cast<float>(age) < fade_in)
            {
                float t = smoothstep(static_cast<float>(age) / fade_in);

                alpha = t;

                slide = (1.0f - t) * (width + margin);
            }
            else if (static_cast<float>(age) > entry.lifetime - fade_out)
            {
                alpha = smoothstep((entry.lifetime - static_cast<float>(age)) / fade_out);
            }

            float x = screen_w - margin - width + slide;

            uint32_t back = scale_alpha(renderer::rgba(14, 14, 17, 230), alpha);

            uint32_t accent = scale_alpha(entry.accent, alpha);

            uint32_t text = scale_alpha(renderer::rgba(238, 238, 244, 255), alpha);

            uint32_t shadow = scale_alpha(renderer::rgba(0, 0, 0, 200), alpha);

            renderer::draw_rect(x, y, width, height, back);

            renderer::draw_rect(x, y, bar, height, accent);

            renderer::draw_text_shadow(x + bar + pad_x, y + pad_y, entry.text.c_str(), text, shadow);

            y += height + gap;
        }
    }

    static bool ensure_target(IDXGISwapChain* swapchain, float& out_w, float& out_h)
    {
        DXGI_SWAP_CHAIN_DESC desc = {};

        if (FAILED(swapchain->GetDesc(&desc)))
        {
            return false;
        }

        out_w = static_cast<float>(desc.BufferDesc.Width);

        out_h = static_cast<float>(desc.BufferDesc.Height);

        if (g_rtv != nullptr)
        {
            return true;
        }

        ID3D11Texture2D* back_buffer = nullptr;

        if (FAILED(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer))))
        {
            return false;
        }

        HRESULT result = g_device->CreateRenderTargetView(back_buffer, nullptr, &g_rtv);

        back_buffer->Release();

        return SUCCEEDED(result);
    }

    static void render(IDXGISwapChain* swapchain)
    {
        if (!g_initialized)
        {
            if (FAILED(swapchain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g_device))))
            {
                return;
            }

            g_device->GetImmediateContext(&g_context);

            if (!renderer::init(g_device))
            {
                T7_LOG(cx("overlay: renderer init failed."));

                g_initialized = true;

                return;
            }

            g_initialized = true;

            T7_LOG(cx("overlay: hooked and drawing."));
        }

        if (!renderer::ready() || g_context == nullptr)
        {
            return;
        }

        float screen_w = 0.0f;

        float screen_h = 0.0f;

        if (!ensure_target(swapchain, screen_w, screen_h))
        {
            return;
        }

        renderer::begin(g_context, g_rtv, screen_w, screen_h);

        const char* watermark = "t7-release";

        uint32_t mark = renderer::rgba(200, 200, 210, 90);

        uint32_t mark_shadow = renderer::rgba(0, 0, 0, 90);

        renderer::draw_text_shadow(14.0f, screen_h - renderer::line_height() - 12.0f, watermark, mark, mark_shadow);

        draw_toasts(screen_w);

        menu::tick(screen_w, screen_h);

        renderer::end();
    }

    static HRESULT STDMETHODCALLTYPE hk_present(IDXGISwapChain* swapchain, UINT sync_interval, UINT flags)
    {
        render(swapchain);

        return g_present_original(swapchain, sync_interval, flags);
    }

    static HRESULT STDMETHODCALLTYPE hk_resize_buffers(IDXGISwapChain* swapchain, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
    {
        if (g_rtv != nullptr)
        {
            g_rtv->Release();

            g_rtv = nullptr;
        }

        return g_resize_original(swapchain, buffer_count, width, height, format, flags);
    }

    static void patch_vtable(void** vtable, int index, void* detour, void** original)
    {
        DWORD protect = 0;

        VirtualProtect(&vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &protect);

        *original = vtable[index];

        vtable[index] = detour;

        VirtualProtect(&vtable[index], sizeof(void*), protect, &protect);
    }

    void initialize()
    {
        HMODULE real = LoadLibraryW(L"C:\\Windows\\System32\\d3d11.dll");

        if (real == nullptr)
        {
            T7_LOG(cx("overlay: could not load system d3d11."));

            return;
        }

        create_device_t create = reinterpret_cast<create_device_t>(GetProcAddress(real, "D3D11CreateDeviceAndSwapChain"));

        if (create == nullptr)
        {
            T7_LOG(cx("overlay: no D3D11CreateDeviceAndSwapChain."));

            return;
        }

        HWND window = CreateWindowExA(0, "STATIC", "", WS_OVERLAPPED, 0, 0, 64, 64, nullptr, nullptr, nullptr, nullptr);

        if (window == nullptr)
        {
            return;
        }

        DXGI_SWAP_CHAIN_DESC desc = {};

        desc.BufferCount = 1;

        desc.BufferDesc.Width = 64;

        desc.BufferDesc.Height = 64;

        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        desc.BufferDesc.RefreshRate.Numerator = 60;

        desc.BufferDesc.RefreshRate.Denominator = 1;

        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

        desc.OutputWindow = window;

        desc.SampleDesc.Count = 1;

        desc.Windowed = TRUE;

        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;

        IDXGISwapChain* swapchain = nullptr;

        ID3D11Device* device = nullptr;

        ID3D11DeviceContext* context = nullptr;

        HRESULT result = create(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &level, 1, D3D11_SDK_VERSION, &desc, &swapchain, &device, nullptr, &context);

        if (FAILED(result) || swapchain == nullptr)
        {
            DestroyWindow(window);

            T7_LOG(cx("overlay: dummy swapchain creation failed."));

            return;
        }

        void** vtable = *reinterpret_cast<void***>(swapchain);

        patch_vtable(vtable, 8, hk_present, reinterpret_cast<void**>(&g_present_original));

        patch_vtable(vtable, 13, hk_resize_buffers, reinterpret_cast<void**>(&g_resize_original));

        swapchain->Release();

        context->Release();

        device->Release();

        DestroyWindow(window);

        notify(cx("t7-release loaded."), level::good);

        T7_LOG(cx("overlay: present hook installed."));
    }
}
