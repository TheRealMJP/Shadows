//=================================================================================================
//
//	Shadows Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

//=================================================================================================
// Includes
//=================================================================================================
#include "VSM.hlsl"
#include "MSM.hlsl"
#include "AppSettings.hlsl"
#include "SharedConstants.h"

//=================================================================================================
// Constants
//=================================================================================================s
#ifndef MSAASamples_
    #define MSAASamples_ 1
#endif

#ifndef SampleRadius_
    #define SampleRadius_ 0
#endif

//=================================================================================================
// Resources
//=================================================================================================
#if MSAASamples_ > 1
    Texture2DMS<float> ShadowMap : register(t0);
#else
    Texture2D<float> ShadowMap : register(t0);
#endif

#if Vertical_
    Texture2D VSMMap : register(t0);
#else
    Texture2DArray VSMMap : register(t0);
#endif

cbuffer VSMConstants : register(b0)
{
    float4 CascadeScale;
    float4 Cascade0Scale;
    float2 ShadowMapDimensions;
}

struct VSOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD;
};

VSOutput FullScreenVS(in uint VertexID : SV_VertexID)
{
    VSOutput output;

    if(VertexID == 0)
    {
        output.Position = float4(-1.0f, 1.0f, 1.0f, 1.0f);
        output.TexCoord = float2(0.0f, 0.0f);
    }
    else if(VertexID == 1)
    {
        output.Position = float4(3.0f, 1.0f, 1.0f, 1.0f);
        output.TexCoord = float2(2.0f, 0.0f);
    }
    else
    {
        output.Position = float4(-1.0f, -3.0f, 1.0f, 1.0f);
        output.TexCoord = float2(0.0f, 2.0f);
    }

    return output;
}

float4 ConvertToVSM(in VSOutput input) : SV_Target0
{
    float sampleWeight = 1.0f / float(MSAASamples_);
    uint2 coords = uint2(input.Position.xy);

    #if UseEVSM_
        float2 exponents = GetEVSMExponents(PositiveExponent, NegativeExponent, SMFormat);
    #endif

    float4 average = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // Sample indices to Load() must be literal, so force unroll
    [unroll]
    for(uint i = 0; i < MSAASamples_; ++i)
    {
        // Convert to EVSM representation
        #if MSAASamples_ > 1
            float depth = ShadowMap.Load(coords, i);
        #else
            float depth = ShadowMap[coords];
        #endif

        #if UseMSM_
            float4 msmDepth = 0.0f;
            if(SMFormat == SMFormat_SM16Bit)
                msmDepth = GetOptimizedMoments(depth);
            else
                msmDepth = float4(depth, depth * depth, depth * depth * depth, depth * depth * depth * depth);
            average += sampleWeight * msmDepth;
        #else
            #if UseEVSM_
                float2 vsmDepth = WarpDepth(depth, exponents);
            #else
                float2 vsmDepth = depth;
            #endif

            average += sampleWeight * float4(vsmDepth.xy, vsmDepth.xy * vsmDepth.xy);
        #endif
    }

    #if ShadowMode_ == ShadowModeEVSM4_ || UseMSM_
        return average;
    #else
        return average.xzxz;
    #endif
}

float4 BlurSample(in float2 screenPos, in float offset, in float2 mapSize)
{
    #if Vertical_
        float2 samplePos = screenPos;
        samplePos.y = clamp(screenPos.y + offset, 0, mapSize.y);
        return VSMMap[uint2(samplePos)];
    #else
        float2 samplePos = screenPos;
        samplePos.x = clamp(screenPos.x + offset, 0, mapSize.x);
        return VSMMap[uint3(samplePos, 0)];
    #endif
}

float4 BlurVSM(in VSOutput input) : SV_Target0
{
    #if Vertical_
        float scale = abs(CascadeScale.y);
        float maxFilterSize = MaxKernelSize / abs(Cascade0Scale.y);
    #else
        float scale = abs(CascadeScale.x);
        float maxFilterSize = MaxKernelSize / abs(Cascade0Scale.x);
    #endif

    const float KernelSize = clamp(min(FilterSize, maxFilterSize) * scale, 1.0f, MaxKernelSize);
    const float Radius = KernelSize / 2.0f;

    #if GPUSceneSubmission_
        [branch]
        if(KernelSize > 1.0f)
        {
            const int SampleRadius = int(round(Radius));

            float4 sum = 0.0f;

            [loop]
            for(int i = -SampleRadius; i <= SampleRadius; ++i)
            {
                float4 sample = BlurSample(input.Position.xy, i, ShadowMapDimensions);

                sample *= saturate((Radius + 0.5f) - abs(i));

                sum += sample;
            }

            return sum / KernelSize;
        }
        else
        {
            return BlurSample(input.Position.xy, 0, ShadowMapDimensions);
        }
    #else
        float4 sum = 0.0f;

        [unroll]
        for(int i = -SampleRadius_; i <= SampleRadius_; ++i)
        {
            float4 sample = BlurSample(input.Position.xy, i, ShadowMapDimensions);

            sample *= saturate((Radius + 0.5f) - abs(i));

            sum += sample;
        }

        return sum / KernelSize;
    #endif
}