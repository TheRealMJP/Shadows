//=================================================================================================
//
//	Shadows Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "SampleFramework11/Shaders/PPIncludes.hlsl"
#include "SharedConstants.h"

//=================================================================================================
// Constants
//=================================================================================================
cbuffer PPConstants : register(b1)
{
    float BloomThreshold;
    float BloomMagnitude;
    float BloomBlurSigma;
    float Tau;
    float TimeDelta;
    float KeyValue;
};

//=================================================================================================
// Helper Functions
//=================================================================================================

// Approximates luminance from an RGB value
float CalcLuminance(float3 color)
{
    return max(dot(color, float3(0.299f, 0.587f, 0.114f)), 0.0001f);
}

// Retrieves the log-average luminance from the texture
float GetAvgLuminance(Texture2D lumTex)
{
    return lumTex.Load(uint3(0, 0, 0)).x;
}

// Applies the filmic curve from John Hable's presentation
float3 ToneMapFilmicALU(float3 color)
{
    color = max(0, color - 0.004f);
    color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f)+ 0.06f);

    // result has 1/2.2 baked in
    return pow(color, 2.2f);
}

// Determines the color based on exposure settings
float3 CalcExposedColor(float3 color, float avgLuminance, float threshold, out float exposure)
{
    // Use geometric mean
    avgLuminance = max(avgLuminance, 0.001f);
    float keyValue = KeyValue;
    float linearExposure = (KeyValue / avgLuminance);
    exposure = log2(max(linearExposure, 0.0001f));
    exposure -= threshold;
    return exp2(exposure) * color;
}

// Applies exposure and tone mapping to the specific color, and applies
// the threshold to the exposure value.
float3 ToneMap(float3 color, float avgLuminance, float threshold, out float exposure)
{
    float pixelLuminance = CalcLuminance(color);
    color = CalcExposedColor(color, avgLuminance, threshold, exposure);
    color = ToneMapFilmicALU(color);
    return color;
}

// Calculates the gaussian blur weight for a given distance and sigmas
float CalcGaussianWeight(int sampleDist, float sigma)
{
    float g = 1.0f / sqrt(2.0f * 3.14159 * sigma * sigma);
    return (g * exp(-(sampleDist * sampleDist) / (2 * sigma * sigma)));
}

// Performs a gaussian blur in one direction
float4 Blur(in PSInput input, float2 texScale, float sigma)
{
    float4 color = 0;
    for (int i = -6; i < 6; i++)
    {
        float weight = CalcGaussianWeight(i, sigma);
        float2 texCoord = input.TexCoord;
        texCoord += (i / InputSize0) * texScale;
        float4 sample = InputTexture0.Sample(PointSampler, texCoord);
        color += sample * weight;
    }

    return color;
}

// ================================================================================================
// Shader Entry Points
// ================================================================================================

// Uses a lower exposure to produce a value suitable for a bloom pass
float4 Threshold(in PSInput input) : SV_Target
{
    float3 color = 0;

    color = InputTexture0.Sample(LinearSampler, input.TexCoord).rgb;

    // Tone map it to threshold
    float avgLuminance = GetAvgLuminance(InputTexture1);
    float exposure = 0;
    float pixelLuminance = CalcLuminance(color);
    color = CalcExposedColor(color, avgLuminance, BloomThreshold, exposure);

    if(dot(color, 0.333f) <= 0.001f)
        color = 0.0f;

    return float4(color, 1.0f);
}

// Uses hw bilinear filtering for upscaling or downscaling
float4 Scale(in PSInput input) : SV_Target
{
    return InputTexture0.Sample(LinearSampler, input.TexCoord);
}

// Horizontal gaussian blur
float4 BloomBlurH(in PSInput input) : SV_Target
{
    return Blur(input, float2(1, 0), BloomBlurSigma);
}

// Vertical gaussian blur
float4 BloomBlurV(in PSInput input) : SV_Target
{
    return Blur(input, float2(0, 1), BloomBlurSigma);
}

// Applies exposure and tone mapping to the input, and combines it with the
// results of the bloom pass
float4 Composite(in PSInput input) : SV_Target0
{
    // Tone map the primary input
    float avgLuminance = GetAvgLuminance(InputTexture1);
    float3 color = InputTexture0.Sample(PointSampler, input.TexCoord).rgb;

    float exposure = 0;
    color = ToneMap(color, avgLuminance, 0, exposure);

    // Sample the bloom
    float3 bloom = InputTexture2.Sample(LinearSampler, input.TexCoord).rgb;
    bloom *= BloomMagnitude;

    color = color + bloom;

    return float4(color, 1.0f);
}

// Creates the luminance map for the scene
float4 LuminanceMap(in PSInput input) : SV_Target
{
    // Sample the input
    float3 color = InputTexture0.Sample(LinearSampler, input.TexCoord).rgb;

    // calculate the luminance using a weighted average
    float luminance = log(max(CalcLuminance(color), 0.00001f));

    return float4(luminance, 1.0f, 1.0f, 1.0f);
}

// Slowly adjusts the scene luminance based on the previous scene luminance
float4 AdaptLuminance(in PSInput input) : SV_Target
{
    float lastLum = InputTexture0.Load(uint3(0, 0, 0)).x;
    float currentLum = exp(InputTexture1.SampleLevel(PointSampler, float2(0.5f, 0.5f), 10.0f).x);

    // Adapt the luminance using Pattanaik's technique
    float adaptedLum = lastLum + (currentLum - lastLum) * (1 - exp(-TimeDelta * Tau));

    return float4(adaptedLum, 1.0f, 1.0f, 1.0f);
}

Texture2D<float> DepthTexture : register(t0);
Texture2DMS<float> DepthTextureMSAA : register(t0);

float4 DrawDepth(in PSInput input) : SV_Target
{
    return float4(saturate(DepthTexture[input.PositionSS.xy].xxx - 0.95f) * 20.0f, 1.0f);
}

float4 DrawDepthMSAA(in PSInput input) : SV_Target
{
    return float4(saturate(DepthTextureMSAA.Load(input.PositionSS.xy, 0).xxx - 0.95f) * 20.0f, 1.0f);
}
