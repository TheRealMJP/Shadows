//=================================================================================================
//
//	Shadows Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#if _WINDOWS

#pragma once

typedef Float2 float2;
typedef Float3 float3;
typedef Float4 float4;

typedef uint32 uint;
typedef Uint2 uint2;
typedef Uint3 uint3;
typedef Uint4 uint4;

#endif

// Constants
static const float Pi = 3.141592654f;
static const float Pi2 = 6.283185307f;
static const float Pi_2 = 1.570796327f;
static const float InvPi = 0.318309886f;
static const float InvPi2 = 0.159154943f;

static const uint ReductionTGSize = 16;
static const uint CullTGSize = 128;
static const uint BatchTGSize = 256;

static const uint NumCascades = 4;

static const float MaxKernelSize = 9.0f;

// Structures
struct DrawCall
{
    uint StartIndex;
    uint NumIndices;
    float3 SphereCenter;
    float SphereRadius;
};

struct CulledDraw
{
    uint SrcIndexStart;
    uint DstIndexStart;
    uint NumIndices;
};