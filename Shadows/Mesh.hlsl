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
#include "PCFKernels.hlsl"
#include "VSM.hlsl"
#include "MSM.hlsl"
#include "AppSettings.hlsl"

//=================================================================================================
// Constant buffers
//=================================================================================================
cbuffer VSConstants : register(b0)
{
    float4x4 World;
    float4x4 ViewProjection;
}

cbuffer PSConstants : register(b0)
{
    float3 CameraPosWS;
	float4x4 ShadowMatrix;
	float4 CascadeSplits;
    float4 CascadeOffsets[NumCascades];
    float4 CascadeScales[NumCascades];
}

//=================================================================================================
// Resources
//=================================================================================================
Texture2D DiffuseMap : register(t0);
Texture2DArray ShadowMap : register(t1);
Texture2D<float> RandomRotations : register(t2);

SamplerState AnisoSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);
SamplerComparisonState ShadowSamplerPCF : register(s2);
SamplerState VSMSampler : register(s3);

//=================================================================================================
// Input/Output structs
//=================================================================================================
struct VSInput
{
    float3 PositionOS 		    : POSITION;
    float3 NormalOS 		    : NORMAL;
    float2 TexCoord 		    : TEXCOORD0;
};

struct VSOutput
{
    float4 PositionCS 		    : SV_Position;
    float3 PositionWS 		    : POSITIONWS;
    float3 NormalWS 		    : NORMALWS;
    float2 TexCoord 		    : TEXCOORD;
	float DepthVS			    : DEPTHVS;
};

struct PSInput
{
    float4 PositionSS 		    : SV_Position;
    float3 PositionWS 		    : POSITIONWS;
	float3 NormalWS 		    : NORMALWS;
    float2 TexCoord 		    : TEXCOORD;
	float DepthVS			    : DEPTHVS;
};

//=================================================================================================
// Vertex Shader
//=================================================================================================
VSOutput VS(in VSInput input)
{
    VSOutput output;

    // Calc the world-space position
    output.PositionWS = mul(float4(input.PositionOS, 1.0f), World).xyz;

    // Calc the clip-space position
    output.PositionCS = mul(float4(output.PositionWS, 1.0f), ViewProjection);

    output.DepthVS = output.PositionCS.w;

	// Rotate the normal into world space
    output.NormalWS = normalize(mul(input.NormalOS, (float3x3)World));

    // Pass along the texture coordinate
    output.TexCoord = input.TexCoord;

    return output;
}

float2 ComputeReceiverPlaneDepthBias(float3 texCoordDX, float3 texCoordDY)
{
    float2 biasUV;
    biasUV.x = texCoordDY.y * texCoordDX.z - texCoordDX.y * texCoordDY.z;
    biasUV.y = texCoordDX.x * texCoordDY.z - texCoordDY.x * texCoordDX.z;
    biasUV *= 1.0f / ((texCoordDX.x * texCoordDY.y) - (texCoordDX.y * texCoordDY.x));
    return biasUV;
}

