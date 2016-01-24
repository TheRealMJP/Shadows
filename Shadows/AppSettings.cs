enum ShadowMode
{
    [EnumLabel("Fixed Size PCF")]
    FixedSizePCF = 0,

    [EnumLabel("Grid PCF")]
    GridPCF,

    [EnumLabel("Random Disc PCF")]
    RandomDiscPCF,

    [EnumLabel("Optimized PCF")]
    OptimizedPCF,

    [EnumLabel("VSM")]
    VSM,

    [EnumLabel("EVSM 2 Component")]
    EVSM2,

    [EnumLabel("EVSM 4 Component")]
    EVSM4,

    [EnumLabel("MSM Hamburger")]
    MSMHamburger,

    [EnumLabel("MSM Hausdorff")]
    MSMHausdorff,
}

enum Scene
{
    PowerPlant = 0,
    Tower = 1,
    Columns = 2,
}

enum PartitionMode
{
    Manual = 0,
    Logarithmic = 1,
    PSSM = 2,
}

enum FixedFilterSize
{
    [EnumLabel("2x2")]
    Filter2x2 = 0,

    [EnumLabel("3x3")]
    Filter3x3,

    [EnumLabel("5x5")]
    Filter5x5,

    [EnumLabel("7x7")]
    Filter7x7,

    [EnumLabel("9x9")]
    Filter9x9,
}

enum ShadowMapSize
{
    [EnumLabel("512x512")]
    SMSize512 = 0,

    [EnumLabel("1024x1024")]
    SMSize1024,

    [EnumLabel("2048x2048")]
    SMSize2048,
}

enum ShadowMSAA
{
    [EnumLabel("None")]
    MSAANone = 0,

    [EnumLabel("2x")]
    MSAA2x,

    [EnumLabel("4x")]
    MSAA4x,

    [EnumLabel("8x")]
    MSAA8x,
}

enum SMFormat
{
    [EnumLabel("16-bit")]
    SM16Bit = 0,

    [EnumLabel("32-bit")]
    SM32Bit,
};

enum ShadowAnisotropy
{
    [EnumLabel("1x")]
    Anisotropy1x = 0,

    [EnumLabel("2x")]
    Anisotropy2x,

    [EnumLabel("4x")]
    Anisotropy4x,

    [EnumLabel("8x")]
    Anisotropy8x,

    [EnumLabel("16x")]
    Anisotropy16x,
};

public class Settings
{
    public class SceneControls
    {
        [DisplayName("Current Scene")]
        [HelpText("The scene to render")]
        [UseAsShaderConstant(false)]
        Scene CurrentScene = Scene.PowerPlant;

        [DisplayName("Animate Light")]
        [HelpText("Automatically rotates the light about the Y axis")]
        [UseAsShaderConstant(false)]
        bool AnimateLight = false;

        [DisplayName("Light Direction")]
        [HelpText("The direction of the light")]
        Direction LightDirection = new Direction(1.0f, 1.0f, 1.0f);

        [DisplayName("Light Color")]
        [HelpText("The color of the light")]
        [MinValue(0.0f)]
        [MaxValue(20.0f)]
        [StepSize(0.1f)]
        [HDR(true)]
        Color LightColor = new Color(10.0f, 8.0f, 5.0f);

        [DisplayName("Character Orientation")]
        [HelpText("The orientation of the character model")]
        [UseAsShaderConstant(false)]
        Orientation CharacterOrientation;

        [DisplayName("Enable Albedo Map")]
        [HelpText("Enables using albedo maps when rendering the scene")]
        bool EnableAlbedoMap = true;
    }

    public class Shadows
    {
        [DisplayName("Shadow Mode")]
        [HelpText("The shadow mapping technique to use")]
        [UseAsShaderConstant(false)]
        ShadowMode ShadowMode = ShadowMode.FixedSizePCF;

        [DisplayName("Shadow Map Size")]
        [HelpText("The size of the shadow map")]
        ShadowMapSize ShadowMapSize = ShadowMapSize.SMSize2048;

        [DisplayName("Fixed Filter Size")]
        [HelpText("Size of the PCF kernel used for Fixed Sized PCF shadow mode")]
        [UseAsShaderConstant(false)]
        FixedFilterSize FixedFilterSize = FixedFilterSize.Filter2x2;

