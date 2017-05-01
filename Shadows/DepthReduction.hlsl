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
#include "SharedConstants.h"

//=================================================================================================
// Constants
//=================================================================================================
#define MSAA_ 1
static const uint NumThreads = ReductionTGSize * ReductionTGSize;

//=================================================================================================
// Resources
//=================================================================================================
#if MSAA_
    Texture2DMS<float> DepthMap : register(t0);
#else
    Texture2D<float> DepthMap : register(t0);
#endif

Texture2D<unorm float2> ReductionMap : register(t0);

SamplerState LinearClampSampler : register(s0);

RWTexture2D<unorm float2> OutputMap : register(u0);

cbuffer ReductionConstants : register(b0)
{
    float4x4 Projection;
    float NearClip;
    float FarClip;
}

#if CS_
    // -- shared memory
    groupshared float2 depthSamples[NumThreads];
#endif

// ------------------------------------------------------------------------------------------------
// Returns the min of 4 values
// ------------------------------------------------------------------------------------------------
float Min4(in float4 values)
{
    return min(min(values.x, values.y), min(values.z, values.w));
}

// ------------------------------------------------------------------------------------------------
// Returns the max of 4 values
// ------------------------------------------------------------------------------------------------
float Max4(in float4 values)
{
    return max(max(values.x, values.y), max(values.z, values.w));
}

// First pass of the depth reduction
float2 DepthReductionInitialPS(in float4 PositionSS : SV_Position,
                               in float2 TexCoord : TEXCOORD0) : SV_Target0
{
    float minDepth = 1.0f;
    float maxDepth = 0.0f;

    #if MSAA_
        uint w, h, numSamples;
        DepthMap.GetDimensions(w, h, numSamples);

        for(uint sIdx = 0; sIdx < numSamples; ++sIdx)
        {
            [unroll]
            for(uint y = 0; y < 2; ++y)
            {
                [unroll]
                for(uint x = 0; x < 2; ++x)
                {
                    uint2 samplePos = min(uint2(PositionSS.xy) * 2 + uint2(x, y), uint2(w - 1, h - 1));
                    float depthSample = DepthMap.Load(samplePos, sIdx);

                    if(depthSample < 1.0f)
                    {
                        // Convert to linear Z
                        depthSample = Projection._43 / (depthSample - Projection._33);
                        depthSample = saturate((depthSample - NearClip) / (FarClip - NearClip));
                        minDepth = min(minDepth, depthSample);
                        maxDepth = max(maxDepth, depthSample);
                    }
                }
            }
        }
    #else
        float4 depthSamples = DepthMap.GatherRed(LinearClampSampler, TexCoord);
        depthSamples = Projection._43 / (depthSamples - Projection._33);
        depthSamples = saturate((depthSamples - NearClip) / (FarClip - NearClip));
        float minDepth = Min4(depthSamples);
        float maxDepth = Max4(depthSamples);
    #endif

    return float2(minDepth, maxDepth);
}

// Subsequent passes of the depth reduction
float2 DepthReductionPS(in float4 PositionSS : SV_Position,
                        in float2 TexCoord : TEXCOORD0) : SV_Target0
{
    float4 minSamples = ReductionMap.GatherRed(LinearClampSampler, TexCoord);
    float4 maxSamples = ReductionMap.GatherGreen(LinearClampSampler, TexCoord);
    float minDepth = Min4(minSamples);
    float maxDepth = Max4(maxSamples);

    return float2(minDepth, maxDepth);
}

#if CS_