//-------------------------------------------------------------------------------------------------
// Samples the shadow map with a fixed-size PCF kernel optimized with GatherCmp. Uses code
// from "Fast Conventional Shadow Filtering" by Holger Gruen, in GPU Pro.
//-------------------------------------------------------------------------------------------------
float SampleShadowMapFixedSizePCF(in float3 shadowPos, in float3 shadowPosDX,
                         in float3 shadowPosDY, in uint cascadeIdx) {
    float2 shadowMapSize;
    float numSlices;
    ShadowMap.GetDimensions(shadowMapSize.x, shadowMapSize.y, numSlices);

    float lightDepth = shadowPos.z;

    const float bias = Bias;

    #if UsePlaneDepthBias_
        float2 texelSize = 1.0f / shadowMapSize;

        float2 receiverPlaneDepthBias = ComputeReceiverPlaneDepthBias(shadowPosDX, shadowPosDY);

        // Static depth biasing to make up for incorrect fractional sampling on the shadow map grid
        float fractionalSamplingError = dot(float2(1.0f, 1.0f) * texelSize, abs(receiverPlaneDepthBias));
        lightDepth -= min(fractionalSamplingError, 0.01f);
    #else
        lightDepth -= bias;
    #endif

    #if FilterSize_ == 2
        return ShadowMap.SampleCmpLevelZero(ShadowSamplerPCF, float3(shadowPos.xy, cascadeIdx), lightDepth);
    #else
        const int FS_2 = FilterSize_ / 2;

        float2 tc = shadowPos.xy;

        float4 s = 0.0f;
        float2 stc = (shadowMapSize * tc.xy) + float2(0.5f, 0.5f);
        float2 tcs = floor(stc);
        float2 fc;
        int row;
        int col;
        float w = 0.0f;
        float4 v1[FS_2 + 1];
        float2 v0[FS_2 + 1];

        fc.xy = stc - tcs;
        tc.xy = tcs / shadowMapSize;

        for(row = 0; row < FilterSize_; ++row)
            for(col = 0; col < FilterSize_; ++col)
                w += W[row][col];

        // -- loop over the rows
        [unroll]
        for(row = -FS_2; row <= FS_2; row += 2)
        {
            [unroll]
            for(col = -FS_2; col <= FS_2; col += 2)
            {
                float value = W[row + FS_2][col + FS_2];

                if(col > -FS_2)
                    value += W[row + FS_2][col + FS_2 - 1];

                if(col < FS_2)
                    value += W[row + FS_2][col + FS_2 + 1];

                if(row > -FS_2) {
                    value += W[row + FS_2 - 1][col + FS_2];

                    if(col < FS_2)
                        value += W[row + FS_2 - 1][col + FS_2 + 1];

                    if(col > -FS_2)
                        value += W[row + FS_2 - 1][col + FS_2 - 1];
                }

                if(value != 0.0f)
                {
                    float sampleDepth = lightDepth;

                    #if UsePlaneDepthBias_
                        // Compute offset and apply planar depth bias
                        float2 offset = float2(col, row) * texelSize;
                        sampleDepth += dot(offset, receiverPlaneDepthBias);
                    #endif

                    v1[(col + FS_2) / 2] = ShadowMap.GatherCmp(ShadowSampler, float3(tc.xy, cascadeIdx),
                                                                 sampleDepth, int2(col, row));
                }
                else
                    v1[(col + FS_2) / 2] = 0.0f;

                if(col == -FS_2)
                {
                    s.x += (1.0f - fc.y) * (v1[0].w * (W[row + FS_2][col + FS_2]
                                         - W[row + FS_2][col + FS_2] * fc.x)
                                         + v1[0].z * (fc.x * (W[row + FS_2][col + FS_2]
                                         - W[row + FS_2][col + FS_2 + 1.0f])
                                         + W[row + FS_2][col + FS_2 + 1]));
                    s.y += fc.y * (v1[0].x * (W[row + FS_2][col + FS_2]
                                         - W[row + FS_2][col + FS_2] * fc.x)
                                         + v1[0].y * (fc.x * (W[row + FS_2][col + FS_2]
                                         - W[row + FS_2][col + FS_2 + 1])
                                         +  W[row + FS_2][col + FS_2 + 1]));
                    if(row > -FS_2)
                    {
                        s.z += (1.0f - fc.y) * (v0[0].x * (W[row + FS_2 - 1][col + FS_2]
                                               - W[row + FS_2 - 1][col + FS_2] * fc.x)
                                               + v0[0].y * (fc.x * (W[row + FS_2 - 1][col + FS_2]
                                               - W[row + FS_2 - 1][col + FS_2 + 1])
                                               + W[row + FS_2 - 1][col + FS_2 + 1]));
                        s.w += fc.y * (v1[0].w * (W[row + FS_2 - 1][col + FS_2]
                                            - W[row + FS_2 - 1][col + FS_2] * fc.x)
                                            + v1[0].z * (fc.x * (W[row + FS_2 - 1][col + FS_2]
                                            - W[row + FS_2 - 1][col + FS_2 + 1])
                                            + W[row + FS_2 - 1][col + FS_2 + 1]));
                    }
                }
                else if(col == FS_2)
                {
                    s.x += (1 - fc.y) * (v1[FS_2].w * (fc.x * (W[row + FS_2][col + FS_2 - 1]
                                         - W[row + FS_2][col + FS_2]) + W[row + FS_2][col + FS_2])
                                         + v1[FS_2].z * fc.x * W[row + FS_2][col + FS_2]);
                    s.y += fc.y * (v1[FS_2].x * (fc.x * (W[row + FS_2][col + FS_2 - 1]
                                         - W[row + FS_2][col + FS_2] ) + W[row + FS_2][col + FS_2])
                                         + v1[FS_2].y * fc.x * W[row + FS_2][col + FS_2]);
                    if(row > -FS_2) {
                        s.z += (1 - fc.y) * (v0[FS_2].x * (fc.x * (W[row + FS_2 - 1][col + FS_2 - 1]
                                            - W[row + FS_2 - 1][col + FS_2])
                                            + W[row + FS_2 - 1][col + FS_2])
                                            + v0[FS_2].y * fc.x * W[row + FS_2 - 1][col + FS_2]);
                        s.w += fc.y * (v1[FS_2].w * (fc.x * (W[row + FS_2 - 1][col + FS_2 - 1]
                                            - W[row + FS_2 - 1][col + FS_2])
                                            + W[row + FS_2 - 1][col + FS_2])
                                            + v1[FS_2].z * fc.x * W[row + FS_2 - 1][col + FS_2]);
                    }
                }
                else
                {
                    s.x += (1 - fc.y) * (v1[(col + FS_2) / 2].w * (fc.x * (W[row + FS_2][col + FS_2 - 1]
                                        - W[row + FS_2][col + FS_2 + 0] ) + W[row + FS_2][col + FS_2 + 0])
                                        + v1[(col + FS_2) / 2].z * (fc.x * (W[row + FS_2][col + FS_2 - 0]
                                        - W[row + FS_2][col + FS_2 + 1]) + W[row + FS_2][col + FS_2 + 1]));
                    s.y += fc.y * (v1[(col + FS_2) / 2].x * (fc.x * (W[row + FS_2][col + FS_2-1]
                                        - W[row + FS_2][col + FS_2 + 0]) + W[row + FS_2][col + FS_2 + 0])
                                        + v1[(col + FS_2) / 2].y * (fc.x * (W[row + FS_2][col + FS_2 - 0]
                                        - W[row + FS_2][col + FS_2 + 1]) + W[row + FS_2][col + FS_2 + 1]));
                    if(row > -FS_2) {
                        s.z += (1 - fc.y) * (v0[(col + FS_2) / 2].x * (fc.x * (W[row + FS_2 - 1][col + FS_2 - 1]
                                                - W[row + FS_2 - 1][col + FS_2 + 0]) + W[row + FS_2 - 1][col + FS_2 + 0])
                                                + v0[(col + FS_2) / 2].y * (fc.x * (W[row + FS_2 - 1][col + FS_2 - 0]
                                                - W[row + FS_2 - 1][col + FS_2 + 1]) + W[row + FS_2 - 1][col + FS_2 + 1]));
                        s.w += fc.y * (v1[(col + FS_2) / 2].w * (fc.x * (W[row + FS_2 - 1][col + FS_2 - 1]
                                                - W[row + FS_2 - 1][col + FS_2 + 0]) + W[row + FS_2 - 1][col + FS_2 + 0])
                                                + v1[(col + FS_2) / 2].z * (fc.x * (W[row + FS_2 - 1][col + FS_2 - 0]
                                                - W[row + FS_2 - 1][col + FS_2 + 1]) + W[row + FS_2 - 1][col + FS_2 + 1]));
                    }
                }

                if(row != FS_2)
                    v0[(col + FS_2) / 2] = v1[(col + FS_2) / 2].xy;
            }
        }

        return dot(s, 1.0f) / w;
    #endif
}

