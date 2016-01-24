//=================================================================================================
//
//	MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

cbuffer PSConstants : register(b0)
{
    float2 InputSize0 : packoffset(c0.x);
    float2 InputSize1 : packoffset(c0.z);
    float2 InputSize2 : packoffset(c1.x);
    float2 InputSize3 : packoffset(c1.z);
    float2 OutputSize : packoffset(c2.x);
}

SamplerState PointSampler : register(s0);
SamplerState LinearSampler : register(s1);
SamplerState LinearWrapSampler : register(s2);
SamplerState LinearBorderSampler : register(s3);
SamplerState PointBorderSampler : register(s4);

Texture2D InputTexture0 : register(t0);
Texture2D InputTexture1 : register(t1);
Texture2D InputTexture2 : register(t2);
Texture2D InputTexture3 : register(t3);

Texture2D<int4> InputTextureInt0 : register(t0);
Texture2D<int4> InputTextureInt1 : register(t1);
Texture2D<int4> InputTextureInt2 : register(t2);
Texture2D<int4> InputTextureInt3 : register(t3);

Texture2D<uint4> InputTextureUint0 : register(t0);
Texture2D<uint4> InputTextureUint1 : register(t1);
Texture2D<uint4> InputTextureUint2 : register(t2);
Texture2D<uint4> InputTextureUint3 : register(t3);

struct PSInput
{
    float4 PositionSS : SV_Position;
    float2 TexCoord : TEXCOORD;
};