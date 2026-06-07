//=================================================================================================
//
//  Shadows Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

//=================================================================================================
// Includes
//=================================================================================================
#include "SharedConstants.h"
#include "AppSettings.hlsl"

#if MSAA_
    Texture2DMSArray<float> ShadowMap : register(t0);
#else
    Texture2DArray<float> ShadowMap : register(t0);
#endif

//=================================================================================================
// Constant buffers
//=================================================================================================
cbuffer DrawCascadesConstants : register(b0)
{
    uint2 RTSize;
    uint2 DrawSize;
    float DrawPixelToShadowTexelScale;
}

float4 VSMain(in uint VertexID : SV_VertexID) : SV_Position
{
    float x = (VertexID % 2) ? 3.0f : -1.0f;
    float y = (VertexID / 2) ? -3.0f : 1.0f;

    return float4(x, y, 0.0f, 1.0f);
}

float4 PSMain(in float4 ScreenPos : SV_Position) : SV_Target0
{
    uint2 pixelXY = uint2(ScreenPos.xy);
    uint2 drawXY = uint2(pixelXY.x % DrawSize.x, pixelXY.y - (RTSize.y - DrawSize.y));
    uint cascadeIdx = pixelXY.x / DrawSize.x;
    uint3 texelXYS = uint3(drawXY * DrawPixelToShadowTexelScale, cascadeIdx);
#if MSAA_
    return ShadowMap.Load(texelXYS, 0);
#else
    return ShadowMap[texelXYS];
#endif
}
