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
#include "AppSettings.hlsl"

//=================================================================================================
// Constant buffers
//=================================================================================================
cbuffer SetupConstants : register(b0)
{
    float4x4 GlobalShadowMatrix;
    float4x4 ViewProjInv;
    float3 CameraRight;
    float CameraNearClip;
    float CameraFarClip;
}

//=================================================================================================
// Resources
//=================================================================================================
Texture2D<float2> DepthReductionOutput : register(t0);
RWStructuredBuffer<float4> ShadowRenderingMatrices : register(u0);
RWStructuredBuffer<float> CascadeSplits : register(u1);
RWStructuredBuffer<float4> CascadeOffsets : register(u2);
RWStructuredBuffer<float4> CascadeScales : register(u3);
RWStructuredBuffer<float4> CascadePlanes : register(u4);

float4x4 OrthographicProjection(float l, float b, float r,
                                float t, float zn, float zf)
{
    return float4x4(float4(2.0f / (r - l), 0, 0, 0),
                    float4(0, 2.0f / (t - b), 0, 0),
                    float4(0, 0, 1 / (zf - zn), 0),
                    float4((l + r) / (l - r), (t + b)/(b - t), zn / (zn - zf),  1));
}

float4x4 InverseRotationTranslation(in float3x3 r, in float3 t)
{
    float4x4 inv = float4x4(float4(r._11_21_31, 0.0f),
                            float4(r._12_22_32, 0.0f),
                            float4(r._13_23_33, 0.0f),
                            float4(0.0f, 0.0f, 0.0f, 1.0f));
    inv[3][0] = -dot(t, r[0]);
    inv[3][1] = -dot(t, r[1]);
    inv[3][2] = -dot(t, r[2]);
    return inv;
}

float4x4 InverseScaleTranslation(in float4x4 m)
{
    float4x4 inv = float4x4(float4(1.0f, 0.0f, 0.0f, 0.0f),
                            float4(0.0f, 1.0f, 0.0f, 0.0f),
                            float4(0.0f, 0.0f, 1.0f, 0.0f),
                            float4(0.0f, 0.0f, 0.0f, 1.0f));

    inv[0][0] = 1.0f / m[0][0];
    inv[1][1] = 1.0f / m[1][1];
    inv[2][2] = 1.0f / m[2][2];
    inv[3][0] = -m[3][0] * inv[0][0];
    inv[3][1] = -m[3][1] * inv[1][1];
    inv[3][2] = -m[3][2] * inv[2][2];

    return inv;
}

float4 PlaneFromPoints(in float3 point1, in float3 point2, in float3 point3)
{
    float3 v21 = point1 - point2;
    float3 v31 = point1 - point3;

    float3 n = normalize(cross(v21, v31));
    float d = -dot(n, point1);

    return float4(n, d);
}