//--------------------------------------------------------------------------------------
// Samples the shadow map using a PCF kernel made up from random points on a disc
//--------------------------------------------------------------------------------------
float SampleShadowMapRandomDiscPCF(in float3 shadowPos, in float3 shadowPosDX,
                                   in float3 shadowPosDY, in uint cascadeIdx,
                                   in uint2 screenPos)
{
    float2 maxFilterSize = MaxKernelSize / abs(CascadeScales[0].xy);
    float2 filterSize = clamp(min(FilterSize.xx, maxFilterSize) * abs(CascadeScales[cascadeIdx].xy), 1.0f, MaxKernelSize);

    float result = 1.0f;

    // Get the size of the shadow map
    uint2 shadowMapSize;
    uint numSlices;
    ShadowMap.GetDimensions(shadowMapSize.x, shadowMapSize.y, numSlices);

    #if UsePlaneDepthBias_
        float2 texelSize = 1.0f / shadowMapSize;

        float2 receiverPlaneDepthBias = ComputeReceiverPlaneDepthBias(shadowPosDX, shadowPosDY);

        // Static depth biasing to make up for incorrect fractional sampling on the shadow map grid
        float fractionalSamplingError = dot(float2(1.0f, 1.0f) * texelSize, abs(receiverPlaneDepthBias));
        float shadowDepth = shadowPos.z - min(fractionalSamplingError, 0.01f);
    #else
        float shadowDepth = shadowPos.z - Bias;
    #endif

    [branch]
    if(filterSize.x > 1.0f || filterSize.y > 1.0f)
    {
        #if RandomizeOffsets_
            // Get a value to randomly rotate the kernel by
            uint2 randomRotationsSize;
            RandomRotations.GetDimensions(randomRotationsSize.x, randomRotationsSize.y);
            uint2 randomSamplePos = screenPos % randomRotationsSize;
            float theta = RandomRotations[randomSamplePos] * Pi2;
            float2x2 randomRotationMatrix = float2x2(float2(cos(theta), -sin(theta)),
                                                     float2(sin(theta), cos(theta)));
        #endif

        float2 sampleScale = (0.5f * filterSize) / shadowMapSize;

        float sum = 0.0f;
        for(uint i = 0; i < uint(NumDiscSamples); ++i)
        {
            #if RandomizeOffsets_
                float2 sampleOffset = mul(PoissonSamples[i], randomRotationMatrix) * sampleScale;
            #else
                float2 sampleOffset = PoissonSamples[i] * sampleScale;
            #endif

            float2 samplePos = shadowPos.xy + sampleOffset;

            #if UsePlaneDepthBias_
                // Compute offset and apply planar depth bias
                float sampleDepth = shadowDepth + dot(sampleOffset, receiverPlaneDepthBias);
            #else
                float sampleDepth = shadowDepth;
            #endif

            sum += ShadowMap.SampleCmpLevelZero(ShadowSamplerPCF, float3(samplePos, cascadeIdx), sampleDepth);
        }

        result = sum / NumDiscSamples;
    }
    else
    {
        result = ShadowMap.SampleCmpLevelZero(ShadowSamplerPCF, float3(shadowPos.xy, cascadeIdx), shadowDepth);
    }

    return result;
}

