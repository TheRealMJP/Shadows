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
// Constants
//=================================================================================================
#define ShadowModeFixedSizePCF_ 0
#define ShadowModeGridPCF_ 1
#define ShadowModeRandomDiscPCF_ 2
#define ShadowModeOptimizedPCF_ 3
#define ShadowModeVSM_ 4
#define ShadowModeEVSM2_ 5
#define ShadowModeEVSM4_ 6
#define ShadowModeMSMHamburger_ 7
#define ShadowModeMSMHausdorff_ 8

#ifndef ShadowMode_
    #define ShadowMode_ 4
#endif

#if ShadowMode_ == ShadowModeEVSM2_ || ShadowMode_ == ShadowModeEVSM4_
    #define UseEVSM_ 1
#else
    #define UseEVSM_ 0
#endif

static const uint SMFormat16Bit = 0;
static const uint SMFormat32Bit = 1;

float2 GetEVSMExponents(in float positiveExponent, in float negativeExponent, in uint vsmFormat)
{
    const float maxExponent = vsmFormat == SMFormat16Bit ? 5.54f : 42.0f;

    float2 lightSpaceExponents = float2(positiveExponent, negativeExponent);

    // Clamp to maximum range of fp32/fp16 to prevent overflow/underflow
    return min(lightSpaceExponents, maxExponent);
}

// Applies exponential warp to shadow map depth, input depth should be in [0, 1]
float2 WarpDepth(float depth, float2 exponents)
{
    // Rescale depth into [-1, 1]
    depth = 2.0f * depth - 1.0f;
    float pos =  exp( exponents.x * depth);
    float neg = -exp(-exponents.y * depth);
    return float2(pos, neg);
}

float Linstep(float a, float b, float v)
{
    return saturate((v - a) / (b - a));
}

// Reduces VSM light bleedning
float ReduceLightBleeding(float pMax, float amount)
{
  // Remove the [0, amount] tail and linearly rescale (amount, 1].
   return Linstep(amount, 1.0f, pMax);
}

float ChebyshevUpperBound(float2 moments, float mean, float minVariance,
                          float lightBleedingReduction)
{
    // Compute variance
    float variance = moments.y - (moments.x * moments.x);
    variance = max(variance, minVariance);

    // Compute probabilistic upper bound
    float d = mean - moments.x;
    float pMax = variance / (variance + (d * d));

    pMax = ReduceLightBleeding(pMax, lightBleedingReduction);

    // One-tailed Chebyshev
    return (mean <= moments.x ? 1.0f : pMax);
}