[numthreads(NumCascades, 1, 1)]
void SetupCascades(in uint3 GroupID : SV_GroupID, in uint3 GroupThreadID : SV_GroupThreadID,
                   uint ThreadIndex : SV_GroupIndex)
{
    float sMapSize = 512.0f;
    if(ShadowMapSize == ShadowMapSize_SMSize1024)
        sMapSize = 1024.0f;
    else if(ShadowMapSize == ShadowMapSize_SMSize2048)
        sMapSize = 2048.0f;

    const float2 ReductionDepth = DepthReductionOutput[uint2(0, 0)];
    const float MinDistance = AutoComputeDepthBounds ? ReductionDepth.x : MinCascadeDistance;
    const float MaxDistance = AutoComputeDepthBounds ? ReductionDepth.y : MaxCascadeDistance;

    // Compute the split distances based on the partitioning mode
    float cascadeSplits[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    if(PartitionMode == PartitionMode_Manual)
    {
        cascadeSplits[0] = MinDistance + SplitDistance0 * MaxDistance;
        cascadeSplits[1] = MinDistance + SplitDistance1 * MaxDistance;
        cascadeSplits[2] = MinDistance + SplitDistance2 * MaxDistance;
        cascadeSplits[3] = MinDistance + SplitDistance3 * MaxDistance;
    }
    else if(PartitionMode == PartitionMode_Logarithmic
        ||  PartitionMode == PartitionMode_PSSM)
    {
        float lambda = 1.0f;
        if(PartitionMode == PartitionMode_PSSM)
            lambda = PSSMLambda;

        float nearClip = CameraNearClip;
        float farClip = CameraFarClip;
        float clipRange = farClip - nearClip;

        float minZ = nearClip + MinDistance * clipRange;
        float maxZ = nearClip + MaxDistance * clipRange;

        float range = maxZ - minZ;
        float ratio = maxZ / minZ;

        for(uint i = 0; i < NumCascades; ++i)
        {
            float p = (i + 1) / float(NumCascades);
            float logScale = minZ * pow(abs(ratio), p);
            float uniformScale = minZ + range * p;
            float d = lambda * (logScale - uniformScale) + uniformScale;
            cascadeSplits[i] = (d - nearClip) / clipRange;
        }
    }

    const uint cascadeIdx = ThreadIndex;

    // Get the 8 points of the view frustum in world space
    float3 frustumCornersWS[8] =
    {
        float3(-1.0f,  1.0f, 0.0f),
        float3( 1.0f,  1.0f, 0.0f),
        float3( 1.0f, -1.0f, 0.0f),
        float3(-1.0f, -1.0f, 0.0f),
        float3(-1.0f,  1.0f, 1.0f),
        float3( 1.0f,  1.0f, 1.0f),
        float3( 1.0f, -1.0f, 1.0f),
        float3(-1.0f, -1.0f, 1.0f),
    };

    float prevSplitDist = cascadeIdx == 0 ? MinDistance : cascadeSplits[cascadeIdx - 1];
    float splitDist = cascadeSplits[cascadeIdx];

    [unroll]
    for(uint i = 0; i < 8; ++i)
    {
        float4 corner = mul(float4(frustumCornersWS[i], 1.0f), ViewProjInv);
        frustumCornersWS[i] = corner.xyz / corner.w;
    }

    // Get the corners of the current cascade slice of the view frustum
    [unroll]
    for(i = 0; i < 4; ++i)
    {
        float3 cornerRay = frustumCornersWS[i + 4] - frustumCornersWS[i];
        float3 nearCornerRay = cornerRay * prevSplitDist;
        float3 farCornerRay = cornerRay * splitDist;
        frustumCornersWS[i + 4] = frustumCornersWS[i] + farCornerRay;
        frustumCornersWS[i] = frustumCornersWS[i] + nearCornerRay;
    }

    // Calculate the centroid of the view frustum slice
    float3 frustumCenter = 0.0f;
    [unroll]
    for(i = 0; i < 8; ++i)
        frustumCenter += frustumCornersWS[i];
    frustumCenter /= 8.0f;

    // Pick the up vector to use for the light camera
    float3 upDir = CameraRight;

    // This needs to be constant it to be stable
    if(StabilizeCascades)
        upDir = float3(0.0f, 1.0f, 0.0f);

    // Create a temporary view matrix for the light
    float3 lightCameraPos = frustumCenter;
    float3x3 lightCameraRot;
    lightCameraRot[2] = -LightDirection;
    lightCameraRot[0] = normalize(cross(upDir, lightCameraRot[2]));
    lightCameraRot[1] = cross(lightCameraRot[2], lightCameraRot[0]);
    float4x4 lightView = InverseRotationTranslation(lightCameraRot, lightCameraPos);

    float3 minExtents;
    float3 maxExtents;
    if(StabilizeCascades)
    {
        // Calculate the radius of a bounding sphere surrounding the frustum corners
        float sphereRadius = 0.0f;
        [unroll]
        for(i = 0; i < 8; ++i)
        {
            float dist = length(frustumCornersWS[i] - frustumCenter);
            sphereRadius = max(sphereRadius, dist);
        }

        sphereRadius = ceil(sphereRadius * 16.0f) / 16.0f;

        maxExtents = sphereRadius;
        minExtents = -maxExtents;
    }
    else
    {
        // Calculate an AABB around the frustum corners
        const float MaxFloat = 3.402823466e+38F;
        float3 mins = float3(MaxFloat, MaxFloat, MaxFloat);
        float3 maxes = float3(-MaxFloat, -MaxFloat, -MaxFloat);
        for(i = 0; i < 8; ++i)
        {
            float3 corner = mul(float4(frustumCornersWS[i], 1.0f), lightView).xyz;
            mins = min(mins, corner);
            maxes = max(maxes, corner);
        }

        minExtents = mins;
        maxExtents = maxes;

        // Adjust the min/max to accommodate the filtering size
        float scale = (sMapSize + 9.0f) / float(sMapSize);
        minExtents.x *= scale;
        minExtents.y *= scale;
        maxExtents.x *= scale;
        maxExtents.y *= scale;
    }

    float3 cascadeExtents = maxExtents - minExtents;

    // Get position of the shadow camera
    float3 shadowCameraPos = frustumCenter + LightDirection * -minExtents.z;

    // Come up with a new orthographic camera for the shadow caster
    float4x4 shadowView = InverseRotationTranslation(lightCameraRot, shadowCameraPos);
    float4x4 shadowProj = OrthographicProjection(minExtents.x, minExtents.y, maxExtents.x,
                                           maxExtents.y, 0.0f, cascadeExtents.z);

    if(StabilizeCascades)
    {
        // Create the rounding matrix, by projecting the world-space origin and determining
        // the fractional offset in texel space
        float4x4 shadowMatrix = mul(shadowView, shadowProj);
        float3 shadowOrigin = 0.0f;
        shadowOrigin = mul(float4(shadowOrigin, 1.0f), shadowMatrix).xyz;
        shadowOrigin *= (sMapSize / 2.0f);

        float3 roundedOrigin = round(shadowOrigin);
        float3 roundOffset = roundedOrigin - shadowOrigin;
        roundOffset = roundOffset * (2.0f / sMapSize);
        roundOffset.z = 0.0f;

        shadowProj[3][0] += roundOffset.x;
        shadowProj[3][1] += roundOffset.y;
    }

    float4x4 shadowRenderingMatrix = mul(shadowView, shadowProj);
    ShadowRenderingMatrices[cascadeIdx * 4 + 0] = shadowRenderingMatrix._11_21_31_41;
    ShadowRenderingMatrices[cascadeIdx * 4 + 1] = shadowRenderingMatrix._12_22_32_42;
    ShadowRenderingMatrices[cascadeIdx * 4 + 2] = shadowRenderingMatrix._13_23_33_43;
    ShadowRenderingMatrices[cascadeIdx * 4 + 3] = shadowRenderingMatrix._14_24_34_44;

    float4x4 invView = float4x4(float4(lightCameraRot[0], 0.0f),
                                float4(lightCameraRot[1], 0.0f),
                                float4(lightCameraRot[2], 0.0f),
                                float4(shadowCameraPos, 1.0f));
    float4x4 invProj = InverseScaleTranslation(shadowProj);

    float4x4 shadowRenderingInv = mul(invProj, invView);

    // Create frustum planes for the shadow rendering matrix
    float3 corners[8] =
    {
        float3( 1.0f, -1.0f, 0.0f),
        float3(-1.0f, -1.0f, 0.0f),
        float3( 1.0f,  1.0f, 0.0f),
        float3(-1.0f,  1.0f, 0.0f),
        float3( 1.0f, -1.0f, 1.0f),
        float3(-1.0f, -1.0f, 1.0f),
        float3( 1.0f,  1.0f, 1.0f),
        float3(-1.0f,  1.0f, 1.0f),
    };

    [unroll]
    for(i = 0; i < 8; ++i)
    {
        float4 corner = mul(float4(corners[i], 1.0f), shadowRenderingInv);
        corners[i] = corner.xyz / corner.w;
    }

    float4 frustumPlanes[6];
    frustumPlanes[0] = PlaneFromPoints(corners[0], corners[4], corners[2]);
    frustumPlanes[1] = PlaneFromPoints(corners[1], corners[3], corners[5]);
    frustumPlanes[2] = PlaneFromPoints(corners[3], corners[2], corners[7]);
    frustumPlanes[3] = PlaneFromPoints(corners[1], corners[5], corners[0]);
    frustumPlanes[4] = PlaneFromPoints(corners[5], corners[7], corners[4]);
    frustumPlanes[5] = PlaneFromPoints(corners[1], corners[0], corners[3]);

    [unroll]
    for(i = 0; i < 6; ++i)
        CascadePlanes[i + cascadeIdx * 6] = frustumPlanes[i];

    // Apply the scale/offset matrix, which transforms from [-1,1]
    // post-projection space to [0,1] UV space
    float4x4 texScaleBias = float4x4(float4(0.5f,  0.0f, 0.0f, 0.0f),
                                     float4(0.0f, -0.5f, 0.0f, 0.0f),
                                     float4(0.0f,  0.0f, 1.0f, 0.0f),
                                     float4(0.5f,  0.5f, 0.0f, 1.0f));
    float4x4 texScaleBiasInv = InverseScaleTranslation(texScaleBias);

    // Store the split distance in terms of view space depth
    const float clipDist = CameraFarClip - CameraNearClip;
    CascadeSplits[cascadeIdx] = CameraNearClip + splitDist * clipDist;

    // Calculate the position of the lower corner of the cascade partition, in the UV space
    // of the first cascade partition
    float4x4 invCascadeMat = mul(mul(texScaleBiasInv, invProj), invView);
    float3 cascadeCorner = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), invCascadeMat).xyz;
    cascadeCorner = mul(float4(cascadeCorner, 1.0f), GlobalShadowMatrix).xyz;

    // Do the same for the upper corner
    float3 otherCorner = mul(float4(1.0f, 1.0f, 1.0f, 1.0f), invCascadeMat).xyz;
    otherCorner = mul(float4(otherCorner, 1.0f), GlobalShadowMatrix).xyz;

    // Calculate the scale and offset
    float3 cascadeScale = 1.0f / (otherCorner - cascadeCorner);
    CascadeOffsets[cascadeIdx] = float4(-cascadeCorner, 0.0f);
    CascadeScales[cascadeIdx] = float4(cascadeScale, 1.0f);
}