//--------------------------------------------------------------------------------------
// Samples the shadow map grid-sampled PCF
//--------------------------------------------------------------------------------------
float SampleShadowMapGridPCF(in float3 shadowPos, in float3 shadowPosDX,
                             in float3 shadowPosDY, in uint cascadeIdx)
{
    float2 maxFilterSize = MaxKernelSize / abs(CascadeScales[0].xy);
    float2 filterSize = clamp(min(FilterSize.xx, maxFilterSize) * abs(CascadeScales[cascadeIdx].xy), 1.0f, MaxKernelSize);

    float result = 0.0f;

    // Get the size of the shadow map
    uint2 shadowMapSize;
    uint numSlices;
    ShadowMap.GetDimensions(shadowMapSize.x, shadowMapSize.y, numSlices);

    float2 texelSize = 1.0f / shadowMapSize;

    #if UsePlaneDepthBias_
        float2 receiverPlaneDepthBias = ComputeReceiverPlaneDepthBias(shadowPosDX, shadowPosDY);

        // Static depth biasing to make up for incorrect fractional sampling on the shadow map grid
        float fractionalSamplingError = dot(float2(1.0f, 1.0f) * texelSize, abs(receiverPlaneDepthBias));
        float shadowDepth = shadowPos.z - min(fractionalSamplingError, 0.01f);
    #else
        float shadowDepth = shadowPos.z - Bias;
    #endif

    [branch]
    if(filterSize.x > 1.0f || filterSize.y > 1.0f)
    {
        // Get the texel that will be sampled
        float2 shadowTexel = shadowPos.xy * shadowMapSize;
        float2 texelFraction = frac(shadowTexel);

        const float2 Radius = filterSize / 2.0f;

        int2 minOffset = int2(floor(texelFraction - Radius));
        int2 maxOffset = int2(texelFraction + Radius);

        float weightSum = 0.0f;

        [loop]
        for(int y = minOffset.y; y <= maxOffset.y; ++y)
        {
            float yWeight = 1.0f;
            if(y == minOffset.y)
                yWeight = saturate((Radius.y - texelFraction.y) + 1.0f + y);
            else if(y == maxOffset.y)
                yWeight = saturate(Radius.y + texelFraction.y - y);

            [loop]
            for(int x = minOffset.x; x <= maxOffset.x; ++x)
            {
                float2 sampleOffset = texelSize * float2(x, y);
                float2 samplePos = shadowPos.xy + sampleOffset;

                #if UsePlaneDepthBias_
                    // Compute offset and apply planar depth bias
                    float sampleDepth = shadowDepth + dot(sampleOffset, receiverPlaneDepthBias);
                #else
                    float sampleDepth = shadowDepth;
                #endif

                float sample = ShadowMap.SampleCmpLevelZero(ShadowSampler, float3(samplePos.xy, cascadeIdx), sampleDepth);

                float xWeight = 1.0f;
                if(x == minOffset.x)
                    xWeight = saturate((Radius.x - texelFraction.x) + 1.0f + x);
                else if(x == maxOffset.x)
                    xWeight = saturate(Radius.x + texelFraction.x - x);

                float2 sampleCoverage = float2(xWeight, yWeight);

                float sampleWeight = sampleCoverage.x * sampleCoverage.y;
                weightSum += sampleWeight;

                result += sample * sampleWeight;
            }
        }

        result /= (filterSize.x * filterSize.y);
    }
    else
    {
        result = ShadowMap.SampleCmpLevelZero(ShadowSamplerPCF, float3(shadowPos.xy, cascadeIdx), shadowDepth);
    }

    return result;
}

