//=================================================================================================
//
//  MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "PCH.h"

#include "Skybox.h"

#include "Utility.h"
#include "ShaderCompilation.h"

namespace SampleFramework11
{

Skybox::Skybox()
{
}

Skybox::~Skybox()
{
}

void Skybox::Initialize(ID3D11Device* device)
{
    // Load the shaders
    vertexShader = CompileVSFromFile(device, L"SampleFramework11\\Shaders\\Skybox.hlsl", "SkyboxVS", "vs_4_0");
    geometryShader = CompileGSFromFile(device, L"SampleFramework11\\Shaders\\Skybox.hlsl", "SkyboxGS", "gs_4_0");
    emPixelShader = CompilePSFromFile(device, L"SampleFramework11\\Shaders\\Skybox.hlsl", "SkyboxPS", "ps_4_0");
    skyPixelShader = CompilePSFromFile(device, L"SampleFramework11\\Shaders\\Skybox.hlsl", "SkyModelPS", "ps_4_0");

    // Create the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    ID3DBlob* byteCode = vertexShader->ByteCode;
    DXCall(device->CreateInputLayout(layout, 1, byteCode->GetBufferPointer(), byteCode->GetBufferSize(), &inputLayout));

    // Create and initialize the vertex and index buffers
    Float3 verts[NumVertices] =
    {
        Float3(-1, 1, 1),
        Float3(1, 1, 1),
        Float3(1, -1, 1),
        Float3(-1, -1, 1),
        Float3(1, 1, -1),
        Float3(-1, 1, -1),
        Float3(-1, -1, -1),
        Float3(1, -1,- 1),
    };

    D3D11_BUFFER_DESC desc;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.ByteWidth = sizeof(verts);
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = verts;
    initData.SysMemPitch = 0;
    initData.SysMemSlicePitch = 0;
    DXCall(device->CreateBuffer(&desc, &initData, &vertexBuffer));

    unsigned short indices[NumIndices] =
    {
        0, 1, 2, 2, 3, 0,   // Front
        1, 4, 7, 7, 2, 1,   // Right
        4, 5, 6, 6, 7, 4,   // Back
        5, 0, 3, 3, 6, 5,   // Left
        5, 4, 1, 1, 0, 5,   // Top
        3, 2, 7, 7, 6, 3    // Bottom
    };

    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.ByteWidth = sizeof(indices);
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = 0;
    initData.pSysMem = indices;
    DXCall(device->CreateBuffer(&desc, &initData, &indexBuffer));

    // Create the VS constant buffer
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = CBSize(sizeof(VSConstants));
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    DXCall(device->CreateBuffer(&desc, nullptr, &vsConstantBuffer));

    // Create the GS constant buffer
    desc.ByteWidth = CBSize(sizeof(GSConstants));
    DXCall(device->CreateBuffer(&desc, nullptr, &gsConstantBuffer));

    // Create the PS constant buffer
    desc.ByteWidth = CBSize(sizeof(PSConstants));
    DXCall(device->CreateBuffer(&desc, nullptr, &psConstantBuffer));

    // Create a depth-stencil state
    D3D11_DEPTH_STENCIL_DESC dsDesc;
    dsDesc.DepthEnable = true;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    dsDesc.StencilEnable = false;
    dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    dsDesc.BackFace = dsDesc.FrontFace;
    DXCall(device->CreateDepthStencilState(&dsDesc, &dsState));

    // Create a blend state
    D3D11_BLEND_DESC blendDesc;
    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.IndependentBlendEnable = false;
    for (UINT i = 0; i < 8; ++i)
    {
        blendDesc.RenderTarget[i].BlendEnable = false;
        blendDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[i].DestBlend = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        blendDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
    }
    DXCall(device->CreateBlendState(&blendDesc, &blendState));

    // Create a rasterizer state
    D3D11_RASTERIZER_DESC rastDesc;
    rastDesc.AntialiasedLineEnable = false;
    rastDesc.CullMode = D3D11_CULL_NONE;
    rastDesc.DepthBias = 0;
    rastDesc.DepthBiasClamp = 0.0f;
    rastDesc.DepthClipEnable = true;
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.FrontCounterClockwise = false;
    rastDesc.MultisampleEnable = true;
    rastDesc.ScissorEnable = false;
    rastDesc.SlopeScaledDepthBias = 0;
    DXCall(device->CreateRasterizerState(&rastDesc, &rastState));

    D3D11_SAMPLER_DESC sampDesc;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.BorderColor[0] = 0;
    sampDesc.BorderColor[1] = 0;
    sampDesc.BorderColor[2] = 0;
    sampDesc.BorderColor[3] = 0;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.MaxAnisotropy = 1;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    sampDesc.MinLOD = 0;
    sampDesc.MipLODBias = 0;
    DXCall(device->CreateSamplerState(&sampDesc, &samplerState));
}

void Skybox::RenderCommon(  ID3D11DeviceContext* context,
                            ID3D11ShaderResourceView* environmentMap,
                            ID3D11PixelShader* ps,
                            const Float4x4& view,
                            const Float4x4& projection,
                            Float3 bias,
                            bool setStates,
                            uint32 rtIndex)
{
    if(setStates)
    {
        float blendFactor[4] = {1, 1, 1, 1};
        context->RSSetState(rastState);
        context->OMSetBlendState(blendState, blendFactor, 0xFFFFFFFF);
        context->OMSetDepthStencilState(dsState, 0);
        context->PSSetSamplers(0, 1, &(samplerState.GetInterfacePtr()));
    }

    // Get the viewports
    UINT numViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT oldViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    context->RSGetViewports(&numViewports, oldViewports);

    // Set a viewport with MinZ pushed back
    D3D11_VIEWPORT newViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    for(UINT i = 0; i < numViewports; ++i)
    {
        newViewports[i] = oldViewports[0];
        newViewports[i].MinDepth = 1.0f;
        newViewports[i].MaxDepth = 1.0f;
    }
    context->RSSetViewports(numViewports, newViewports);

    // Set the input layout
    context->IASetInputLayout(inputLayout);

    // Set the vertex buffer
    UINT stride = sizeof(XMFLOAT3);
    UINT offset = 0;
    ID3D11Buffer* vertexBuffers[1] = { vertexBuffer.GetInterfacePtr() };
    context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);

