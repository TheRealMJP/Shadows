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

//=================================================================================================
// Constant buffers
//=================================================================================================
cbuffer DrawFrustumConstants : register(b0)
{
    float4x4 ViewProjection;
    float4x4 InvFrustumViewProj;
    float4 Color;
}

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
};

VSOutput VSMain(in uint VertexID : SV_VertexID)
{
    float x = (VertexID % 2) ? 1.0f : -1.0f;
    float y = ((VertexID / 2) % 2) ? 1.0f : -1.0f;
    float z = (VertexID / 4) ? 1.0f : 0.0f;
    float4 vertexPosWS = mul(float4(x, y, z, 1.0f), InvFrustumViewProj);
    vertexPosWS.xyz /= vertexPosWS.w;

    VSOutput output;
    output.Position = mul(float4(vertexPosWS.xyz, 1.0f), ViewProjection);
    output.Color = Color;

    return output;
}

float4 PSMain(in VSOutput input) : SV_Target0
{
    return input.Color;
}
