cbuffer AppSettings : register(b7)
{
    float3 LightDirection;
    float3 LightColor;
    bool EnableAlbedoMap;
    int ShadowMapSize;
    float FilterSize;
    bool StabilizeCascades;
    int NumDiscSamples;
    bool AutoComputeDepthBounds;
    float MinCascadeDistance;
    float MaxCascadeDistance;
    int PartitionMode;
    float SplitDistance0;
    float SplitDistance1;
    float SplitDistance2;
    float SplitDistance3;
    float PSSMLambda;
    float Bias;
    float VSMBias;
    float OffsetScale;
    int SMFormat;
    float PositiveExponent;
    float NegativeExponent;
    float LightBleedingReduction;
    float MSMDepthBias;
    float MSMMomentBias;
    float BloomThreshold;
    float BloomMagnitude;
    float BloomBlurSigma;
    float KeyValue;
    float AdaptationRate;
}

static const int Scene_PowerPlant = 0;
static const int Scene_Tower = 1;
static const int Scene_Columns = 2;

static const int ShadowMode_FixedSizePCF = 0;
static const int ShadowMode_GridPCF = 1;
static const int ShadowMode_RandomDiscPCF = 2;
static const int ShadowMode_OptimizedPCF = 3;
static const int ShadowMode_VSM = 4;
static const int ShadowMode_EVSM2 = 5;
static const int ShadowMode_EVSM4 = 6;
static const int ShadowMode_MSMHamburger = 7;
static const int ShadowMode_MSMHausdorff = 8;

static const int ShadowMapSize_SMSize512 = 0;
static const int ShadowMapSize_SMSize1024 = 1;
static const int ShadowMapSize_SMSize2048 = 2;

static const int FixedFilterSize_Filter2x2 = 0;
static const int FixedFilterSize_Filter3x3 = 1;
static const int FixedFilterSize_Filter5x5 = 2;
static const int FixedFilterSize_Filter7x7 = 3;
static const int FixedFilterSize_Filter9x9 = 4;

static const int PartitionMode_Manual = 0;
static const int PartitionMode_Logarithmic = 1;
static const int PartitionMode_PSSM = 2;

static const int ShadowMSAA_MSAANone = 0;
static const int ShadowMSAA_MSAA2x = 1;
static const int ShadowMSAA_MSAA4x = 2;
static const int ShadowMSAA_MSAA8x = 3;

static const int SMFormat_SM16Bit = 0;
static const int SMFormat_SM32Bit = 1;

static const int ShadowAnisotropy_Anisotropy1x = 0;
static const int ShadowAnisotropy_Anisotropy2x = 1;
static const int ShadowAnisotropy_Anisotropy4x = 2;
static const int ShadowAnisotropy_Anisotropy8x = 3;
static const int ShadowAnisotropy_Anisotropy16x = 4;

