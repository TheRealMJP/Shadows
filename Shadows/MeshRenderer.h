//=================================================================================================
//
//	Shadows Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#pragma once

#include "SampleFramework11/PCH.h"

#include "SampleFramework11/Model.h"
#include "SampleFramework11/GraphicsTypes.h"
#include "SampleFramework11/DeviceStates.h"
#include "SampleFramework11/Camera.h"
#include "SampleFramework11/SH.h"
#include "SampleFramework11/ShaderCompilation.h"

#include "AppSettings.h"
#include "SharedConstants.h"

using namespace SampleFramework11;

// Represents a bouncing sphere for a MeshPart
struct Sphere
{
    XMFLOAT3 Center;
    float Radius;
};

// Represents the 6 planes of a frustum
Float4Align struct Frustum
{
    XMVECTOR Planes[6];
};

struct MeshData
{
    Model* Model;
    ID3D11BufferPtr PositionsVB;
    StructuredBuffer Indices;
    StructuredBuffer DrawCalls;
    StructuredBuffer CulledDraws;
    RWBuffer CulledIndices;

    std::vector<Sphere> BoundingSpheres;
    std::vector<uint32> FrustumTests;
    uint32 NumSuccessfulTests;

    std::vector<ID3D11InputLayoutPtr> InputLayouts;
    std::vector<ID3D11InputLayoutPtr> DepthInputLayouts;

    MeshData() : Model(NULL), NumSuccessfulTests(0) {}
};

class MeshRenderer
{

protected:

    // Constants
    static const uint32 MaxReadbackLatency = 4;
    static const uint32 MaxBlurRadius = 4;

public:

    MeshRenderer();

    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void SetSceneMesh(ID3D11DeviceContext* context, Model* model, const Float4x4& world);
    void SetCharacterMesh(ID3D11DeviceContext* context, Model* model, const Float4x4& world);

    void RenderDepthCPU(ID3D11DeviceContext* context, const Camera& camera, const Float4x4& world,
                        const Float4x4& characterWorld, bool shadowRendering);
    void RenderDepthGPU(ID3D11DeviceContext* context, const Camera& camera, const Float4x4& world,
                        const Float4x4& characterWorld, bool shadowRendering);

    void Render(ID3D11DeviceContext* context, const Camera& camera, const Float4x4& world,
                const Float4x4& characterWorld);

    void RenderShadowMap(ID3D11DeviceContext* context, const Camera& camera,
                         const Float4x4& world, const Float4x4& characterWorld);
    void RenderShadowMapGPU(ID3D11DeviceContext* context, const Camera& camera,
                            const Float4x4& world, const Float4x4& characterWorld);

    void RenderCascadeDebug(ID3D11DeviceContext* context, const Camera& camera, const Camera& cameraForShadows);

    void Update();

    void CreateReductionTargets(uint32 width, uint32 height);

    void ReduceDepth(ID3D11DeviceContext* context, ID3D11ShaderResourceView* depthTarget,
                     const Camera& camera);

    DepthStencilBuffer& ShadowMap() { return shadowMap; }
    ID3D11ShaderResourceView* ShadowMapCascadeSlice(uint32 cascadeIdx)
    {
        Assert_(cascadeIdx < NumCascades);
        return cascadeSlices[cascadeIdx];
    }

protected:

    void LoadShaders();
    void CreateShadowMaps();
    void ConvertToVSM(ID3D11DeviceContext* context, uint32 cascadeIdx,
                      Float3 cascadeScale, Float3 cascade0Scale);

    void SetupRenderDepthState(ID3D11DeviceContext* context, bool shadowRendering);

    void RenderDepthGPU(ID3D11DeviceContext* context, const Float4x4& world,
                        const Float4x4& characterWorld, bool shadowRendering,
                        ID3D11Buffer* viewProj, uint32 viewProjOffset,
                        ID3D11Buffer* frustumPlanes, uint32 planesOffset);

    void RenderModelDepthGPU(ID3D11DeviceContext* context, MeshData& meshData, bool shadowRendering,
                            const Float4x4& world, ID3D11Buffer* viewProj, uint32 viewProjOffset,
                            ID3D11Buffer* frustumPlanes, uint32 planeOffset);
    void RenderModelDepthCPU(ID3D11DeviceContext* context, const Camera& camera, const Float4x4& world,
                            MeshData& meshData);

    void RenderModel(ID3D11DeviceContext* context, const Camera& camera, const Float4x4& world,
                    MeshData& meshData);

    ID3D11DevicePtr device;

    BlendStates blendStates;
    RasterizerStates rasterizerStates;
    DepthStencilStates depthStencilStates;
    SamplerStates samplerStates;

