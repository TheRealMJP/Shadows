#pragma once

#include "SampleFramework11/PCH.h"
#include "SampleFramework11/Settings.h"
#include "SampleFramework11/GraphicsTypes.h"

using namespace SampleFramework11;

enum class Scene
{
    PowerPlant = 0,
    Tower = 1,
    Columns = 2,

    NumValues
};

typedef EnumSettingT<Scene> SceneSetting;

enum class ShadowMode
{
    FixedSizePCF = 0,
    GridPCF = 1,
    RandomDiscPCF = 2,
    OptimizedPCF = 3,
    VSM = 4,
    EVSM2 = 5,
    EVSM4 = 6,
    MSMHamburger = 7,
    MSMHausdorff = 8,

    NumValues
};

typedef EnumSettingT<ShadowMode> ShadowModeSetting;

enum class ShadowMapSize
{
    SMSize512 = 0,
    SMSize1024 = 1,
    SMSize2048 = 2,

    NumValues
};

typedef EnumSettingT<ShadowMapSize> ShadowMapSizeSetting;

enum class FixedFilterSize
{
    Filter2x2 = 0,
    Filter3x3 = 1,
    Filter5x5 = 2,
    Filter7x7 = 3,
    Filter9x9 = 4,

    NumValues
};

typedef EnumSettingT<FixedFilterSize> FixedFilterSizeSetting;

enum class PartitionMode
{
    Manual = 0,
    Logarithmic = 1,
    PSSM = 2,

    NumValues
};

typedef EnumSettingT<PartitionMode> PartitionModeSetting;

enum class ShadowMSAA
{
    MSAANone = 0,
    MSAA2x = 1,
    MSAA4x = 2,
    MSAA8x = 3,

    NumValues
};

typedef EnumSettingT<ShadowMSAA> ShadowMSAASetting;

enum class SMFormat
{
    SM16Bit = 0,
    SM32Bit = 1,

    NumValues
};

typedef EnumSettingT<SMFormat> SMFormatSetting;

enum class ShadowAnisotropy
{
    Anisotropy1x = 0,
    Anisotropy2x = 1,
    Anisotropy4x = 2,
    Anisotropy8x = 3,
    Anisotropy16x = 4,

    NumValues
};

typedef EnumSettingT<ShadowAnisotropy> ShadowAnisotropySetting;

namespace AppSettings
{
    extern SceneSetting CurrentScene;
    extern BoolSetting AnimateLight;
    extern DirectionSetting LightDirection;
    extern ColorSetting LightColor;
    extern OrientationSetting CharacterOrientation;
    extern BoolSetting EnableAlbedoMap;
    extern ShadowModeSetting ShadowMode;
    extern ShadowMapSizeSetting ShadowMapSize;
    extern FixedFilterSizeSetting FixedFilterSize;
    extern FloatSetting FilterSize;
    extern BoolSetting VisualizeCascades;
    extern BoolSetting StabilizeCascades;
    extern BoolSetting FilterAcrossCascades;
    extern BoolSetting RandomizeDiscOffsets;
    extern IntSetting NumDiscSamples;
    extern BoolSetting AutoComputeDepthBounds;
    extern IntSetting ReadbackLatency;
    extern BoolSetting GPUSceneSubmission;
    extern FloatSetting MinCascadeDistance;
    extern FloatSetting MaxCascadeDistance;
    extern PartitionModeSetting PartitionMode;
    extern FloatSetting SplitDistance0;
    extern FloatSetting SplitDistance1;
    extern FloatSetting SplitDistance2;
    extern FloatSetting SplitDistance3;
    extern FloatSetting PSSMLambda;
    extern BoolSetting UsePlaneDepthBias;
    extern FloatSetting Bias;
    extern FloatSetting VSMBias;
    extern FloatSetting OffsetScale;
    extern ShadowMSAASetting ShadowMSAA;
    extern SMFormatSetting SMFormat;
    extern ShadowAnisotropySetting ShadowAnisotropy;
    extern BoolSetting EnableShadowMips;
    extern FloatSetting PositiveExponent;
    extern FloatSetting NegativeExponent;
    extern FloatSetting LightBleedingReduction;
    extern FloatSetting MSMDepthBias;
    extern FloatSetting MSMMomentBias;
    extern FloatSetting BloomThreshold;
    extern FloatSetting BloomMagnitude;
    extern FloatSetting BloomBlurSigma;
    extern FloatSetting KeyValue;
    extern FloatSetting AdaptationRate;

