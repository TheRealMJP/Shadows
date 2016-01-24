//=================================================================================================
//
//  MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "PCH.h"
#include "SH.h"
#include "Utility.h"
#include "ShaderCompilation.h"

namespace SampleFramework11
{

SH9 ProjectOntoSH9(const Float3& dir)
{
    SH9 sh;

    // Band 0
    sh.Coefficients[0] = 0.282095f;

    // Band 1
    sh.Coefficients[1] = 0.488603f * dir.y;
    sh.Coefficients[2] = 0.488603f * dir.z;
    sh.Coefficients[3] = 0.488603f * dir.x;

    // Band 2
    sh.Coefficients[4] = 1.092548f * dir.x * dir.y;
    sh.Coefficients[5] = 1.092548f * dir.y * dir.z;
    sh.Coefficients[6] = 0.315392f * (3.0f * dir.z * dir.z - 1.0f);
    sh.Coefficients[7] = 1.092548f * dir.x * dir.z;
    sh.Coefficients[8] = 0.546274f * (dir.x * dir.x - dir.y * dir.y);

    return sh;
}

SH9Color ProjectOntoSH9Color(const Float3& dir, const Float3& color)
{
    SH9 sh = ProjectOntoSH9(dir);
    SH9Color shColor;
    for(uint64 i = 0; i < 9; ++i)
        shColor.Coefficients[i] = color * sh.Coefficients[i];
    return shColor;
}

H4 ProjectOntoH4(const Float3& dir)
{
    H4 result;

    result[0] = (1.0f / std::sqrt(2.0f * 3.14159f));

    // Band 1
    result[1] = std::sqrt(1.5f / 3.14159f) * dir.y;
    result[2] = std::sqrt(1.5f / 3.14159f) * (2 * dir.z - 1.0f);
    result[3] = std::sqrt(1.5f / 3.14159f) * dir.x;

    return result;
}

float EvalH4(const H4& h, const Float3& dir)
{
    H4 b = ProjectOntoH4(dir);
    return H4::Dot(h, b);
}

H4 ConvertToH4(const SH9& sh)
{
    const float rt2 = sqrt(2.0f);
    const float rt32 = sqrt(3.0f / 2.0f);
    const float rt52 = sqrt(5.0f / 2.0f);
    const float rt152 = sqrt(15.0f / 2.0f);
    const float convMatrix[4][9] =
    {
        { 1.0f / rt2, 0, 0.5f * rt32, 0, 0, 0, 0, 0, 0 },
        { 0, 1.0f / rt2, 0, 0, 0, (3.0f / 8.0f) * rt52, 0, 0, 0 },
        { 0, 0, 1.0f / (2.0f * rt2), 0, 0, 0, 0.25f * rt152, 0, 0 },
        { 0, 0, 0, 1.0f / rt2, 0, 0, 0, (3.0f / 8.0f) * rt52, 0 }
    };

    H4 hBasis;

    for(uint64 row = 0; row < 4; ++row)
    {
        hBasis.Coefficients[row] = 0.0f;

        for(uint64 col = 0; col < 9; ++col)
            hBasis.Coefficients[row] += convMatrix[row][col] * sh.Coefficients[col];
    }

    return hBasis;
}

SH9Color ProjectCubemapToSH(ID3D11DeviceContext* context, ID3D11ShaderResourceView* cubeMap)
{
    ID3D11Texture2DPtr srcTexture;
    cubeMap->GetResource(reinterpret_cast<ID3D11Resource**>(&srcTexture));

    D3D11_TEXTURE2D_DESC srcDesc;
    srcTexture->GetDesc(&srcDesc);

    ID3D11DevicePtr device;
    context->GetDevice(&device);

    ID3D11Texture2DPtr tempTexture;
    D3D11_TEXTURE2D_DESC tempDesc = srcDesc;
    tempDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    tempDesc.MipLevels = 1;
    tempDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    tempDesc.Usage = D3D11_USAGE_DEFAULT;
    DXCall(device->CreateTexture2D(&tempDesc, nullptr, &tempTexture));

    ID3D11UnorderedAccessViewPtr tempUAV;
    DXCall(device->CreateUnorderedAccessView(tempTexture, nullptr, &tempUAV));

    ID3D11ShaderResourceViewPtr tempSRV;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = srcDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = srcDesc.MipLevels;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = 6;
    DXCall(device->CreateShaderResourceView(srcTexture, &srvDesc, &tempSRV));

    static const uint32 TGSize = 1024;
    static ComputeShaderPtr decodeShader;
    if(decodeShader.Valid() == false)
    {
        CompileOptions opts;
        opts.Add("TGSize_", TGSize);
        decodeShader = CompileCSFromFile(device, L"SampleFramework11\\Shaders\\DecodeTextureCS.hlsl",
                                         "DecodeTextureCS", "cs_5_0", opts);
    }

    ID3D11ShaderResourceView* srvs[1] = { tempSRV };
    context->CSSetShaderResources(0, 1, srvs);

    ID3D11UnorderedAccessView* uavs[1] = { tempUAV };
    context->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

    context->CSSetShader(decodeShader, nullptr, 0);

    context->Dispatch(DispatchSize(TGSize, srcDesc.Width), srcDesc.Height, 6);

    float red[9] = { 0.0f };
    float green[9] = { 0.0f };
    float blue[9] = { 0.0f };

    //$$$TODO
    // DXCall(D3DX11SHProjectCubeMap(context, 3, tempTexture, red, green, blue));

    SH9Color sh;
    for(uint64 i = 0; i < 9; ++i)
        sh.Coefficients[i] = Float3(red[i], green[i], blue[i]);

    return sh;
}

}