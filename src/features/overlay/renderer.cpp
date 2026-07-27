#include "renderer.hpp"
#include "font.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <d3dcompiler.h>
#include <vector>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

namespace overlay::renderer
{
    struct vertex
    {
        float x;

        float y;

        float u;

        float v;

        uint32_t color;
    };

    struct constants
    {
        float screen[4];
    };

    static ID3D11Device* g_device = nullptr;

    static ID3D11DeviceContext* g_context = nullptr;

    static ID3D11RenderTargetView* g_target = nullptr;

    static ID3D11VertexShader* g_vs = nullptr;

    static ID3D11PixelShader* g_ps = nullptr;

    static ID3D11PixelShader* g_ps_image = nullptr;

    static ID3D11InputLayout* g_layout = nullptr;

    static ID3D11Buffer* g_vertex_buffer = nullptr;

    static ID3D11Buffer* g_constant_buffer = nullptr;

    static ID3D11BlendState* g_blend = nullptr;

    static ID3D11SamplerState* g_sampler = nullptr;

    static ID3D11RasterizerState* g_raster = nullptr;

    static ID3D11DepthStencilState* g_depth = nullptr;

    static ID3D11ShaderResourceView* g_atlas = nullptr;

    static uint32_t g_vertex_capacity = 0;

    static std::vector<vertex> g_vertices;

    struct image_command
    {
        float x;

        float y;

        float w;

        float h;

        ID3D11ShaderResourceView* srv;
    };

    static std::vector<image_command> g_images;

    static float g_screen_w = 0.0f;

    static float g_screen_h = 0.0f;

    static bool g_ready = false;

    static const char* shader_source =
        "cbuffer cb : register(b0) { float4 screen; }\n"
        "struct vs_in { float2 pos : POSITION; float2 uv : TEXCOORD0; float4 col : COLOR0; };\n"
        "struct ps_in { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; float4 col : COLOR0; };\n"
        "ps_in vs_main(vs_in i) {\n"
        "    ps_in o;\n"
        "    o.pos = float4(i.pos.x / screen.x * 2.0 - 1.0, 1.0 - i.pos.y / screen.y * 2.0, 0.0, 1.0);\n"
        "    o.uv = i.uv;\n"
        "    o.col = i.col;\n"
        "    return o;\n"
        "}\n"
        "Texture2D tex : register(t0);\n"
        "SamplerState smp : register(s0);\n"
        "float4 ps_main(ps_in i) : SV_TARGET {\n"
        "    float a = tex.Sample(smp, i.uv).r;\n"
        "    return float4(i.col.rgb, i.col.a * a);\n"
        "}\n"
        "float4 ps_image(ps_in i) : SV_TARGET {\n"
        "    float4 c = tex.Sample(smp, i.uv);\n"
        "    return c * i.col;\n"
        "}\n";

    uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        return static_cast<uint32_t>(r) | (static_cast<uint32_t>(g) << 8) | (static_cast<uint32_t>(b) << 16) | (static_cast<uint32_t>(a) << 24);
    }

    float text_width(const char* text)
    {
        return font::text_width(text);
    }

    float line_height()
    {
        return font::line_height();
    }

    bool ready()
    {
        return g_ready;
    }

    static bool create_atlas()
    {
        if (!font::build())
        {
            return false;
        }

        D3D11_TEXTURE2D_DESC desc = {};

        desc.Width = font::atlas_width();

        desc.Height = font::atlas_height();

        desc.MipLevels = 1;

        desc.ArraySize = 1;

        desc.Format = DXGI_FORMAT_R8_UNORM;

        desc.SampleDesc.Count = 1;

        desc.Usage = D3D11_USAGE_IMMUTABLE;

        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initial = {};

        initial.pSysMem = font::atlas_pixels();

        initial.SysMemPitch = font::atlas_width();

        ID3D11Texture2D* texture = nullptr;

        if (FAILED(g_device->CreateTexture2D(&desc, &initial, &texture)))
        {
            return false;
        }

        HRESULT result = g_device->CreateShaderResourceView(texture, nullptr, &g_atlas);

        texture->Release();

        return SUCCEEDED(result);
    }

    static bool create_shaders()
    {
        ID3DBlob* vs_blob = nullptr;

        ID3DBlob* ps_blob = nullptr;

        ID3DBlob* errors = nullptr;

        if (FAILED(D3DCompile(shader_source, strlen(shader_source), nullptr, nullptr, nullptr, "vs_main", "vs_4_0", 0, 0, &vs_blob, &errors)))
        {
            if (errors != nullptr)
            {
                errors->Release();
            }

            return false;
        }

        if (FAILED(D3DCompile(shader_source, strlen(shader_source), nullptr, nullptr, nullptr, "ps_main", "ps_4_0", 0, 0, &ps_blob, &errors)))
        {
            if (errors != nullptr)
            {
                errors->Release();
            }

            vs_blob->Release();

            return false;
        }

        bool ok = SUCCEEDED(g_device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &g_vs));

        if (ok)
        {
            ok = SUCCEEDED(g_device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &g_ps));
        }

        if (ok)
        {
            ID3DBlob* image_blob = nullptr;

            if (SUCCEEDED(D3DCompile(shader_source, strlen(shader_source), nullptr, nullptr, nullptr, "ps_image", "ps_4_0", 0, 0, &image_blob, &errors)))
            {
                ok = SUCCEEDED(g_device->CreatePixelShader(image_blob->GetBufferPointer(), image_blob->GetBufferSize(), nullptr, &g_ps_image));

                image_blob->Release();
            }
            else
            {
                if (errors != nullptr)
                {
                    errors->Release();
                }

                ok = false;
            }
        }

        if (ok)
        {
            D3D11_INPUT_ELEMENT_DESC elements[3] = {};

            elements[0] = { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };

            elements[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 };

            elements[2] = { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 };

            ok = SUCCEEDED(g_device->CreateInputLayout(elements, 3, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &g_layout));
        }

        vs_blob->Release();

        ps_blob->Release();

        return ok;
    }

    static bool create_states()
    {
        D3D11_BLEND_DESC blend = {};

        blend.RenderTarget[0].BlendEnable = TRUE;

        blend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;

        blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

        blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

        blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;

        blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;

        blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

        blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        if (FAILED(g_device->CreateBlendState(&blend, &g_blend)))
        {
            return false;
        }

        D3D11_SAMPLER_DESC sampler = {};

        sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

        sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;

        sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;

        sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

        sampler.ComparisonFunc = D3D11_COMPARISON_NEVER;

        if (FAILED(g_device->CreateSamplerState(&sampler, &g_sampler)))
        {
            return false;
        }

        D3D11_RASTERIZER_DESC raster = {};

        raster.FillMode = D3D11_FILL_SOLID;

        raster.CullMode = D3D11_CULL_NONE;

        raster.ScissorEnable = FALSE;

        raster.DepthClipEnable = TRUE;

        if (FAILED(g_device->CreateRasterizerState(&raster, &g_raster)))
        {
            return false;
        }

        D3D11_DEPTH_STENCIL_DESC depth = {};

        depth.DepthEnable = FALSE;

        depth.StencilEnable = FALSE;

        if (FAILED(g_device->CreateDepthStencilState(&depth, &g_depth)))
        {
            return false;
        }

        return true;
    }

    static bool ensure_vertex_buffer(uint32_t count)
    {
        if (g_vertex_buffer != nullptr && count <= g_vertex_capacity)
        {
            return true;
        }

        if (g_vertex_buffer != nullptr)
        {
            g_vertex_buffer->Release();

            g_vertex_buffer = nullptr;
        }

        uint32_t capacity = count < 4096 ? 4096 : count + 2048;

        D3D11_BUFFER_DESC desc = {};

        desc.ByteWidth = capacity * sizeof(vertex);

        desc.Usage = D3D11_USAGE_DYNAMIC;

        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        if (FAILED(g_device->CreateBuffer(&desc, nullptr, &g_vertex_buffer)))
        {
            return false;
        }

        g_vertex_capacity = capacity;

        return true;
    }

    bool init(ID3D11Device* device)
    {
        if (g_ready)
        {
            return true;
        }

        g_device = device;

        if (!create_atlas() || !create_shaders() || !create_states())
        {
            shutdown();

            return false;
        }

        D3D11_BUFFER_DESC desc = {};

        desc.ByteWidth = sizeof(constants);

        desc.Usage = D3D11_USAGE_DYNAMIC;

        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        if (FAILED(g_device->CreateBuffer(&desc, nullptr, &g_constant_buffer)))
        {
            shutdown();

            return false;
        }

        g_ready = true;

        T7_LOG(cx("overlay/renderer: pipeline ready."));

        return true;
    }

    void begin(ID3D11DeviceContext* context, ID3D11RenderTargetView* target, float screen_w, float screen_h)
    {
        g_context = context;

        g_target = target;

        g_screen_w = screen_w;

        g_screen_h = screen_h;

        g_vertices.clear();

        g_images.clear();
    }

    static void push_quad(float x, float y, float w, float h, const overlay::font::glyph& src, uint32_t color)
    {
        vertex v0 = { x, y, src.u0, src.v0, color };

        vertex v1 = { x + w, y, src.u1, src.v0, color };

        vertex v2 = { x, y + h, src.u0, src.v1, color };

        vertex v3 = { x + w, y + h, src.u1, src.v1, color };

        g_vertices.push_back(v0);

        g_vertices.push_back(v1);

        g_vertices.push_back(v2);

        g_vertices.push_back(v2);

        g_vertices.push_back(v1);

        g_vertices.push_back(v3);
    }

    void draw_rect(float x, float y, float w, float h, uint32_t color)
    {
        push_quad(x, y, w, h, font::white_texel(), color);
    }

    void draw_image(float x, float y, float w, float h, ID3D11ShaderResourceView* srv)
    {
        if (srv == nullptr)
        {
            return;
        }

        image_command command = { x, y, w, h, srv };

        g_images.push_back(command);
    }

    void draw_text(float x, float y, const char* text, uint32_t color)
    {
        float pen_x = x;

        for (const char* p = text; *p != '\0'; p++)
        {
            const overlay::font::glyph& g = font::get(*p);

            if (g.width > 0.0f && *p != ' ')
            {
                push_quad(pen_x, y, g.width, g.height, g, color);
            }

            pen_x += g.advance;
        }
    }

    void draw_text_shadow(float x, float y, const char* text, uint32_t color, uint32_t shadow)
    {
        draw_text(x + 1.0f, y + 1.0f, text, shadow);

        draw_text(x, y, text, color);
    }

    struct state_backup
    {
        ID3D11InputLayout* layout;

        D3D11_PRIMITIVE_TOPOLOGY topology;

        ID3D11Buffer* vertex_buffer;

        UINT vertex_stride;

        UINT vertex_offset;

        ID3D11VertexShader* vs;

        ID3D11ClassInstance* vs_instances[256];

        UINT vs_instance_count;

        ID3D11PixelShader* ps;

        ID3D11ClassInstance* ps_instances[256];

        UINT ps_instance_count;

        ID3D11Buffer* vs_constant_buffer;

        ID3D11ShaderResourceView* ps_srv;

        ID3D11SamplerState* ps_sampler;

        ID3D11BlendState* blend;

        FLOAT blend_factor[4];

        UINT sample_mask;

        ID3D11DepthStencilState* depth;

        UINT stencil_ref;

        ID3D11RasterizerState* raster;

        UINT viewport_count;

        D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];

        ID3D11RenderTargetView* render_targets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];

        ID3D11DepthStencilView* depth_target;
    };

    static void capture_state(state_backup& backup)
    {
        backup.vs_instance_count = 256;

        backup.ps_instance_count = 256;

        backup.viewport_count = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;

        g_context->IAGetInputLayout(&backup.layout);

        g_context->IAGetPrimitiveTopology(&backup.topology);

        g_context->IAGetVertexBuffers(0, 1, &backup.vertex_buffer, &backup.vertex_stride, &backup.vertex_offset);

        g_context->VSGetShader(&backup.vs, backup.vs_instances, &backup.vs_instance_count);

        g_context->VSGetConstantBuffers(0, 1, &backup.vs_constant_buffer);

        g_context->PSGetShader(&backup.ps, backup.ps_instances, &backup.ps_instance_count);

        g_context->PSGetShaderResources(0, 1, &backup.ps_srv);

        g_context->PSGetSamplers(0, 1, &backup.ps_sampler);

        g_context->OMGetBlendState(&backup.blend, backup.blend_factor, &backup.sample_mask);

        g_context->OMGetDepthStencilState(&backup.depth, &backup.stencil_ref);

        g_context->RSGetState(&backup.raster);

        g_context->RSGetViewports(&backup.viewport_count, backup.viewports);

        g_context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, backup.render_targets, &backup.depth_target);
    }

    static void restore_state(state_backup& backup)
    {
        g_context->IASetInputLayout(backup.layout);

        g_context->IASetPrimitiveTopology(backup.topology);

        g_context->IASetVertexBuffers(0, 1, &backup.vertex_buffer, &backup.vertex_stride, &backup.vertex_offset);

        g_context->VSSetShader(backup.vs, backup.vs_instances, backup.vs_instance_count);

        g_context->VSSetConstantBuffers(0, 1, &backup.vs_constant_buffer);

        g_context->PSSetShader(backup.ps, backup.ps_instances, backup.ps_instance_count);

        g_context->PSSetShaderResources(0, 1, &backup.ps_srv);

        g_context->PSSetSamplers(0, 1, &backup.ps_sampler);

        g_context->OMSetBlendState(backup.blend, backup.blend_factor, backup.sample_mask);

        g_context->OMSetDepthStencilState(backup.depth, backup.stencil_ref);

        g_context->RSSetState(backup.raster);

        g_context->RSSetViewports(backup.viewport_count, backup.viewports);

        g_context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, backup.render_targets, backup.depth_target);

        if (backup.layout != nullptr) { backup.layout->Release(); }

        if (backup.vertex_buffer != nullptr) { backup.vertex_buffer->Release(); }

        if (backup.vs != nullptr) { backup.vs->Release(); }

        for (UINT i = 0; i < backup.vs_instance_count; i++) { if (backup.vs_instances[i] != nullptr) { backup.vs_instances[i]->Release(); } }

        if (backup.vs_constant_buffer != nullptr) { backup.vs_constant_buffer->Release(); }

        if (backup.ps != nullptr) { backup.ps->Release(); }

        for (UINT i = 0; i < backup.ps_instance_count; i++) { if (backup.ps_instances[i] != nullptr) { backup.ps_instances[i]->Release(); } }

        if (backup.ps_srv != nullptr) { backup.ps_srv->Release(); }

        if (backup.ps_sampler != nullptr) { backup.ps_sampler->Release(); }

        if (backup.blend != nullptr) { backup.blend->Release(); }

        if (backup.depth != nullptr) { backup.depth->Release(); }

        if (backup.raster != nullptr) { backup.raster->Release(); }

        for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++) { if (backup.render_targets[i] != nullptr) { backup.render_targets[i]->Release(); } }

        if (backup.depth_target != nullptr) { backup.depth_target->Release(); }
    }

    static void draw_image_command(const image_command& command)
    {
        vertex quad[6];

        quad[0] = { command.x, command.y, 0.0f, 0.0f, 0xFFFFFFFF };

        quad[1] = { command.x + command.w, command.y, 1.0f, 0.0f, 0xFFFFFFFF };

        quad[2] = { command.x, command.y + command.h, 0.0f, 1.0f, 0xFFFFFFFF };

        quad[3] = quad[2];

        quad[4] = quad[1];

        quad[5] = { command.x + command.w, command.y + command.h, 1.0f, 1.0f, 0xFFFFFFFF };

        D3D11_MAPPED_SUBRESOURCE mapped = {};

        if (FAILED(g_context->Map(g_vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            return;
        }

        memcpy(mapped.pData, quad, sizeof(quad));

        g_context->Unmap(g_vertex_buffer, 0);

        g_context->PSSetShader(g_ps_image, nullptr, 0);

        g_context->PSSetShaderResources(0, 1, &command.srv);

        g_context->Draw(6, 0);
    }

    void end()
    {
        if (!g_ready || g_context == nullptr || g_target == nullptr || (g_vertices.empty() && g_images.empty()))
        {
            return;
        }

        uint32_t count = static_cast<uint32_t>(g_vertices.size());

        uint32_t needed = count < 6 ? 6 : count;

        if (!ensure_vertex_buffer(needed))
        {
            return;
        }

        if (count > 0)
        {
            D3D11_MAPPED_SUBRESOURCE mapped = {};

            if (FAILED(g_context->Map(g_vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            {
                return;
            }

            memcpy(mapped.pData, g_vertices.data(), count * sizeof(vertex));

            g_context->Unmap(g_vertex_buffer, 0);
        }

        D3D11_MAPPED_SUBRESOURCE cmapped = {};

        if (SUCCEEDED(g_context->Map(g_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cmapped)))
        {
            constants* cb = static_cast<constants*>(cmapped.pData);

            cb->screen[0] = g_screen_w;

            cb->screen[1] = g_screen_h;

            cb->screen[2] = 0.0f;

            cb->screen[3] = 0.0f;

            g_context->Unmap(g_constant_buffer, 0);
        }

        state_backup backup;

        capture_state(backup);

        UINT stride = sizeof(vertex);

        UINT offset = 0;

        float blend_factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        D3D11_VIEWPORT viewport = {};

        viewport.TopLeftX = 0.0f;

        viewport.TopLeftY = 0.0f;

        viewport.Width = g_screen_w;

        viewport.Height = g_screen_h;

        viewport.MinDepth = 0.0f;

        viewport.MaxDepth = 1.0f;

        g_context->IASetInputLayout(g_layout);

        g_context->IASetVertexBuffers(0, 1, &g_vertex_buffer, &stride, &offset);

        g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        g_context->VSSetShader(g_vs, nullptr, 0);

        g_context->VSSetConstantBuffers(0, 1, &g_constant_buffer);

        g_context->PSSetShader(g_ps, nullptr, 0);

        g_context->PSSetShaderResources(0, 1, &g_atlas);

        g_context->PSSetSamplers(0, 1, &g_sampler);

        g_context->OMSetBlendState(g_blend, blend_factor, 0xFFFFFFFF);

        g_context->OMSetDepthStencilState(g_depth, 0);

        g_context->RSSetState(g_raster);

        g_context->RSSetViewports(1, &viewport);

        g_context->OMSetRenderTargets(1, &g_target, nullptr);

        if (count > 0)
        {
            g_context->Draw(count, 0);
        }

        for (const image_command& command : g_images)
        {
            draw_image_command(command);
        }

        restore_state(backup);

        g_vertices.clear();

        g_images.clear();
    }

    void shutdown()
    {
        if (g_atlas != nullptr) { g_atlas->Release(); g_atlas = nullptr; }

        if (g_vs != nullptr) { g_vs->Release(); g_vs = nullptr; }

        if (g_ps != nullptr) { g_ps->Release(); g_ps = nullptr; }

        if (g_ps_image != nullptr) { g_ps_image->Release(); g_ps_image = nullptr; }

        if (g_layout != nullptr) { g_layout->Release(); g_layout = nullptr; }

        if (g_vertex_buffer != nullptr) { g_vertex_buffer->Release(); g_vertex_buffer = nullptr; }

        if (g_constant_buffer != nullptr) { g_constant_buffer->Release(); g_constant_buffer = nullptr; }

        if (g_blend != nullptr) { g_blend->Release(); g_blend = nullptr; }

        if (g_sampler != nullptr) { g_sampler->Release(); g_sampler = nullptr; }

        if (g_raster != nullptr) { g_raster->Release(); g_raster = nullptr; }

        if (g_depth != nullptr) { g_depth->Release(); g_depth = nullptr; }

        g_vertex_capacity = 0;

        g_ready = false;
    }
}