    struct AppSettingsCBuffer
    {
        Float3 LightDirection;
        Float4Align Float3 LightColor;
        bool32 EnableAlbedoMap;
        int32 ShadowMapSize;
        float FilterSize;
        bool32 StabilizeCascades;
        int32 NumDiscSamples;
        bool32 AutoComputeDepthBounds;
        float MinCascadeDistance;
        float MaxCascadeDistance;
        int32 PartitionMode;
        float SplitDistance0;
        float SplitDistance1;
        float SplitDistance2;
        float SplitDistance3;
        float PSSMLambda;
        float Bias;
        float VSMBias;
        float OffsetScale;
        int32 SMFormat;
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
    };

    extern ConstantBuffer<AppSettingsCBuffer> CBuffer;

    void Initialize(ID3D11Device* device);
    void UpdateCBuffer(ID3D11DeviceContext* context);
};

// ================================================================================================

namespace AppSettings
{
    static const uint64 NumFilterableShadowModes = uint64(ShadowMode::NumValues) - uint64(ShadowMode::VSM);

    inline uint32 FixedFilterKernelSize(uint32 value)
    {
        static const uint32 KernelSizes[] = { 2, 3, 5, 7, 9 };
        StaticAssert_(_countof(KernelSizes) >= uint64(FixedFilterSize::NumValues));
        return KernelSizes[value];
    }

    inline uint32 FixedFilterKernelSize()
    {
        return FixedFilterKernelSize(FixedFilterSize);
    }

    inline uint32 ShadowMapResolution(uint32 value)
    {
        static const uint32 Sizes[] = { 512, 1024, 2048 };
        StaticAssert_(_countof(Sizes) == uint64(ShadowMapSize::NumValues));
        return Sizes[value];
    }

    inline uint32 ShadowMapResolution()
    {
        return ShadowMapResolution(ShadowMapSize);
    }

    inline bool UseVSM(uint32 value)
    {
        return value >= uint32(ShadowMode::VSM) &&
               value <= uint32(ShadowMode::EVSM4);
    }

    inline bool UseVSM()
    {
        return UseVSM(ShadowMode);
    }

    inline bool UseEVSM(uint32 value)
    {
        return value == uint32(ShadowMode::EVSM2) ||
               value == uint32(ShadowMode::EVSM4);
    }

    inline bool UseEVSM()
    {
        return UseEVSM(ShadowMode);
    }

    inline bool UseMSM(uint32 value)
    {
        return value == uint32(ShadowMode::MSMHamburger) ||
               value == uint32(ShadowMode::MSMHausdorff);
    }

    inline bool UseMSM()
    {
        return UseMSM(ShadowMode);
    }

    inline bool UseFilterableShadows(uint32 value)
    {
        return UseVSM(value) || UseMSM(value);
    }

    inline bool UseFilterableShadows()
    {
        return UseFilterableShadows(ShadowMode);
    }

    inline uint32 MSAASamples(uint32 value)
    {
        static const uint32 Samples[] = { 1, 2, 4, 8 };
        StaticAssert_(_countof(Samples) == uint64(ShadowMSAA::NumValues));
        return Samples[value];
    }

    inline uint32 MSAASamples()
    {
        return MSAASamples(ShadowMSAA);
    }

    inline uint32 NumAnisotropicSamples(uint32 value)
    {
        const uint32 NumSamples[] = { 1, 2, 4, 8, 16 };
        StaticAssert_(_countof(NumSamples) == uint64(ShadowAnisotropy::NumValues));
        return NumSamples[value];
    }

    inline uint32 NumAnisotropicSamples()
    {
        return NumAnisotropicSamples(ShadowAnisotropy);
    }

    void Update();
}