// First pass of the depth reduction
[numthreads(ReductionTGSize, ReductionTGSize, 1)]
void DepthReductionInitialCS(in uint3 GroupID : SV_GroupID,
                             in uint3 GroupThreadID : SV_GroupThreadID,
                             uint ThreadIndex : SV_GroupIndex)
{
    float minDepth = 1.0f;
    float maxDepth = 0.0f;

    #if MSAA_
        uint2 textureSize;
        uint numSamples;
        DepthMap.GetDimensions(textureSize.x, textureSize.y, numSamples);
    #else
        uint2 textureSize;
        DepthMap.GetDimensions(textureSize.x, textureSize.y);
    #endif

    uint2 samplePos = GroupID.xy * ReductionTGSize + GroupThreadID.xy;
    samplePos = min(samplePos, textureSize - 1);

    #if MSAA_
        for(uint sIdx = 0; sIdx < numSamples; ++sIdx)
        {
            float depthSample = DepthMap.Load(samplePos, sIdx);

            if(depthSample < 1.0f)
            {
                // Convert to linear Z
                depthSample = Projection._43 / (depthSample - Projection._33);
                depthSample = saturate((depthSample - NearClip) / (FarClip - NearClip));
                minDepth = min(minDepth, depthSample);
                maxDepth = max(maxDepth, depthSample);
            }
        }
    #else
        float depthSample = DepthMap[samplePos];

        if(depthSample < 1.0f)
        {
            // Convert to linear Z
            depthSample = Projection._43 / (depthSample - Projection._33);
            depthSample = saturate((depthSample - NearClip) / (FarClip - NearClip));
            minDepth = min(minDepth, depthSample);
            maxDepth = max(maxDepth, depthSample);
        }
    #endif

    // Store in shared memory
    depthSamples[ThreadIndex] = float2(minDepth, maxDepth);
    GroupMemoryBarrierWithGroupSync();

    // Reduce
	[unroll]
	for(uint s = NumThreads / 2; s > 0; s >>= 1)
    {
		if(ThreadIndex < s)
        {
			depthSamples[ThreadIndex].x = min(depthSamples[ThreadIndex].x, depthSamples[ThreadIndex + s].x);
            depthSamples[ThreadIndex].y = max(depthSamples[ThreadIndex].y, depthSamples[ThreadIndex + s].y);
        }

		GroupMemoryBarrierWithGroupSync();
	}

    if(ThreadIndex == 0)
    {
        minDepth = depthSamples[0].x;
        maxDepth = depthSamples[0].y;
        OutputMap[GroupID.xy] = float2(minDepth, maxDepth);
    }
}

// Subsequent passes of the depth reduction
[numthreads(ReductionTGSize, ReductionTGSize, 1)]
void DepthReductionCS(in uint3 GroupID : SV_GroupID, in uint3 GroupThreadID : SV_GroupThreadID,
                      in uint ThreadIndex : SV_GroupIndex)
{
    uint2 textureSize;
    ReductionMap.GetDimensions(textureSize.x, textureSize.y);

    uint2 samplePos = GroupID.xy * ReductionTGSize + GroupThreadID.xy;
    samplePos = min(samplePos, textureSize - 1);

    float minDepth = ReductionMap[samplePos].x;
    float maxDepth = ReductionMap[samplePos].y;

    if(minDepth == 0.0f)
        minDepth = 1.0f;

    // Store in shared memory
    depthSamples[ThreadIndex] = float2(minDepth, maxDepth);
    GroupMemoryBarrierWithGroupSync();

    // Reduce
	[unroll]
	for(uint s = NumThreads / 2; s > 0; s >>= 1)
    {
		if(ThreadIndex < s)
        {
			depthSamples[ThreadIndex].x = min(depthSamples[ThreadIndex].x, depthSamples[ThreadIndex + s].x);
            depthSamples[ThreadIndex].y = max(depthSamples[ThreadIndex].y, depthSamples[ThreadIndex + s].y);
        }

		GroupMemoryBarrierWithGroupSync();
	}

    if(ThreadIndex == 0)
    {
        minDepth = depthSamples[0].x;
        maxDepth = depthSamples[0].y;
        OutputMap[GroupID.xy] = float2(minDepth, maxDepth);
    }
}

#endif // CS_