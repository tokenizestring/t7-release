#include "menulogo.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/resource/resource.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"
#include "../../resources/resources.hpp"

#include <d3d11.h>
#include <wincodec.h>
#include <vector>
#include <algorithm>

#pragma comment(lib, "windowscodecs.lib")

static constexpr size_t max_frames = 256;

static constexpr uint32_t default_delay = 66;

struct frame_data
{
    std::vector<uint8_t> pixels;

    uint32_t width;

    uint32_t height;

    uint32_t delay_ms;
};

static std::vector<frame_data> g_raw;

static std::vector<ID3D11ShaderResourceView*> g_views;

static std::vector<uint32_t> g_delays;

static ID3D11Device* g_device = nullptr;

static uint32_t g_width = 0;

static uint32_t g_height = 0;

static size_t g_current = 0;

static uint64_t g_next_switch = 0;

static bool com_begin(bool& uninit)
{
    HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    uninit = SUCCEEDED(com);

    return SUCCEEDED(com) || com == RPC_E_CHANGED_MODE;
}

static uint32_t meta_uint(IWICMetadataQueryReader* reader, const wchar_t* key, uint32_t fallback)
{
    if (reader == nullptr)
    {
        return fallback;
    }

    PROPVARIANT var;

    PropVariantInit(&var);

    uint32_t value = fallback;

    if (SUCCEEDED(reader->GetMetadataByName(key, &var)))
    {
        if (var.vt == VT_UI2)
        {
            value = var.uiVal;
        }
        else if (var.vt == VT_UI1)
        {
            value = var.bVal;
        }
        else if (var.vt == VT_UI4)
        {
            value = var.ulVal;
        }
    }

    PropVariantClear(&var);

    return value;
}

static bool decode_frame(IWICImagingFactory* factory, IWICBitmapFrameDecode* frame, std::vector<uint8_t>& pixels, uint32_t& width, uint32_t& height)
{
    IWICFormatConverter* converter = nullptr;

    if (FAILED(factory->CreateFormatConverter(&converter)))
    {
        return false;
    }

    bool ok = SUCCEEDED(converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom));

    if (ok)
    {
        ok = SUCCEEDED(converter->GetSize(&width, &height));
    }

    if (ok && width != 0 && height != 0)
    {
        pixels.resize(static_cast<size_t>(width) * height * 4);

        ok = SUCCEEDED(converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(pixels.size()), pixels.data()));
    }
    else
    {
        ok = false;
    }

    converter->Release();

    return ok;
}

static bool load_gif(IWICImagingFactory* factory, IWICBitmapDecoder* decoder, std::vector<frame_data>& out)
{
    UINT total = 0;

    if (FAILED(decoder->GetFrameCount(&total)) || total == 0)
    {
        return false;
    }

    IWICMetadataQueryReader* global = nullptr;

    decoder->GetMetadataQueryReader(&global);

    uint32_t canvas_w = meta_uint(global, L"/logscrdesc/Width", 0);

    uint32_t canvas_h = meta_uint(global, L"/logscrdesc/Height", 0);

    if (global != nullptr)
    {
        global->Release();
    }

    if (total > max_frames)
    {
        total = static_cast<UINT>(max_frames);
    }

    std::vector<uint8_t> canvas;

    std::vector<uint8_t> previous;

    for (UINT i = 0; i < total; i++)
    {
        IWICBitmapFrameDecode* frame = nullptr;

        if (FAILED(decoder->GetFrame(i, &frame)))
        {
            return !out.empty();
        }

        IWICMetadataQueryReader* meta = nullptr;

        frame->GetMetadataQueryReader(&meta);

        uint32_t left = meta_uint(meta, L"/imgdesc/Left", 0);

        uint32_t top = meta_uint(meta, L"/imgdesc/Top", 0);

        uint32_t delay = meta_uint(meta, L"/grctlext/Delay", 0);

        uint32_t disposal = meta_uint(meta, L"/grctlext/Disposal", 0);

        if (meta != nullptr)
        {
            meta->Release();
        }

        std::vector<uint8_t> pixels;

        uint32_t fw = 0;

        uint32_t fh = 0;

        bool ok = decode_frame(factory, frame, pixels, fw, fh);

        frame->Release();

        if (!ok)
        {
            return !out.empty();
        }

        if (canvas_w == 0 || canvas_h == 0)
        {
            canvas_w = fw;

            canvas_h = fh;
        }

        if (canvas.empty())
        {
            canvas.assign(static_cast<size_t>(canvas_w) * canvas_h * 4, 0);
        }

        if (disposal == 3)
        {
            previous = canvas;
        }

        for (uint32_t y = 0; y < fh; y++)
        {
            uint32_t cy = top + y;

            if (cy >= canvas_h)
            {
                break;
            }

            for (uint32_t x = 0; x < fw; x++)
            {
                uint32_t cx = left + x;

                if (cx >= canvas_w)
                {
                    break;
                }

                const uint8_t* src = &pixels[(static_cast<size_t>(y) * fw + x) * 4];

                if (src[3] == 0)
                {
                    continue;
                }

                uint8_t* dst = &canvas[(static_cast<size_t>(cy) * canvas_w + cx) * 4];

                dst[0] = src[0];

                dst[1] = src[1];

                dst[2] = src[2];

                dst[3] = src[3];
            }
        }

        frame_data fd;

        fd.pixels = canvas;

        fd.width = canvas_w;

        fd.height = canvas_h;

        fd.delay_ms = delay != 0 ? delay * 10 : 0;

        out.push_back(std::move(fd));

        if (disposal == 2)
        {
            std::fill(canvas.begin(), canvas.end(), static_cast<uint8_t>(0));
        }
        else if (disposal == 3 && !previous.empty())
        {
            canvas = previous;
        }
    }

    return !out.empty();
}