//-------------------------------------------------------------------------------------------------
// Samples the VSM shadow map
//-------------------------------------------------------------------------------------------------
float SampleShadowMapVSM(in float3 shadowPos, in float3 shadowPosDX,
                         in float3 shadowPosDY, uint cascadeIdx)
{
    float depth = shadowPos.z;

    float2 occluder = ShadowMap.SampleGrad(VSMSampler, float3(shadowPos.xy, cascadeIdx),
                                           shadowPosDX.xy, shadowPosDY.xy).xy;

    return ChebyshevUpperBound(occluder, depth, VSMBias * 0.01, LightBleedingReduction);
}

//-------------------------------------------------------------------------------------------------
// Samples the EVSM shadow map
//-------------------------------------------------------------------------------------------------
float SampleShadowMapEVSM(in float3 shadowPos, in float3 shadowPosDX,
                          in float3 shadowPosDY, uint cascadeIdx)
{
    float2 exponents = GetEVSMExponents(PositiveExponent, NegativeExponent, SMFormat);
    float2 warpedDepth = WarpDepth(shadowPos.z, exponents);

    float4 occluder = ShadowMap.SampleGrad(VSMSampler, float3(shadowPos.xy, cascadeIdx),
                                            shadowPosDX.xy, shadowPosDY.xy);

    // Derivative of warping at depth
    float2 depthScale = VSMBias * 0.01f * exponents * warpedDepth;
    float2 minVariance = depthScale * depthScale;

    #if ShadowMode_ == ShadowModeEVSM4_
        float posContrib = ChebyshevUpperBound(occluder.xz, warpedDepth.x, minVariance.x, LightBleedingReduction);
        float negContrib = ChebyshevUpperBound(occluder.yw, warpedDepth.y, minVariance.y, LightBleedingReduction);
        return min(posContrib, negContrib);
    #else
        // Positive only
        return ChebyshevUpperBound(occluder.xy, warpedDepth.x, minVariance.x, LightBleedingReduction);
    #endif
}

//-------------------------------------------------------------------------------------------------
// Samples the MSM shadow map
//-------------------------------------------------------------------------------------------------
float SampleShadowMapMSM(in float3 shadowPos, in float3 shadowPosDX,
                         in float3 shadowPosDY, uint cascadeIdx)
{
    float depth = shadowPos.z;
    float4 moments = ShadowMap.SampleGrad(VSMSampler, float3(shadowPos.xy, cascadeIdx),
                                          shadowPosDX.xy, shadowPosDY.xy);
    if(SMFormat == SMFormat_SM16Bit)
        moments = ConvertOptimizedMoments(moments);

    #if ShadowMode_ == ShadowModeMSMHausdorff_
        float result = ComputeMSMHausdorff(moments, depth, MSMDepthBias * 0.001f, MSMMomentBias * 0.001f);
    #else
        float result = ComputeMSMHamburger(moments, depth, MSMDepthBias * 0.001f, MSMMomentBias * 0.001f);
    #endif

    return ReduceLightBleeding(result, LightBleedingReduction);
}


//-------------------------------------------------------------------------------------------------
// Helper function for SampleShadowMapOptimizedPCF
//-------------------------------------------------------------------------------------------------
float SampleShadowMap(in float2 base_uv, in float u, in float v, in float2 shadowMapSizeInv,
                      in uint cascadeIdx,  in float depth, in float2 receiverPlaneDepthBias) {

    float2 uv = base_uv + float2(u, v) * shadowMapSizeInv;

    #if UsePlaneDepthBias_
        float z = depth + dot(float2(u, v) * shadowMapSizeInv, receiverPlaneDepthBias);
    #else
        float z = depth;
    #endif

    return ShadowMap.SampleCmpLevelZero(ShadowSamplerPCF, float3(uv, cascadeIdx), z);
}

