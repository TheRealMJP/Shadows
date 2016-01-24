//=================================================================================================
//
//  MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#pragma once

#include "PCH.h"

#include "InterfacePointers.h"
#include "Math.h"
#include "ShaderCompilation.h"

namespace SampleFramework11
{

class Skybox
{

public:

    Skybox();
    ~Skybox();

    void Initialize(ID3D11Device* device);

    void Render(ID3D11DeviceContext* context,
                ID3D11ShaderResourceView* environmentMap,
                const Float4x4& view,
                const Float4x4& projection,
                Float3 bias = Float3(1, 1, 1),
                bool setStates = true,
                uint32 rtIndex = 0);

    void RenderSky(ID3D11DeviceContext* context,
                    const Float3& sunDirection,
                    bool enableSun,
                    const Float4x4& view,
                    const Float4x4& projection,
                    Float3 bias = Float3(1, 1, 1),
                    bool setStates = true,
                    uint32 rtIndex = 0);

    static Float3 ComputeSkyColor(const Float3& dir, const Float3& sunDir, bool enableSun);

protected:

    void RenderCommon(ID3D11DeviceContext* context,
                        ID3D11ShaderResourceView* environmentMap,
                        ID3D11PixelShader* ps,
                        const Float4x4& view,
                        const Float4x4& projection,
                        Float3 bias,
                        bool setStates,
                        uint32 rtIndex);

    static const uint64 NumIndices = 36;
    static const uint64 NumVertices = 8;

    struct VSConstants
    {
        Float4x4 View;
        Float4x4 Projection;
        Float3 Bias;
    };

    struct GSConstants
    {
        uint32 RTIndex;
    };

    struct PSConstants
    {
        Float3 SunDirection;
        float pad;
        uint32 EnableSun;
    };

    VertexShaderPtr vertexShader;
    GeometryShaderPtr geometryShader;
    PixelShaderPtr emPixelShader;
    PixelShaderPtr skyPixelShader;
    ID3D11InputLayoutPtr inputLayout;
    ID3D11BufferPtr vertexBuffer;
    ID3D11BufferPtr indexBuffer;
    ID3D11BufferPtr vsConstantBuffer;
    ID3D11BufferPtr gsConstantBuffer;
    ID3D11BufferPtr psConstantBuffer;
    ID3D11DepthStencilStatePtr dsState;
    ID3D11BlendStatePtr blendState;
    ID3D11RasterizerStatePtr rastState;
    ID3D11SamplerStatePtr samplerState;
};

}