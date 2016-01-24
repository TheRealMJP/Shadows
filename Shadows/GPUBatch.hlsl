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
cbuffer Constants : register(b0)
{
    float4 FrustumPlanes[6];
    uint NumDrawCalls;
    bool CullNearZ;
}

//=================================================================================================
// Resources
//=================================================================================================
StructuredBuffer<DrawCall> DrawCalls : register(t0);
RWByteAddressBuffer DrawArgsBuffer : SV_GroupIndex : register(u0);
AppendStructuredBuffer<CulledDraw> CulledDrawsOutput : register(u1);

StructuredBuffer<CulledDraw> CulledDraws : register(t0);
StructuredBuffer<uint> Indices : register(t1);
RWBuffer<uint> CulledIndices : register(u0);

// Clears an indirect args buffer to default values
[numthreads(1, 1, 1)]
void ClearArgsBuffer()
{
    DrawArgsBuffer.Store4(0, uint4(0, 1, 0, 0));
    DrawArgsBuffer.Store(16, 0);
}

// Checks if the bounding sphere of a draw call intersects with the view frustum
static bool IsVisible(in DrawCall drawCall)
{
    bool inFrustum = true;

    [unroll]
    for(uint i = 0; i < 5; ++i)
    {
        float d = dot(FrustumPlanes[i], float4(drawCall.SphereCenter, 1.0f));
        inFrustum = inFrustum && (d >= -drawCall.SphereRadius);
    }

    if(CullNearZ)
    {
        float d = dot(FrustumPlanes[5], float4(drawCall.SphereCenter, 1.0f));
        inFrustum = inFrustum && (d >= -drawCall.SphereRadius);
    }

    return inFrustum;
}

// Frustum culls each draw call, and allocates space for visible indices in the
// culled indices buffer. This uses one thread per draw call.
[numthreads(CullTGSize, 1, 1)]
void CullDrawCalls(in uint3 GroupID : SV_GroupID, in uint3 GroupThreadID : SV_GroupThreadID,
                   uint ThreadIndex : SV_GroupIndex)
{
    const uint drawIdx = CullTGSize * GroupID.x + ThreadIndex;
    if(drawIdx >= NumDrawCalls)
        return;

    DrawCall drawCall = DrawCalls[drawIdx];

    if(IsVisible(drawCall))
    {
        CulledDraw culledDraw;
        culledDraw.SrcIndexStart = drawCall.StartIndex;
        culledDraw.NumIndices = drawCall.NumIndices;
        DrawArgsBuffer.InterlockedAdd(0, drawCall.NumIndices, culledDraw.DstIndexStart);

        CulledDrawsOutput.Append(culledDraw);
    }
}

// Copies indices from visible draw calls to the combined index buffer. Thus uses one
// thread group per draw call, where the indices of a draw call are copied in parallel
// to the output buffer using the threads belonging to the thread group
[numthreads(CullTGSize, 1, 1)]
void BatchDrawCalls(in uint3 GroupID : SV_GroupID, in uint3 GroupThreadID : SV_GroupThreadID,
                    uint ThreadIndex : SV_GroupIndex)
{
    const uint drawIdx = GroupID.x;
    CulledDraw drawCall = CulledDraws[drawIdx];

    for(uint currIdx = ThreadIndex; currIdx < drawCall.NumIndices; currIdx += CullTGSize)
    {
        const uint srcIdx = currIdx + drawCall.SrcIndexStart;
        const uint dstIdx = currIdx + drawCall.DstIndexStart;
        CulledIndices[dstIdx] = Indices[srcIdx];
    }
}