    // Set the index buffer
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);

    // Set primitive topology
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set the shaders
    context->VSSetShader(vertexShader, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
    context->GSSetShader(rtIndex != 0 ? geometryShader->Shader() : nullptr, nullptr, 0);
    context->DSSetShader(nullptr, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);

    // Set the constants
    D3D11_MAPPED_SUBRESOURCE mapped;
    DXCall(context->Map(vsConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    VSConstants* constants = reinterpret_cast<VSConstants*>(mapped.pData);
    constants->View = Float4x4::Transpose(view);
    constants->Projection = Float4x4::Transpose(projection);
    constants->Bias = bias;
    context->Unmap(vsConstantBuffer, 0);

    ID3D11Buffer* buffers[1] = { vsConstantBuffer };
    context->VSSetConstantBuffers(0, 1, buffers);

    if (rtIndex != 0)
    {
        DXCall(context->Map(gsConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
        GSConstants* gsConstants = reinterpret_cast<GSConstants*>(mapped.pData);
        gsConstants->RTIndex = rtIndex;
        context->Unmap(gsConstantBuffer, 0);
        buffers[0] = gsConstantBuffer;
        context->GSSetConstantBuffers(0, 1, buffers);
    }

    // Set the texture
    ID3D11ShaderResourceView* srViews[1] = { environmentMap };
    context->PSSetShaderResources(0, 1, srViews);

    // Draw
    context->DrawIndexed(NumIndices, 0, 0);

    // Set the viewport back to what it was
    context->RSSetViewports(numViewports, oldViewports);
}

void Skybox::Render(ID3D11DeviceContext* context,
                    ID3D11ShaderResourceView* environmentMap,
                    const Float4x4& view,
                    const Float4x4& projection,
                    Float3 bias,
                    bool setStates,
                    uint32 rtIndex)
{
    D3DPERF_BeginEvent(0xFFFFFFFF, L"Skybox Render");

    RenderCommon(context, environmentMap, emPixelShader, view, projection, bias, setStates, rtIndex);

    D3DPERF_EndEvent();
}

void Skybox::RenderSky( ID3D11DeviceContext* context,
                        const Float3& sunDirection,
                        bool enableSun,
                        const Float4x4& view,
                        const Float4x4& projection,
                        Float3 bias,
                        bool setStates,
                        uint32 rtIndex)
{
    D3DPERF_BeginEvent(0xFFFFFFFF, L"Skybox Render");

    // Set the pixel shader constants
    D3D11_MAPPED_SUBRESOURCE mapped;
    DXCall(context->Map(psConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    PSConstants* constants = reinterpret_cast<PSConstants*>(mapped.pData);
    constants->SunDirection = sunDirection;
    constants->EnableSun = enableSun;
    context->Unmap(psConstantBuffer, 0);

    ID3D11Buffer* buffers[1] = { psConstantBuffer };
    context->PSSetConstantBuffers(0, 1, buffers);

    RenderCommon(context, nullptr, skyPixelShader, view, projection, bias, setStates, rtIndex);

    D3DPERF_EndEvent();
}

const float AngleBetween(const Float3& dir0, const Float3& dir1)
{
    return std::acos(Float3::Dot(dir0, dir1));
}

Float3 Skybox::ComputeSkyColor(const Float3& dir, const Float3& sunDir, bool enableSun)
{
    Float3 skyDir = Float3(dir.x, std::abs(dir.y), dir.z);
    float gamma = AngleBetween(skyDir, sunDir);
    float S = AngleBetween(sunDir, Float3(0, 1, 0));
    float theta = AngleBetween(skyDir, Float3(0, 1, 0));

    float cosTheta = std::cos(theta);
    float cosS = std::cos(S);
    float cosGamma = std::cos(gamma);

    float num = (0.91f + 10 * std::exp(-3 * gamma) + 0.45f * cosGamma * cosGamma) * (1 - std::exp(-0.32f / cosTheta));
    float denom = (0.91f + 10 * std::exp(-3 * S) + 0.45f * cosS * cosS) * (1 - std::exp(-0.32f));

    float lum = num / std::max(denom, 0.0001f);

    // Clear Sky model only calculates luminance, so we'll pick a strong blue color for the sky
    const Float3 SkyColor = Float3(0.2f, 0.5f, 1.0f) * 1;
    const Float3 SunColor = Float3(1.0f, 0.8f, 0.3f) * 1500;
    const float SunWidth = 0.015f;

    Float3 color = SkyColor;

    // Add a circle for the sun
    if(enableSun)
    {
        float sunGamma = AngleBetween(dir, sunDir);
        color = Lerp(SunColor, SkyColor, Saturate(abs(sunGamma) / SunWidth));
    }

    color *= lum;
    color.x = std::max(color.x, 0.0f);
    color.y = std::max(color.y, 0.0f);
    color.z = std::max(color.z, 0.0f);

    return color;
}

}