    MeshData scene;
    MeshData character;

    ID3D11ShaderResourceViewPtr defaultTexture;

    DepthStencilBuffer shadowMap;
    RenderTarget2D  varianceShadowMap;
    RenderTarget2D tempVSM;
    ID3D11ShaderResourceViewPtr cascadeSlices[NumCascades];

    ID3D11ShaderResourceViewPtr randomRotations;

    ID3D11RasterizerStatePtr shadowRSState;
    ID3D11SamplerStatePtr evsmSamplers[uint64(ShadowAnisotropy::NumValues)];

    VertexShaderPtr meshVS;
    PixelShaderPtr meshPS;

    VertexShaderPtr meshDepthVS;

    VertexShaderPtr fullScreenVS;
    PixelShaderPtr vsmConvertPS[AppSettings::NumFilterableShadowModes][uint64(ShadowMSAA::NumValues)];
    PixelShaderPtr vsmBlurH[MaxBlurRadius + 1];
    PixelShaderPtr vsmBlurV[MaxBlurRadius + 1];
    PixelShaderPtr vsmBlurGPUH;
    PixelShaderPtr vsmBlurGPUV;

    PixelShaderPtr depthReductionInitialPS;
    PixelShaderPtr depthReductionPS;
    ComputeShaderPtr depthReductionInitialCS;
    ComputeShaderPtr depthReductionCS;
    std::vector<RenderTarget2D> depthReductionTargets;
    StagingTexture2D reductionStagingTextures[MaxReadbackLatency];
    uint32 currFrame;

    Float2 reductionDepth;

    ComputeShaderPtr clearArgsBuffer;
    ComputeShaderPtr cullDrawCalls;
    ComputeShaderPtr batchDrawCalls;
    RWBuffer drawArgsBuffer;
    ID3D11InputLayoutPtr depthGPUInputLayout;
    RWBuffer batchDispatchArgs;

    ComputeShaderPtr setupCascades;
    StructuredBuffer cascadeMatrixBuffer;
    StructuredBuffer cascadeSplitBuffer;
    StructuredBuffer cascadeOffsetBuffer;
    StructuredBuffer cascadeScaleBuffer;
    StructuredBuffer cascadePlanesBuffer;

    VertexShaderPtr drawFrustumVS;
    PixelShaderPtr drawFrustumPS;
    ID3D11BufferPtr frustumLineIB;
    uint32 frustumLineIndexCount = 0;
    Float4x4 invCascadeMats[NumCascades];

    // Constant buffers
    struct DepthOnlyConstants
    {
        Float4x4 World;
        Float4x4 ViewProjection;
    };

    struct MeshVSConstants
    {
        Float4x4 World;
        Float4x4 ViewProjection;
    };

    struct MeshPSConstants
    {
        Float4Align Float3 CameraPosWS;

        Float4Align Float4x4 ShadowMatrix;
        Float4Align float CascadeSplits[NumCascades];

        Float4Align Float4 CascadeOffsets[NumCascades];
        Float4Align Float4 CascadeScales[NumCascades];
    };

    struct VSMConstants
    {
        Float4 CascadeScale;
        Float4 Cascade0Scale;
        Float2 ShadowMapSize;
    };

    struct ReductionConstants
    {
        Float4x4 Projection;
        float NearClip;
        float FarClip;
    };

    struct GPUBatchConstants
    {
        Float4 FrustumPlanes[6];
        uint32 NumDrawCalls;
        bool32 CullNearZ;
    };

    struct ShadowSetupConstants
    {
        Float4x4 GlobalShadowMatrix;
        Float4x4 ViewProjInv;
        Float3 CameraRight;
        float CameraNearClip;
        float CameraFarClip;
    };

    struct FrustumConstants
    {
        Float4 FrustumPlanes[6];
    };

    struct DrawFrustumConstants
    {
        Float4x4 ViewProjection;
        Float4x4 InvFrustumViewProj;
        Float4 Color;
    };

    ConstantBuffer<DepthOnlyConstants> depthOnlyConstants;
    ConstantBuffer<MeshVSConstants> meshVSConstants;
    ConstantBuffer<MeshPSConstants> meshPSConstants;
    ConstantBuffer<VSMConstants> vsmConstants;
    ConstantBuffer<ReductionConstants> reductionConstants;
    ConstantBuffer<GPUBatchConstants> gpuBatchConstants;
    ConstantBuffer<ShadowSetupConstants> shadowSetupConstants;

    ConstantBuffer<Float4x4> tempViewProjBuffer;
    ConstantBuffer<FrustumConstants> tempFrustumPlanesBuffer;

    ConstantBuffer<DrawFrustumConstants> drawFrustumConstants;
};