//-------------------------------------------------------------------------------------------------
// The method used in The Witness
//-------------------------------------------------------------------------------------------------
float SampleShadowMapOptimizedPCF(in float3 shadowPos, in float3 shadowPosDX,
                         in float3 shadowPosDY, in uint cascadeIdx) {
    float2 shadowMapSize;
    float numSlices;
    ShadowMap.GetDimensions(shadowMapSize.x, shadowMapSize.y, numSlices);

    float lightDepth = shadowPos.z;

    const float bias = Bias;

    #if UsePlaneDepthBias_
        float2 texelSize = 1.0f / shadowMapSize;

        float2 receiverPlaneDepthBias = ComputeReceiverPlaneDepthBias(shadowPosDX, shadowPosDY);

        // Static depth biasing to make up for incorrect fractional sampling on the shadow map grid
        float fractionalSamplingError = 2 * dot(float2(1.0f, 1.0f) * texelSize, abs(receiverPlaneDepthBias));
        lightDepth -= min(fractionalSamplingError, 0.01f);
    #else
        float2 receiverPlaneDepthBias;
        lightDepth -= bias;
    #endif

    float2 uv = shadowPos.xy * shadowMapSize; // 1 unit - 1 texel

    float2 shadowMapSizeInv = 1.0 / shadowMapSize;

    float2 base_uv;
    base_uv.x = floor(uv.x + 0.5);
    base_uv.y = floor(uv.y + 0.5);

    float s = (uv.x + 0.5 - base_uv.x);
    float t = (uv.y + 0.5 - base_uv.y);

    base_uv -= float2(0.5, 0.5);
    base_uv *= shadowMapSizeInv;

    float sum = 0;

    #if FilterSize_ == 2
        return ShadowMap.SampleCmpLevelZero(ShadowSamplerPCF, float3(shadowPos.xy, cascadeIdx), lightDepth);
    #elif FilterSize_ == 3

        float uw0 = (3 - 2 * s);
        float uw1 = (1 + 2 * s);

        float u0 = (2 - s) / uw0 - 1;
        float u1 = s / uw1 + 1;

        float vw0 = (3 - 2 * t);
        float vw1 = (1 + 2 * t);

        float v0 = (2 - t) / vw0 - 1;
        float v1 = t / vw1 + 1;

        sum += uw0 * vw0 * SampleShadowMap(base_uv, u0, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw0 * SampleShadowMap(base_uv, u1, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw0 * vw1 * SampleShadowMap(base_uv, u0, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw1 * SampleShadowMap(base_uv, u1, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        return sum * 1.0f / 16;

    #elif FilterSize_ == 5

        float uw0 = (4 - 3 * s);
        float uw1 = 7;
        float uw2 = (1 + 3 * s);

        float u0 = (3 - 2 * s) / uw0 - 2;
        float u1 = (3 + s) / uw1;
        float u2 = s / uw2 + 2;

        float vw0 = (4 - 3 * t);
        float vw1 = 7;
        float vw2 = (1 + 3 * t);

        float v0 = (3 - 2 * t) / vw0 - 2;
        float v1 = (3 + t) / vw1;
        float v2 = t / vw2 + 2;

        sum += uw0 * vw0 * SampleShadowMap(base_uv, u0, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw0 * SampleShadowMap(base_uv, u1, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw0 * SampleShadowMap(base_uv, u2, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        sum += uw0 * vw1 * SampleShadowMap(base_uv, u0, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw1 * SampleShadowMap(base_uv, u1, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw1 * SampleShadowMap(base_uv, u2, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        sum += uw0 * vw2 * SampleShadowMap(base_uv, u0, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw2 * SampleShadowMap(base_uv, u1, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw2 * SampleShadowMap(base_uv, u2, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        return sum * 1.0f / 144;

    #else // FilterSize_ == 7

        float uw0 = (5 * s - 6);
        float uw1 = (11 * s - 28);
        float uw2 = -(11 * s + 17);
        float uw3 = -(5 * s + 1);

        float u0 = (4 * s - 5) / uw0 - 3;
        float u1 = (4 * s - 16) / uw1 - 1;
        float u2 = -(7 * s + 5) / uw2 + 1;
        float u3 = -s / uw3 + 3;

        float vw0 = (5 * t - 6);
        float vw1 = (11 * t - 28);
        float vw2 = -(11 * t + 17);
        float vw3 = -(5 * t + 1);

        float v0 = (4 * t - 5) / vw0 - 3;
        float v1 = (4 * t - 16) / vw1 - 1;
        float v2 = -(7 * t + 5) / vw2 + 1;
        float v3 = -t / vw3 + 3;

        sum += uw0 * vw0 * SampleShadowMap(base_uv, u0, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw0 * SampleShadowMap(base_uv, u1, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw0 * SampleShadowMap(base_uv, u2, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw3 * vw0 * SampleShadowMap(base_uv, u3, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        sum += uw0 * vw1 * SampleShadowMap(base_uv, u0, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw1 * SampleShadowMap(base_uv, u1, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw1 * SampleShadowMap(base_uv, u2, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw3 * vw1 * SampleShadowMap(base_uv, u3, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        sum += uw0 * vw2 * SampleShadowMap(base_uv, u0, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw2 * SampleShadowMap(base_uv, u1, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw2 * SampleShadowMap(base_uv, u2, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw3 * vw2 * SampleShadowMap(base_uv, u3, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        sum += uw0 * vw3 * SampleShadowMap(base_uv, u0, v3, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw3 * SampleShadowMap(base_uv, u1, v3, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw3 * SampleShadowMap(base_uv, u2, v3, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw3 * vw3 * SampleShadowMap(base_uv, u3, v3, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        return sum * 1.0f / 2704;

    #endif
}

//-------------------------------------------------------------------------------------------------
// Samples the appropriate shadow map cascade
//-------------------------------------------------------------------------------------------------
float3 SampleShadowCascade(in float3 shadowPosition, in float3 shadowPosDX,
                           in float3 shadowPosDY, in uint cascadeIdx,
                           in uint2 screenPos)
{
    shadowPosition += CascadeOffsets[cascadeIdx].xyz;
    shadowPosition *= CascadeScales[cascadeIdx].xyz;

    shadowPosDX *= CascadeScales[cascadeIdx].xyz;
    shadowPosDY *= CascadeScales[cascadeIdx].xyz;

    float3 cascadeColor = 1.0f;

    #if VisualizeCascades_
        const float3 CascadeColors[NumCascades] =
        {
            float3(1.0f, 0.0, 0.0f),
            float3(0.0f, 1.0f, 0.0f),
            float3(0.0f, 0.0f, 1.0f),
            float3(1.0f, 1.0f, 0.0f)
        };

        cascadeColor = CascadeColors[cascadeIdx];
    #endif

    #if UseEVSM_
        float shadow = SampleShadowMapEVSM(shadowPosition, shadowPosDX, shadowPosDY, cascadeIdx);
    #elif UseMSM_
        float shadow = SampleShadowMapMSM(shadowPosition, shadowPosDX, shadowPosDY, cascadeIdx);
    #elif ShadowMode_ == ShadowModeVSM_
        float shadow = SampleShadowMapVSM(shadowPosition, shadowPosDX, shadowPosDY, cascadeIdx);
    #elif ShadowMode_ == ShadowModeFixedSizePCF_
        float shadow = SampleShadowMapFixedSizePCF(shadowPosition, shadowPosDX, shadowPosDY, cascadeIdx);
    #elif ShadowMode_ == ShadowModeGridPCF_
        float shadow = SampleShadowMapGridPCF(shadowPosition, shadowPosDX, shadowPosDY, cascadeIdx);
    #elif ShadowMode_ == ShadowModeRandomDiscPCF_
        float shadow = SampleShadowMapRandomDiscPCF(shadowPosition, shadowPosDX, shadowPosDY, cascadeIdx, screenPos);
    #else //if ShadowMode_ == SampleShadowMapOptimizedPCF_
        float shadow = SampleShadowMapOptimizedPCF(shadowPosition, shadowPosDX, shadowPosDY, cascadeIdx);
    #endif

    return shadow * cascadeColor;
}

//-------------------------------------------------------------------------------------------------
// Calculates the offset to use for sampling the shadow map, based on the surface normal
//-------------------------------------------------------------------------------------------------
float3 GetShadowPosOffset(in float nDotL, in float3 normal)
{
    float2 shadowMapSize;
    float numSlices;
    ShadowMap.GetDimensions(shadowMapSize.x, shadowMapSize.y, numSlices);
    float texelSize = 2.0f / shadowMapSize.x;
    float nmlOffsetScale = saturate(1.0f - nDotL);
    return texelSize * OffsetScale * nmlOffsetScale * normal;
}

//-------------------------------------------------------------------------------------------------
// Computes the visibility term by performing the shadow test
//-------------------------------------------------------------------------------------------------
float3 ShadowVisibility(in float3 positionWS, in float depthVS, in float nDotL, in float3 normal,
                        in uint2 screenPos)
{
	float3 shadowVisibility = 1.0f;
	uint cascadeIdx = NumCascades - 1;
    float3 projectionPos = mul(float4(positionWS, 1.0f), ShadowMatrix).xyz;

	// Figure out which cascade to sample from
	[unroll]
	for(int i = NumCascades - 1; i >= 0; --i)
	{
        #if SelectFromProjection_
            // Select based on whether or not the pixel is inside the projection
            // used for rendering to the cascade
            float3 cascadePos = projectionPos + CascadeOffsets[i].xyz;
            cascadePos *= CascadeScales[i].xyz;
            cascadePos = abs(cascadePos - 0.5f);
            if(all(cascadePos <= 0.5f))
                cascadeIdx = i;
        #else
            // Select based on whether or not our view-space depth falls within
            // the depth range of a cascade split
            if(depthVS <= CascadeSplits[i])
                cascadeIdx = i;
        #endif
	}

    // Apply offset
    float3 offset = GetShadowPosOffset(nDotL, normal) / abs(CascadeScales[cascadeIdx].z);

    // Project into shadow space
    float3 samplePos = positionWS + offset;
	float3 shadowPosition = mul(float4(samplePos, 1.0f), ShadowMatrix).xyz;
    float3 shadowPosDX = ddx_fine(shadowPosition);
    float3 shadowPosDY = ddy_fine(shadowPosition);

	shadowVisibility = SampleShadowCascade(shadowPosition, shadowPosDX, shadowPosDY,
                                           cascadeIdx, screenPos);

    #if FilterAcrossCascades_
        // Sample the next cascade, and blend between the two results to
        // smooth the transition
        const float BlendThreshold = 0.1f;

        float nextSplit = CascadeSplits[cascadeIdx];
        float splitSize = cascadeIdx == 0 ? nextSplit : nextSplit - CascadeSplits[cascadeIdx - 1];
        float fadeFactor = (nextSplit - depthVS) / splitSize;

        #if SelectFromProjection_
            float3 cascadePos = projectionPos + CascadeOffsets[cascadeIdx].xyz;
            cascadePos *= CascadeScales[cascadeIdx].xyz;
            cascadePos = abs(cascadePos * 2.0f - 1.0f);
            float distToEdge = 1.0f - max(max(cascadePos.x, cascadePos.y), cascadePos.z);
            fadeFactor = max(distToEdge, fadeFactor);
        #endif

        [branch]
        if(fadeFactor <= BlendThreshold && cascadeIdx != NumCascades - 1)
        {
            float3 nextSplitVisibility = SampleShadowCascade(shadowPosition, shadowPosDX,
                                                             shadowPosDY, cascadeIdx + 1,
                                                             screenPos);
            float lerpAmt = smoothstep(0.0f, BlendThreshold, fadeFactor);
            shadowVisibility = lerp(nextSplitVisibility, shadowVisibility, lerpAmt);
        }
    #endif

	return shadowVisibility;
}

//=================================================================================================
// Pixel Shader
//=================================================================================================
float4 PS(in PSInput input) : SV_Target0
{
    // Normalize after interpolation
    float3 normalWS = normalize(input.NormalWS);

    float3 diffuseAlbedo = 1.0f;
    if(EnableAlbedoMap)
        diffuseAlbedo = DiffuseMap.Sample(AnisoSampler, input.TexCoord).xyz;

    float nDotL = saturate(dot(normalWS, LightDirection));
    uint2 screenPos = uint2(input.PositionSS.xy);
	float3 shadowVisibility = ShadowVisibility(input.PositionWS, input.DepthVS, nDotL,
                                               normalWS, screenPos);

	float3 lighting = 0.0f;

    // Add in the primary directional light
    lighting += nDotL * LightColor * diffuseAlbedo * (1.0f / 3.14159f) * shadowVisibility;

    lighting += float3(0.2f, 0.5f, 1.0f) * 0.1f * diffuseAlbedo;

    return float4(max(lighting, 0.0001f), 1.0f);
}