static ID3D11ShaderResourceView* create_srv(ID3D11Device* device, const frame_data& frame)
{
    D3D11_TEXTURE2D_DESC desc = {};

    desc.Width = frame.width;

    desc.Height = frame.height;

    desc.MipLevels = 1;

    desc.ArraySize = 1;

    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    desc.SampleDesc.Count = 1;

    desc.Usage = D3D11_USAGE_DEFAULT;

    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initial = {};

    initial.pSysMem = frame.pixels.data();

    initial.SysMemPitch = frame.width * 4;

    ID3D11Texture2D* created = nullptr;

    if (FAILED(device->CreateTexture2D(&desc, &initial, &created)))
    {
        return nullptr;
    }

    ID3D11ShaderResourceView* view = nullptr;

    HRESULT result = device->CreateShaderResourceView(created, nullptr, &view);

    created->Release();

    if (FAILED(result))
    {
        return nullptr;
    }

    return view;
}

static bool decode_embedded()
{
    const void* data = nullptr;

    size_t size = 0;

    if (!utils::resource::load(RES_MENU_LOGO, data, size) || data == nullptr || size == 0)
    {
        return false;
    }

    bool uninit = false;

    if (!com_begin(uninit))
    {
        return false;
    }

    IWICImagingFactory* factory = nullptr;

    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
    {
        if (uninit)
        {
            CoUninitialize();
        }

        return false;
    }

    IWICStream* stream = nullptr;

    bool ok = SUCCEEDED(factory->CreateStream(&stream));

    if (ok)
    {
        ok = SUCCEEDED(stream->InitializeFromMemory(const_cast<BYTE*>(static_cast<const BYTE*>(data)), static_cast<DWORD>(size)));
    }

    IWICBitmapDecoder* decoder = nullptr;

    if (ok)
    {
        ok = SUCCEEDED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder));
    }

    if (ok)
    {
        load_gif(factory, decoder, g_raw);
    }

    if (decoder != nullptr)
    {
        decoder->Release();
    }

    if (stream != nullptr)
    {
        stream->Release();
    }

    factory->Release();

    if (uninit)
    {
        CoUninitialize();
    }

    return !g_raw.empty();
}

static bool ensure_uploaded()
{
    if (!g_views.empty())
    {
        return true;
    }

    if (g_raw.empty())
    {
        return false;
    }

    ID3D11Device* device = engine::d3d_device();

    if (device == nullptr)
    {
        return false;
    }

    for (const frame_data& frame : g_raw)
    {
        ID3D11ShaderResourceView* view = create_srv(device, frame);

        if (view == nullptr)
        {
            continue;
        }

        g_views.push_back(view);

        g_delays.push_back(frame.delay_ms != 0 ? frame.delay_ms : default_delay);
    }

    if (g_views.empty())
    {
        return false;
    }

    g_device = device;

    g_width = g_raw[0].width;

    g_height = g_raw[0].height;

    g_current = 0;

    g_next_switch = GetTickCount64() + g_delays[0];

    T7_LOG(std::string(cx("menulogo: uploaded ")) + std::to_string(g_views.size()) + cx(" frames at ") + std::to_string(g_width) + "x" + std::to_string(g_height) + cx("."));

    return true;
}

namespace features::menulogo
{
    void initialize()
    {
        if (!decode_embedded())
        {
            T7_LOG(cx("menulogo: failed to decode embedded logo."));

            return;
        }

        T7_LOG(std::string(cx("menulogo: decoded logo, ")) + std::to_string(g_raw.size()) + cx(" frames."));
    }

    void tick()
    {
        if (!ensure_uploaded())
        {
            return;
        }

        if (g_views.size() < 2)
        {
            return;
        }

        uint64_t now = GetTickCount64();

        if (now < g_next_switch)
        {
            return;
        }

        g_current = (g_current + 1) % g_views.size();

        g_next_switch = now + g_delays[g_current];
    }

    ID3D11ShaderResourceView* current_srv()
    {
        if (g_views.empty() || g_current >= g_views.size())
        {
            return nullptr;
        }

        return g_views[g_current];
    }

    uint32_t width()
    {
        return g_width;
    }

    uint32_t height()
    {
        return g_height;
    }
}