        [DisplayName("Filter Size")]
        [MinValue(0.0f)]
        [MaxValue(100.0f)]
        [StepSize(0.1f)]
        [HelpText("Width of the filter kernel used for PCF or VSM filtering")]
        float FilterSize = 0.0f;

        [DisplayName("Visualize Cascades")]
        [HelpText("Colors each cascade a different color to visualize their start and end points")]
        [UseAsShaderConstant(false)]
        bool VisualizeCascades = false;

        [DisplayName("Stabilize Cascades")]
        [HelpText("Keeps consistent sizes for each cascade, and snaps each cascade so that they " +
                  "move in texel-sized increments. Reduces temporal aliasing artifacts, but " +
                  "reduces the effective resolution of the cascades")]
        bool StabilizeCascades = false;

        [DisplayName("Filter Across Cascades")]
        [HelpText("Enables blending across cascade boundaries to reduce the appearance of seams")]
        [UseAsShaderConstant(false)]
        bool FilterAcrossCascades = false;

        [DisplayName("Randomize Disc Offsets")]
        [HelpText("Applies a per-pixel random rotation to the sample locations when using disc PCF")]
        [UseAsShaderConstant(false)]
        bool RandomizeDiscOffsets = false;

        [DisplayName("Num Disc Samples")]
        [MinValue(1)]
        [MaxValue(64)]
        [HelpText("Number of samples to take when using randomized disc PCF")]
        int NumDiscSamples = 16;

        [DisplayName("Auto-Compute Depth Bounds")]
        [HelpText("Automatically fits the cascades to the min and max depth of the scene " +
                  "based on the contents of the depth buffer")]
        bool AutoComputeDepthBounds = false;

        [DisplayName("Depth Bounds Readback Latency")]
        [HelpText("Number of frames to wait before reading back the depth reduction results")]
        [MinValue(0)]
        [MaxValue(3)]
        [UseAsShaderConstant(false)]
        int ReadbackLatency = 1;

        [DisplayName("GPU Scene Submission")]
        [HelpText("Uses compute shaders to handle shadow setup and mesh batching to minimize draw calls " +
                  "and avoid depth readback latency")]
        [UseAsShaderConstant(false)]
        bool GPUSceneSubmission = false;

        [DisplayName("Min Cascade Distance")]
        [HelpText("The closest depth that is covered by the shadow cascades")]
        [MinValue(0.0f)]
        [MaxValue(0.1f)]
        [StepSize(0.001f)]
        float MinCascadeDistance = 0.0f;

