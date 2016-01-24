//=================================================================================================
//
//  MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "PCH.h"

#include "Utility.h"
#include "Exceptions.h"
#include "InterfacePointers.h"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include "FileIO.h"
#include "ShaderCompilation.h"
#include "GraphicsTypes.h"

namespace SampleFramework11
{

// Converts from cartesian to barycentric coordinates
XMFLOAT3 CartesianToBarycentric(float x, float y, const XMFLOAT2& pos1, const XMFLOAT2& pos2, const XMFLOAT2& pos3)
{
    float r1 = (pos2.y - pos3.y) * (x - pos3.x) + (pos3.x - pos2.x) * (y - pos3.y);
    r1 /= (pos2.y - pos3.y) * (pos1.x - pos3.x) + (pos3.x - pos2.x) * (pos1.y - pos3.y);

    float r2 = (pos3.y - pos1.y) * (x - pos3.x) + (pos1.x - pos3.x) * (y - pos3.y);
    r2 /= (pos3.y - pos1.y) * (pos2.x - pos3.x) + (pos1.x - pos3.x) * (pos2.y - pos3.y);

    float r3 = 1.0f - r1 - r2;

    return XMFLOAT3(r1, r2, r3);
}

// Converts from barycentric to cartesian coordinates
XMFLOAT2 BarycentricToCartesian(const XMFLOAT3& r, const XMFLOAT2& pos1, const XMFLOAT2& pos2, const XMFLOAT2& pos3)
{
    float x = r.x * pos1.x + r.y * pos2.x + r.z * pos3.x;
    float y = r.x * pos1.y + r.y * pos2.y + r.z * pos3.y;

    return XMFLOAT2(x, y);
}

// Converts from barycentric to cartesian coordinates
XMVECTOR BarycentricToCartesian(const XMFLOAT3& r, FXMVECTOR pos1, FXMVECTOR pos2, FXMVECTOR pos3)
{
    XMVECTOR rvec;
    rvec = XMVectorScale(pos1, r.x);
    rvec += XMVectorScale(pos2, r.y);
    rvec += XMVectorScale(pos3, r.z);

    return rvec;
}

// Returns true if the given barycentric coordinate is in the triangle
BOOL PointIsInTriangle(const XMFLOAT3& r, float epsilon)
{
    float minr = 0.0f - epsilon;
    float maxr = 1.0f + epsilon;

    if(r.x < minr || r.x > maxr)
        return false;

    if(r.y < minr || r.y > maxr)
        return false;

    if(r.z < minr || r.z > maxr)
        return false;

    float rsum = r.x + r.y + r.z;
    return rsum >= minr && rsum <= maxr;
}

// Computes a compute shader dispatch size given a thread group size, and number of elements to process
uint32 DispatchSize(uint32 tgSize, uint32 numElements)
{
    uint32 dispatchSize = numElements / tgSize;
    dispatchSize += numElements % tgSize > 0 ? 1 : 0;
    return dispatchSize;
}

void SetCSInputs(ID3D11DeviceContext* context, ID3D11ShaderResourceView* srv0, ID3D11ShaderResourceView* srv1,
                    ID3D11ShaderResourceView* srv2, ID3D11ShaderResourceView* srv3)
{
    ID3D11ShaderResourceView* srvs[4] = { srv0, srv1, srv2, srv3 };
    context->CSSetShaderResources(0, 4, srvs);
}

void ClearCSInputs(ID3D11DeviceContext* context)
{
    SetCSInputs(context, nullptr, nullptr, nullptr, nullptr);
}

void SetCSOutputs(ID3D11DeviceContext* context, ID3D11UnorderedAccessView* uav0, ID3D11UnorderedAccessView* uav1,
                    ID3D11UnorderedAccessView* uav2, ID3D11UnorderedAccessView* uav3, ID3D11UnorderedAccessView* uav4,
                    ID3D11UnorderedAccessView* uav5)
{
    ID3D11UnorderedAccessView* uavs[6] = { uav0, uav1, uav2, uav3, uav4, uav5 };
    context->CSSetUnorderedAccessViews(0, 6, uavs, nullptr);
}

void ClearCSOutputs(ID3D11DeviceContext* context)
{
    SetCSOutputs(context, nullptr, nullptr, nullptr, nullptr);
}

void SetCSSamplers(ID3D11DeviceContext* context, ID3D11SamplerState* sampler0, ID3D11SamplerState* sampler1,
                    ID3D11SamplerState* sampler2, ID3D11SamplerState* sampler3)
{
    ID3D11SamplerState* samplers[4] = { sampler0, sampler1, sampler2, sampler3 };
    context->CSSetSamplers(0, 4, samplers);
}

void SetCSShader(ID3D11DeviceContext* context, ID3D11ComputeShader* shader)
{
    context->CSSetShader(shader, nullptr, 0);
}

void SetCSConstants(ID3D11DeviceContext* context, ID3D11Buffer* constantBuffer, uint32 slot)
{
    ID3D11Buffer* constants[1] = { constantBuffer };
    context->CSSetConstantBuffers(slot, 1, constants);
}

// Loads a texture, using either the DDS loader or the WIC loader
ID3D11ShaderResourceViewPtr LoadTexture(ID3D11Device* device, const wchar* filePath)
{
    ID3D11DeviceContextPtr context;
    device->GetImmediateContext(&context);

    ID3D11ResourcePtr resource;
    ID3D11ShaderResourceViewPtr srv;

    const std::wstring extension = GetFileExtension(filePath);
    if(extension == L"DDS" || extension == L"dds")
    {
        DXCall(CreateDDSTextureFromFile(device, filePath, &resource, &srv, 0));
        return srv;
    }
    else
    {
        DXCall(CreateWICTextureFromFile(device, context, filePath, &resource, &srv, 0));
        return srv;
    }
}


// Decode a texture into 32-bit floats and copies it to the CPU
void GetTextureData(ID3D11Device* device, ID3D11ShaderResourceView* textureSRV,
                    std::vector<Float4>& textureData)
{
    static ComputeShaderPtr decodeTextureCS;
    static ComputeShaderPtr decodeTextureArrayCS;

    static const uint32 TGSize = 16;

    if(decodeTextureCS.Valid() == false)
    {
        CompileOptions opts;
        opts.Add("TGSize_", TGSize);
        decodeTextureCS = CompileCSFromFile(device, L"SampleFramework11\\Shaders\\DecodeTextureCS.hlsl",
                                            "DecodeTextureCS", "cs_5_0", opts);

        decodeTextureArrayCS = CompileCSFromFile(device, L"SampleFramework11\\Shaders\\DecodeTextureCS.hlsl",
                                                 "DecodeTextureArrayCS", "cs_5_0", opts);
    }

    ID3D11Texture2DPtr texture;
    textureSRV->GetResource(reinterpret_cast<ID3D11Resource**>(&texture));

    D3D11_TEXTURE2D_DESC texDesc;
    texture->GetDesc(&texDesc);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    textureSRV->GetDesc(&srvDesc);

    ID3D11ShaderResourceViewPtr sourceSRV = textureSRV;
    uint32 arraySize = texDesc.ArraySize;
    if(srvDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURECUBE
       || srvDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURECUBEARRAY)
    {
        arraySize *= 6;

        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.ArraySize = arraySize;
        srvDesc.Texture2DArray.FirstArraySlice = 0;
        srvDesc.Texture2DArray.MostDetailedMip = 0;
        srvDesc.Texture2DArray.MipLevels = -1;
        DXCall(device->CreateShaderResourceView(texture, &srvDesc, &sourceSRV));
    }

    D3D11_TEXTURE2D_DESC decodeTextureDesc;
    decodeTextureDesc.Width = texDesc.Width;
    decodeTextureDesc.Height = texDesc.Height;
    decodeTextureDesc.ArraySize = arraySize;
    decodeTextureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    decodeTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    decodeTextureDesc.MipLevels = 1;
    decodeTextureDesc.MiscFlags = 0;
    decodeTextureDesc.SampleDesc.Count = 1;
    decodeTextureDesc.SampleDesc.Quality = 0;
    decodeTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    decodeTextureDesc.CPUAccessFlags = 0;

    ID3D11Texture2DPtr decodeTexture;
    DXCall(device->CreateTexture2D(&decodeTextureDesc, nullptr, &decodeTexture));

    ID3D11UnorderedAccessViewPtr decodeTextureUAV;
    DXCall(device->CreateUnorderedAccessView(decodeTexture, nullptr, &decodeTextureUAV));

    ID3D11DeviceContextPtr context;
    device->GetImmediateContext(&context);

    SetCSInputs(context, sourceSRV);
    SetCSOutputs(context, decodeTextureUAV);
    SetCSShader(context, arraySize > 1 ? decodeTextureArrayCS : decodeTextureCS);

    context->Dispatch(DispatchSize(TGSize, texDesc.Width), DispatchSize(TGSize, texDesc.Height), arraySize);

    ClearCSInputs(context);
    ClearCSOutputs(context);

    StagingTexture2D stagingTexture;
    stagingTexture.Initialize(device, texDesc.Width, texDesc.Height, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 1, 0, arraySize);
    context->CopyResource(stagingTexture.Texture, decodeTexture);

    textureData.resize(texDesc.Width * texDesc.Height * arraySize);

    for(uint32 slice = 0; slice < arraySize; ++slice)
    {
        uint32 pitch = 0;
        const uint8* srcData = reinterpret_cast<const uint8*>(stagingTexture.Map(context, slice, pitch));

        const uint32 sliceOffset = texDesc.Width * texDesc.Height * slice;

        for(uint32 y = 0; y < texDesc.Height; ++y)
        {
            const Float4* rowData = reinterpret_cast<const Float4*>(srcData);

            for(uint32 x = 0; x < texDesc.Width; ++x)
                textureData[y * texDesc.Width + x + sliceOffset] = rowData[x];

            srcData += pitch;
        }
    }
}

}