        [DisplayName("Max Cascade Distance")]
        [HelpText("The furthest depth that is covered by the shadow cascades")]
        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.01f)]
        float MaxCascadeDistance = 1.0f;

        [DisplayName("CSM Partition Model")]
        [HelpText("Controls how the viewable depth range is partitioned into cascades")]
        PartitionMode PartitionMode = PartitionMode.Manual;

        [DisplayName("Split Distance 0")]
        [HelpText("Normalized distance to the end of the first cascade split")]
        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.01f)]
        float SplitDistance0 = 0.05f;

        [DisplayName("Split Distance 1")]
        [HelpText("Normalized distance to the end of the first cascade split")]
        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.01f)]
        float SplitDistance1 = 0.15f;

        [DisplayName("Split Distance 2")]
        [HelpText("Normalized distance to the end of the first cascade split")]
        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.01f)]
        float SplitDistance2 = 0.5f;

        [DisplayName("Split Distance 3")]
        [HelpText("Normalized distance to the end of the first cascade split")]
        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.01f)]
        float SplitDistance3 = 1.0f;

        [DisplayName("PSSM Lambda")]
        [HelpText("Lambda parameter used when PSSM mode is used for generated, " +
                  "blends between a linear and a logarithmic distribution")]
        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.01f)]
        float PSSMLambda = 1.0f;

        [DisplayName("Use Receiver Plane Depth Bias")]
        [HelpText("Automatically computes a bias value based on the slope of the receiver")]
        [UseAsShaderConstant(false)]
        bool UsePlaneDepthBias = true;

        [DisplayName("Bias")]
        [HelpText("Bias used for shadow map depth comparisons")]
        [MinValue(0.0f)]
        [MaxValue(0.01f)]
        [StepSize(0.0001f)]
        float Bias = 0.005f;

        [DisplayName("VSM Bias (x100)")]
        [HelpText("Bias used for VSM evaluation")]
        [MinValue(0.0f)]
        [MaxValue(100.0f)]
        [StepSize(0.001f)]
        float VSMBias = 0.01f;

        [DisplayName("Offset Scale")]
        [HelpText("Shadow receiver offset along the surface normal direction")]
        [MinValue(0.0f)]
        [MaxValue(100.0f)]
        [StepSize(0.1f)]
        float OffsetScale = 0.0f;

        [DisplayName("Shadow MSAA")]
        [HelpText("MSAA mode to use for VSM or MSM shadow maps")]
        [UseAsShaderConstant(false)]
        ShadowMSAA ShadowMSAA = ShadowMSAA.MSAANone;

        [DisplayName("VSM/MSM Format")]
        [HelpText("Texture format to use for VSM or <SM shadow maps")]
        SMFormat SMFormat = SMFormat.SM32Bit;

        [DisplayName("Shadow Anisotropy")]
        [HelpText("Level of anisotropic filtering to use when sampling VSM shadow maps")]
        [UseAsShaderConstant(false)]
        ShadowAnisotropy ShadowAnisotropy = ShadowAnisotropy.Anisotropy1x;

        [DisplayName("Enable Shadow Mip Maps")]
        [HelpText("Generates mip maps when using VSM or MSM")]
        [UseAsShaderConstant(false)]
        bool EnableShadowMips = false;

        [DisplayName("EVSM Positive Exponent")]
        [MinValue(0.0f)]
        [MaxValue(100.0f)]
        [StepSize(0.1f)]
        [HelpText("Exponent used for the positive EVSM warp")]
        float PositiveExponent = 40.0f;

        [DisplayName("EVSM Negative Exponent")]
        [MinValue(0.0f)]
        [MaxValue(100.0f)]
        [StepSize(0.1f)]
        [HelpText("Exponent used for the negative EVSM warp")]
        float NegativeExponent = 5.0f;

        [DisplayName("Light Bleeding Reduction")]
        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.01f)]
        [HelpText("Light bleeding reduction factor for VSM and EVSM shadows")]
        float LightBleedingReduction = 0.0f;

        [DisplayName("MSM Depth Bias (x1000)")]
        [MinValue(0.0f)]
        [MaxValue(100.0f)]
        [StepSize(0.001f)]
        [HelpText("Depth bias amount for MSM shadows")]
        float MSMDepthBias = 0.0f;

        [DisplayName("MSM Moment Bias (x1000)")]
        [MinValue(0.0f)]
        [MaxValue(100.0f)]
        [StepSize(0.001f)]
        [HelpText("Moment bias amount for MSM shadows")]
        float MSMMomentBias = 0.003f;
    }

    public class PostProcessing
    {
        [DisplayName("Bloom Exposure Offset")]
        [MinValue(0.0f)]
        [MaxValue(20.0f)]
        [StepSize(0.01f)]
        [HelpText("Exposure offset applied to generate the input of the bloom pass")]
        float BloomThreshold = 3.0f;

        [DisplayName("Bloom Magnitude")]
        [MinValue(0.0f)]
        [MaxValue(2.0f)]
        [StepSize(0.01f)]
        [HelpText("Scale factor applied to the bloom results when combined with tone-mapped result")]
        float BloomMagnitude = 1.0f;

        [DisplayName("Bloom Blur Sigma")]
        [MinValue(0.5f)]
        [MaxValue(1.5f)]
        [StepSize(0.01f)]
        [HelpText("Sigma parameter of the Gaussian filter used in the bloom pass")]
        float BloomBlurSigma = 0.8f;

        [DisplayName("Auto-Exposure Key Value")]
        [MinValue(0.0f)]
        [MaxValue(0.5f)]
        [StepSize(0.01f)]
        [HelpText("Parameter that biases the result of the auto-exposure pass")]
        float KeyValue = 0.115f;

        [DisplayName("Adaptation Rate")]
        [MinValue(0.0f)]
        [MaxValue(4.0f)]
        [StepSize(0.01f)]
        [HelpText("Controls how quickly auto-exposure adapts to changes in scene brightness")]
        float AdaptationRate = 0.5f;
    }
};