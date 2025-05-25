//=================================================================================================
//
//	Shadows Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "PCH.h"

#include "MeshRenderer.h"
#include "AppSettings.h"
#include "SharedConstants.h"

#include "SampleFramework11/Exceptions.h"
#include "SampleFramework11/Utility.h"
#include "SampleFramework11/ShaderCompilation.h"
#include "SampleFramework11/App.h"
#include "SampleFramework11/Profiler.h"
#include "SampleFramework11/Settings.h"

// Constants
static const float ShadowNearClip = 1.0f;
static const bool UseComputeReduction = true;

// Finds the approximate smallest enclosing bounding sphere for a set of points. Based on
// "An Efficient Bounding Sphere", by Jack Ritter.
static Sphere ComputeBoundingSphereFromPoints(const XMFLOAT3* points, uint32 numPoints, uint32 stride)
{
    Sphere sphere;

    Assert_(numPoints > 0);
    Assert_(points);

    // Find the points with minimum and maximum x, y, and z
    XMVECTOR MinX, MaxX, MinY, MaxY, MinZ, MaxZ;

    MinX = MaxX = MinY = MaxY = MinZ = MaxZ = XMLoadFloat3(points);

    for(uint32 i = 1; i < numPoints; i++)
    {
        XMVECTOR Point = XMLoadFloat3((XMFLOAT3*)((BYTE*)points + i * stride));

        float px = XMVectorGetX(Point);
        float py = XMVectorGetY(Point);
        float pz = XMVectorGetZ(Point);

        if(px < XMVectorGetX(MinX))
            MinX = Point;

        if(px > XMVectorGetX(MaxX))
            MaxX = Point;

        if(py < XMVectorGetY(MinY))
            MinY = Point;

        if(py > XMVectorGetY(MaxY))
            MaxY = Point;

        if(pz < XMVectorGetZ(MinZ))
            MinZ = Point;

        if(pz > XMVectorGetZ(MaxZ))
            MaxZ = Point;
    }

    // Use the min/max pair that are farthest apart to form the initial sphere.
    XMVECTOR DeltaX = MaxX - MinX;
    XMVECTOR DistX = XMVector3Length(DeltaX);

    XMVECTOR DeltaY = MaxY - MinY;
    XMVECTOR DistY = XMVector3Length(DeltaY);

    XMVECTOR DeltaZ = MaxZ - MinZ;
    XMVECTOR DistZ = XMVector3Length(DeltaZ);

    XMVECTOR Center;
    XMVECTOR Radius;

    if(XMVector3Greater(DistX, DistY))
    {
        if(XMVector3Greater(DistX, DistZ))
        {
            // Use min/max x.
            Center = (MaxX + MinX) * 0.5f;
            Radius = DistX * 0.5f;
        }
        else
        {
            // Use min/max z.
            Center = (MaxZ + MinZ) * 0.5f;
            Radius = DistZ * 0.5f;
        }
    }
    else // Y >= X
    {
        if(XMVector3Greater(DistY, DistZ))
        {
            // Use min/max y.
            Center = (MaxY + MinY) * 0.5f;
            Radius = DistY * 0.5f;
        }
        else
        {
            // Use min/max z.
            Center = (MaxZ + MinZ) * 0.5f;
            Radius = DistZ * 0.5f;
        }
    }

    // Add any points not inside the sphere.
    for(uint32 i = 0; i < numPoints; i++)
    {
        XMVECTOR Point = XMLoadFloat3((XMFLOAT3*)((BYTE*)points + i * stride));

        XMVECTOR Delta = Point - Center;

        XMVECTOR Dist = XMVector3Length(Delta);

        if(XMVector3Greater(Dist, Radius))
        {
            // Adjust sphere to include the new point.
            Radius = (Radius + Dist) * 0.5f;
            Center += (XMVectorReplicate(1.0f) - Radius * XMVectorReciprocal(Dist)) * Delta;
        }
    }

    XMStoreFloat3(&sphere.Center, Center);
    XMStoreFloat(&sphere.Radius, Radius);

    return sphere;
}

// Calculates the inverse a camera's view * projection matrices
static Float4x4 CalculateInverseViewProj(const Camera& camera)
{
    Float4x4 invView = camera.WorldMatrix();
    Float4x4 invProj = Float4x4::Invert(camera.ProjectionMatrix());
    return invProj * invView;
}

// Calculates the frustum planes given a camera
static void ComputeFrustum(const Camera& camera, Frustum& frustum)
{
    XMMATRIX invViewProj = CalculateInverseViewProj(camera).ToSIMD();

    // Corners in homogeneous clip space
    XMVECTOR corners[8] =
    {                                               //                         7--------6
        XMVectorSet( 1.0f, -1.0f, 0.0f, 1.0f),      //                        /|       /|
        XMVectorSet(-1.0f, -1.0f, 0.0f, 1.0f),      //     Y ^               / |      / |
        XMVectorSet( 1.0f,  1.0f, 0.0f, 1.0f),      //     | _              3--------2  |
        XMVectorSet(-1.0f,  1.0f, 0.0f, 1.0f),      //     | /' Z           |  |     |  |
        XMVectorSet( 1.0f, -1.0f, 1.0f, 1.0f),      //     |/               |  5-----|--4
        XMVectorSet(-1.0f, -1.0f, 1.0f, 1.0f),      //     + ---> X         | /      | /
        XMVectorSet( 1.0f,  1.0f, 1.0f, 1.0f),      //                      |/       |/
        XMVectorSet(-1.0f,  1.0f, 1.0f, 1.0f),      //                      1--------0
    };

    // Convert to world space
    for(uint32 i = 0; i < 8; ++i)
        corners[i] = XMVector3TransformCoord(corners[i], invViewProj);

    // Calculate the 6 planes
    frustum.Planes[0] = XMPlaneFromPoints(corners[0], corners[4], corners[2]);
    frustum.Planes[1] = XMPlaneFromPoints(corners[1], corners[3], corners[5]);
    frustum.Planes[2] = XMPlaneFromPoints(corners[3], corners[2], corners[7]);
    frustum.Planes[3] = XMPlaneFromPoints(corners[1], corners[5], corners[0]);
    frustum.Planes[4] = XMPlaneFromPoints(corners[5], corners[7], corners[4]);
    frustum.Planes[5] = XMPlaneFromPoints(corners[1], corners[0], corners[3]);
}

// Tests a frustum for intersection with a sphere
static uint32 TestFrustumSphere(const Frustum& frustum, const Sphere& sphere, bool ignoreNearZ)
{
    XMVECTOR sphereCenter = XMLoadFloat3(&sphere.Center);

    uint32 result = 1;
    uint32 numPlanes = ignoreNearZ ? 5 : 6;
    for(uint32 i = 0; i < numPlanes; i++) {
        float distance = XMVectorGetX(XMPlaneDotCoord(frustum.Planes[i], sphereCenter));

        if (distance < -sphere.Radius)
            return 0;
        else if (distance < sphere.Radius)
            result =  1;
    }

    return result;
}

// Calculates the bounding sphere for each MeshPart
static void ComputeBoundingSpheres(ID3D11Device* device, ID3D11DeviceContext* context,
                                   const Float4x4& world, Model* model,
                                   std::vector<Sphere>& boundingSpheres)
{
    boundingSpheres.clear();

    for(uint32 meshIdx = 0; meshIdx < model->Meshes().size(); ++meshIdx)
    {
        Mesh& mesh = model->Meshes()[meshIdx];

        // Create staging buffers for copying the vertex/index data to
        ID3D11BufferPtr stagingVB;
        ID3D11BufferPtr stagingIB;

        D3D11_BUFFER_DESC bufferDesc;
        bufferDesc.BindFlags = 0;
        bufferDesc.ByteWidth = mesh.NumVertices() * mesh.VertexStride();
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;
        bufferDesc.Usage = D3D11_USAGE_STAGING;
        DXCall(device->CreateBuffer(&bufferDesc, nullptr, &stagingVB));

        uint32 indexSize = mesh.IndexBufferType() == Mesh::Index16Bit ? 2 : 4;
        bufferDesc.ByteWidth = mesh.NumIndices() * indexSize;
        DXCall(device->CreateBuffer(&bufferDesc, nullptr, &stagingIB));

        context->CopyResource(stagingVB, mesh.VertexBuffer());
        context->CopyResource(stagingIB, mesh.IndexBuffer());

        D3D11_MAPPED_SUBRESOURCE mapped;
        context->Map(stagingVB, 0, D3D11_MAP_READ, 0, &mapped);
        const BYTE* verts = reinterpret_cast<const BYTE*>(mapped.pData);
        uint32 stride = mesh.VertexStride();

        context->Map(stagingIB, 0, D3D11_MAP_READ, 0, &mapped);
        const BYTE* indices = reinterpret_cast<const BYTE*>(mapped.pData);
        const uint32* indices32 = reinterpret_cast<const uint32*>(mapped.pData);
        const WORD* indices16 = reinterpret_cast<const WORD*>(mapped.pData);

        for(uint32 partIdx = 0; partIdx < mesh.MeshParts().size(); ++partIdx)
        {
            const MeshPart& part = mesh.MeshParts()[partIdx];

            std::vector<Float3> points;
            for(uint32 i = 0; i < part.IndexCount; ++i)
            {
                uint32 index = indexSize == 2 ? indices16[part.IndexStart + i] : indices32[part.IndexStart + i];
                Float3 point = *reinterpret_cast<const Float3*>(verts + (index * stride));
                point = Float3::Transform(point, world);
                points.push_back(point);
            }

            Sphere sphere = ComputeBoundingSphereFromPoints(&points[0], static_cast<uint32>(points.size()), sizeof(XMFLOAT3));
            boundingSpheres.push_back(sphere);
        }

        context->Unmap(stagingVB, 0);
        context->Unmap(stagingIB, 0);
    }
}

// Makes the "global" shadow matrix used as the reference point for the cascades
static Float4x4 MakeGlobalShadowMatrix(const Camera& camera)
{
    // Get the 8 points of the view frustum in world space
    Float3 frustumCorners[8] =
    {
        Float3(-1.0f,  1.0f, 0.0f),
        Float3( 1.0f,  1.0f, 0.0f),
        Float3( 1.0f, -1.0f, 0.0f),
        Float3(-1.0f, -1.0f, 0.0f),
        Float3(-1.0f,  1.0f, 1.0f),
        Float3( 1.0f,  1.0f, 1.0f),
        Float3( 1.0f, -1.0f, 1.0f),
        Float3(-1.0f, -1.0f, 1.0f),
    };

    Float4x4 invViewProj = CalculateInverseViewProj(camera);
    Float3 frustumCenter = 0.0f;
    for(uint64 i = 0; i < 8; ++i)
    {
        frustumCorners[i] = Float3::Transform(frustumCorners[i], invViewProj);
        frustumCenter += frustumCorners[i];
    }

    frustumCenter /= 8.0f;

    // Pick the up vector to use for the light camera
    Float3 upDir = Float3(0.0f, 1.0f, 0.0f);

    // Create a temporary view matrix for the light
    Float3 lightCameraPos = frustumCenter;
    Float3 lookAt = frustumCenter - AppSettings::LightDirection;
    Float4x4 lightView = XMMatrixLookAtLH(lightCameraPos.ToSIMD(), lookAt.ToSIMD(), upDir.ToSIMD());

    // Get position of the shadow camera
    Float3 shadowCameraPos = frustumCenter + AppSettings::LightDirection.Value() * -0.5f;

    // Come up with a new orthographic camera for the shadow caster
    OrthographicCamera shadowCamera(-0.5f, -0.5f, 0.5f,
                                    0.5f, 0.0f, 1.0f);
    shadowCamera.SetLookAt(shadowCameraPos, frustumCenter, upDir);

    Float4x4 texScaleBias = Float4x4::ScaleMatrix(Float3(0.5f, -0.5f, 1.0f));
    texScaleBias.SetTranslation(Float3(0.5f, 0.5f, 0.0f));
    return shadowCamera.ViewProjectionMatrix() * texScaleBias;
}

MeshRenderer::MeshRenderer() : currFrame(0)
{
}

static PixelShaderPtr CompileMeshPS(ID3D11Device* device)
{
    CompileOptions opts;
    opts.Add("VisualizeCascades_", AppSettings::VisualizeCascades);
    opts.Add("UsePlaneDepthBias_", AppSettings::UsePlaneDepthBias);
    opts.Add("FilterAcrossCascades_", AppSettings::FilterAcrossCascades);
    opts.Add("FilterSize_", AppSettings::FixedFilterKernelSize());
    opts.Add("ShadowMode_", uint32(AppSettings::ShadowMode));
    opts.Add("RandomizeOffsets_", AppSettings::RandomizeDiscOffsets);
    opts.Add("SelectFromProjection_", AppSettings::CascadeSelectionMode == CascadeSelectionModes::Projection ? 1 : 0);
    return CompilePSFromFile(device, L"Mesh.hlsl", "PS", "ps_5_0", opts);
}

// Loads all shaders
void MeshRenderer::LoadShaders()
{
    // Load the mesh shaders
    meshDepthVS = CompileVSFromFile(device, L"DepthOnly.hlsl", "VS", "vs_5_0");
    meshVS = CompileVSFromFile(device, L"Mesh.hlsl", "VS", "vs_5_0");
    meshPS = CompileMeshPS(device);

    fullScreenVS = CompileVSFromFile(device, L"VSMConvert.hlsl", "FullScreenVS");
    for(uint32 shadowMode = uint32(ShadowMode::VSM); shadowMode < uint32(ShadowMode::NumValues); ++shadowMode)
    {
        for(uint32 msaaMode = 0; msaaMode < uint32(ShadowMSAA::NumValues); ++msaaMode)
        {
            PixelShaderPtr& shader = vsmConvertPS[shadowMode - uint32(ShadowMode::VSM)][msaaMode];

            CompileOptions opts;
            opts.Add("ShadowMode_", shadowMode);
            opts.Add("MSAASamples_", AppSettings::MSAASamples(msaaMode));
            shader = CompilePSFromFile(device, L"VSMConvert.hlsl", "ConvertToVSM", "ps_5_0", opts);
        }
    }

    for(uint32 i = 0; i <= MaxBlurRadius; ++i)
    {

        CompileOptions opts;
        opts.Add("Horizontal_", 1);
        opts.Add("Vertical_", 0);
        opts.Add("SampleRadius_", i);
        opts.Add("GPUSceneSubmission_", 0);
        vsmBlurH[i] = CompilePSFromFile(device, L"VSMConvert.hlsl", "BlurVSM", "ps_5_0", opts);


        opts.Reset();
        opts.Add("Horizontal_", 0);
        opts.Add("Vertical_", 1);
        opts.Add("SampleRadius_", i);
        opts.Add("GPUSceneSubmission_", 0);
        vsmBlurV[i] = CompilePSFromFile(device, L"VSMConvert.hlsl", "BlurVSM", "ps_5_0", opts);
    }


    CompileOptions opts;
    opts.Add("Horizontal_", 1);
    opts.Add("Vertical_", 0);
    opts.Add("GPUSceneSubmission_", 1);
    vsmBlurGPUH = CompilePSFromFile(device, L"VSMConvert.hlsl", "BlurVSM", "ps_5_0", opts);


    opts.Reset();
    opts.Add("Horizontal_", 0);
    opts.Add("Vertical_", 1);
    opts.Add("GPUSceneSubmission_", 1);
    vsmBlurGPUV = CompilePSFromFile(device, L"VSMConvert.hlsl", "BlurVSM", "ps_5_0", opts);


    depthReductionInitialPS = CompilePSFromFile(device, L"DepthReduction.hlsl", "DepthReductionInitialPS");
    depthReductionPS = CompilePSFromFile(device, L"DepthReduction.hlsl", "DepthReductionPS");


    opts.Reset();
    opts.Add("CS_", 1);
    depthReductionInitialCS = CompileCSFromFile(device, L"DepthReduction.hlsl", "DepthReductionInitialCS",
                                                "cs_5_0", opts);

    depthReductionCS = CompileCSFromFile(device, L"DepthReduction.hlsl", "DepthReductionCS",
                                         "cs_5_0", opts);

    clearArgsBuffer = CompileCSFromFile(device, L"GPUBatch.hlsl", "ClearArgsBuffer");
    cullDrawCalls = CompileCSFromFile(device, L"GPUBatch.hlsl", "CullDrawCalls");
    batchDrawCalls = CompileCSFromFile(device, L"GPUBatch.hlsl", "BatchDrawCalls");

    setupCascades = CompileCSFromFile(device, L"SetupShadows.hlsl", "SetupCascades");

    drawFrustumVS = CompileVSFromFile(device, L"DrawFrustum.hlsl", "VSMain");
    drawFrustumPS = CompilePSFromFile(device, L"DrawFrustum.hlsl", "PSMain");
}

// Creates shadow map render targets and depth targets
void MeshRenderer::CreateShadowMaps()
{
    // Create the shadow map as a texture atlas
    const uint32 ShadowMapSize = AppSettings::ShadowMapResolution();

    DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;
    if(AppSettings::DepthBufferFormat == DepthBufferFormats::DB16Unorm)
        depthFormat = DXGI_FORMAT_D16_UNORM;
    else if(AppSettings::DepthBufferFormat == DepthBufferFormats::DB24Unorm)
        depthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    if(AppSettings::UseFilterableShadows())
    {
        uint32 msaaSamples = AppSettings::MSAASamples();
        shadowMap.Initialize(device, ShadowMapSize, ShadowMapSize, depthFormat, true, msaaSamples, 0, 1);

        DXGI_FORMAT smFmt;
        if(AppSettings::ShadowMode == ShadowMode::EVSM4)
        {
            if(AppSettings::SMFormat == SMFormat::SM16Bit)
                smFmt = DXGI_FORMAT_R16G16B16A16_FLOAT;
            else
                smFmt = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        else if(AppSettings::ShadowMode == ShadowMode::EVSM2)
        {
            if(AppSettings::SMFormat == SMFormat::SM16Bit)
                smFmt = DXGI_FORMAT_R16G16_FLOAT;
            else
                smFmt = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if(AppSettings::ShadowMode == ShadowMode::MSMHamburger || AppSettings::ShadowMode == ShadowMode::MSMHausdorff)
        {
            if(AppSettings::SMFormat == SMFormat::SM16Bit)
                smFmt = DXGI_FORMAT_R16G16B16A16_UNORM;
            else
                smFmt = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        else
        {
            if(AppSettings::SMFormat == SMFormat::SM16Bit)
                smFmt = DXGI_FORMAT_R16G16_UNORM;
            else
                smFmt = DXGI_FORMAT_R32G32_FLOAT;
        }

        uint32 numMips = AppSettings::EnableShadowMips ? 0 : 1;
        varianceShadowMap.Initialize(device, ShadowMapSize, ShadowMapSize, smFmt, numMips, 1, 0,
                                     AppSettings::EnableShadowMips, false, NumCascades, false);

        tempVSM.Initialize(device, ShadowMapSize, ShadowMapSize, smFmt, 1, 1, 0, false, false, 1, false);
    }
    else
    {
        shadowMap.Initialize(device, ShadowMapSize, ShadowMapSize, depthFormat, true, 1, 0, NumCascades);
        varianceShadowMap = RenderTarget2D();
    }

    for(uint32 cascadeIdx = 0; cascadeIdx < NumCascades; ++cascadeIdx)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = { };
        shadowMap.SRView->GetDesc(&srvDesc);
        srvDesc.Texture2DArray.ArraySize = 1;
        srvDesc.Texture2DArray.FirstArraySlice = cascadeIdx;
        device->CreateShaderResourceView(shadowMap.Texture, &srvDesc, &cascadeSlices[cascadeIdx]);
    }
}

// Creates bounding spheres for a mesh, and creates resources used for GPU batching
static void SetupMesh(ID3D11Device* device, ID3D11DeviceContext* context, Model* model,
                      MeshData& meshData, const Float4x4& world, VertexShaderPtr meshVS,
                      VertexShaderPtr meshDepthVS)
{
    meshData.Model = model;

    ComputeBoundingSpheres(device, context, world, model, meshData.BoundingSpheres);

    std::vector<Float3> positions;
    std::vector<uint32> indices;
    std::vector<DrawCall> drawCalls;
    uint64 drawIdx = 0;
    for(uint64 meshIdx = 0; meshIdx < model->Meshes().size(); ++meshIdx)
    {
        const Mesh& mesh = model->Meshes()[meshIdx];
        const uint8* verts = mesh.Vertices();

        const uint32 vtxOffset = uint32(positions.size());
        for(uint64 i = 0; i < mesh.NumVertices(); ++i)
        {
            positions.push_back(*reinterpret_cast<const Float3*>(verts));
            verts += mesh.VertexStride();
        }

        const uint32 idxOffset = uint32(indices.size());
        for(uint32 i = 0; i < mesh.NumIndices(); ++i)
        {
            uint32 idx = GetIndex(mesh.Indices(), i, mesh.IndexSize());
            idx += vtxOffset;
            indices.push_back(idx);
        }

        for(uint64 partIdx = 0; partIdx < mesh.MeshParts().size(); ++partIdx)
        {
            const MeshPart& part = mesh.MeshParts()[partIdx];
            DrawCall drawCall;
            drawCall.StartIndex = part.IndexStart + idxOffset;
            drawCall.NumIndices = part.IndexCount;
            drawCall.SphereCenter = meshData.BoundingSpheres[drawIdx].Center;
            drawCall.SphereRadius= meshData.BoundingSpheres[drawIdx].Radius;
            drawCalls.push_back(drawCall);

            ++drawIdx;
        }
    }

    meshData.Indices.Initialize(device, sizeof(uint32), uint32(indices.size()), false, false, false, indices.data());
    meshData.CulledIndices.Initialize(device, DXGI_FORMAT_R32_UINT, 4, uint32(indices.size()), false, false, true);
    meshData.DrawCalls.Initialize(device, sizeof(DrawCall), uint32(drawCalls.size()), false, false, false, drawCalls.data());
    meshData.CulledDraws.Initialize(device, sizeof(CulledDraw), uint32(drawCalls.size()), true, true, false, nullptr);

    D3D11_BUFFER_DESC vbDesc;
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.ByteWidth = uint32(positions.size() * sizeof(Float3));
    vbDesc.CPUAccessFlags = 0;
    vbDesc.MiscFlags = 0;
    vbDesc.StructureByteStride = 0;
    D3D11_SUBRESOURCE_DATA vbInitData = { positions.data(), 0, 0 };
    DXCall(device->CreateBuffer(&vbDesc, &vbInitData, &meshData.PositionsVB));

    meshData.InputLayouts.clear();
    meshData.DepthInputLayouts.clear();

    for(uint32 i = 0; i < model->Meshes().size(); ++i)
    {
        Mesh& mesh = model->Meshes()[i];
        ID3D11InputLayoutPtr inputLayout;
        DXCall(device->CreateInputLayout(mesh.InputElements(), mesh.NumInputElements(),
                                         meshVS->ByteCode->GetBufferPointer(),
                                         meshVS->ByteCode->GetBufferSize(), &inputLayout));
        meshData.InputLayouts.push_back(inputLayout);

        DXCall(device->CreateInputLayout(mesh.InputElements(), mesh.NumInputElements(),
                                         meshDepthVS->ByteCode->GetBufferPointer(),
                                         meshDepthVS->ByteCode->GetBufferSize(), &inputLayout));
        meshData.DepthInputLayouts.push_back(inputLayout);
    }
}

void MeshRenderer::SetSceneMesh(ID3D11DeviceContext* context, Model* model, const Float4x4& world)
{
    SetupMesh(device, context, model, scene, world, meshVS, meshDepthVS);
}

void MeshRenderer::SetCharacterMesh(ID3D11DeviceContext* context, Model* model, const Float4x4& world)
{
    SetupMesh(device, context, model, character, world, meshVS, meshDepthVS);
}

// Loads resources
void MeshRenderer::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
    this->device = device;

    blendStates.Initialize(device);
    rasterizerStates.Initialize(device);
    depthStencilStates.Initialize(device);
    samplerStates.Initialize(device);

    depthOnlyConstants.Initialize(device, true);
    meshVSConstants.Initialize(device);
    meshPSConstants.Initialize(device, true);
    vsmConstants.Initialize(device, true);
    reductionConstants.Initialize(device);
    gpuBatchConstants.Initialize(device, true);
    shadowSetupConstants.Initialize(device);

    tempViewProjBuffer.Initialize(device);
    tempFrustumPlanesBuffer.Initialize(device);

    drawFrustumConstants.Initialize(device);

    defaultTexture = LoadTexture(device, L"..\\Content\\Textures\\Default.dds");

    LoadShaders();

    D3D11_RASTERIZER_DESC rsDesc = RasterizerStates::NoCullDesc();
    rsDesc.DepthClipEnable = FALSE;
    DXCall(device->CreateRasterizerState(&rsDesc, &shadowRSState));

    for(uint32 anisotropy = 0; anisotropy < uint32(ShadowAnisotropy::NumValues); ++anisotropy)
    {
        D3D11_SAMPLER_DESC sampDesc = SamplerStates::AnisotropicDesc();
        sampDesc.MaxAnisotropy = AppSettings::NumAnisotropicSamples(anisotropy);
        DXCall(device->CreateSamplerState(&sampDesc, &evsmSamplers[anisotropy]));
    }

    // Initialize a 64x64 texture containing random rotation values
    static const uint32 RandomTextureSize = 64;
    BYTE randomValues[RandomTextureSize * RandomTextureSize];
    srand(0);

    for(uint32 i = 0; i < RandomTextureSize * RandomTextureSize; ++i)
        randomValues[i] = static_cast<BYTE>(RandFloat() * 255.0f);

    D3D11_TEXTURE2D_DESC texDesc = { };
    texDesc.Width = RandomTextureSize;
    texDesc.Height = RandomTextureSize;
    texDesc.Format = DXGI_FORMAT_R8_UNORM;
    texDesc.ArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.MiscFlags = 0;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = { };
    initData.pSysMem = randomValues;
    initData.SysMemPitch = RandomTextureSize;
    initData.SysMemSlicePitch = 0;

    ID3D11Texture2DPtr randomValuesTexture;
    DXCall(device->CreateTexture2D(&texDesc, &initData, &randomValuesTexture));
    DXCall(device->CreateShaderResourceView(randomValuesTexture, nullptr, &randomRotations));

    // Create the staging textures for reading back the reduced depth buffer
    for(uint32 i = 0; i < MaxReadbackLatency; ++i)
        reductionStagingTextures[i].Initialize(device, 1, 1, DXGI_FORMAT_R16G16_UNORM);

    // Create resources needed for GPU draw call batching
    uint32 drawArgsInit[5] = { 0, 1, 0, 0, 0 };
    drawArgsBuffer.Initialize(device, DXGI_FORMAT_R32_TYPELESS, 4, 5, true, false, false, true, drawArgsInit);

    uint32 dispatchArgsInit[4] = { 1, 1, 1, 0 };
    batchDispatchArgs.Initialize(device, DXGI_FORMAT_R32_TYPELESS, 4, 4, true, false, false, true, dispatchArgsInit);

    D3D11_INPUT_ELEMENT_DESC inputElements[1] = { };
    inputElements[0].AlignedByteOffset = 0;
    inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElements[0].InputSlot = 0;
    inputElements[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    inputElements[0].InstanceDataStepRate = 0;
    inputElements[0].SemanticName = "POSITION";
    inputElements[0].SemanticIndex = 0;
    DXCall(device->CreateInputLayout(inputElements, 1, meshDepthVS->ByteCode->GetBufferPointer(),
                                     meshDepthVS->ByteCode->GetBufferSize(), &depthGPUInputLayout));

    // Create resources for GPU cascade setup
    cascadeMatrixBuffer.Initialize(device, sizeof(Float4), NumCascades * 4, true);
    cascadeSplitBuffer.Initialize(device, sizeof(float), NumCascades, true);
    cascadeOffsetBuffer.Initialize(device, sizeof(Float4), NumCascades, true);
    cascadeScaleBuffer.Initialize(device, sizeof(Float4), NumCascades, true);
    cascadePlanesBuffer.Initialize(device, sizeof(Float4), NumCascades * 6, true);

    {
        const uint16 frustumIndices[] =
        {
            0, 1,
            1, 3,
            3, 2,
            2, 0,

            0, 4,
            1, 5,
            2, 6,
            3, 7,

            4, 5,
            5, 7,
            7, 6,
            6, 4,
        };

        D3D11_BUFFER_DESC ibDesc = { };
        ibDesc.ByteWidth = sizeof(frustumIndices);
        ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibDesc.CPUAccessFlags = 0;
        ibDesc.MiscFlags = 0;
        ibDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA subResData = { };
        subResData.pSysMem = frustumIndices;

        DXCall(device->CreateBuffer(&ibDesc, &subResData, &frustumLineIB));

        frustumLineIndexCount = ArraySize_(frustumIndices);
    }

    CreateShadowMaps();
}

// Performs frustum/sphere intersection tests for all MeshPart's
static void DoFrustumTests(const Camera& camera, bool ignoreNearZ, MeshData& mesh)
{
    mesh.FrustumTests.clear();
    mesh.NumSuccessfulTests = 0;

    Frustum frustum;
    ComputeFrustum(camera, frustum);

    for(uint32 i = 0; i < mesh.BoundingSpheres.size(); ++i)
    {
        const Sphere& sphere = mesh.BoundingSpheres[i];
        uint32 test = TestFrustumSphere(frustum, sphere, ignoreNearZ);
        mesh.FrustumTests.push_back(test);
        mesh.NumSuccessfulTests += test;
    }
}

void MeshRenderer::Update()
{
    if(AppSettings::ShadowMapSize.Changed() || AppSettings::ShadowMode.Changed()
        || AppSettings::ShadowMSAA.Changed() || AppSettings::SMFormat.Changed()
        || AppSettings::EnableShadowMips.Changed() || AppSettings::DepthBufferFormat.Changed())
        CreateShadowMaps();

    if(AppSettings::VisualizeCascades.Changed() || AppSettings::UsePlaneDepthBias.Changed()
       || AppSettings::FilterAcrossCascades.Changed() || AppSettings::FixedFilterSize.Changed()
       || AppSettings::ShadowMode.Changed() || AppSettings::RandomizeDiscOffsets.Changed()
       || AppSettings::CascadeSelectionMode.Changed())
        meshPS = CompileMeshPS(device);

    if(AppSettings::AutoComputeDepthBounds && AppSettings::GPUSceneSubmission == false)
    {
        AppSettings::MinCascadeDistance.SetValue(reductionDepth.x);
        AppSettings::MaxCascadeDistance.SetValue(reductionDepth.y);
    }
    else
        currFrame = 0;
}

// Creates the chain of render targets used for computing min/max depth
void MeshRenderer::CreateReductionTargets(uint32 width, uint32 height)
{
    depthReductionTargets.clear();

    if(UseComputeReduction)
    {
        uint32 w = width;
        uint32 h = height;

        while(w > 1 || h > 1)
        {
            w = DispatchSize(ReductionTGSize, w);
            h = DispatchSize(ReductionTGSize, h);

            RenderTarget2D rt;
            rt.Initialize(device, w, h, DXGI_FORMAT_R16G16_UNORM, 1, 1, 0, FALSE, TRUE);
            depthReductionTargets.push_back(rt);
        }
    }
    else
    {
        // Basically create a mip chain
        uint32 w = width;
        uint32 h = height;

        while(w > 1 || h > 1)
        {
            w = static_cast<uint32>(Round(w / 2.0f));
            h = static_cast<uint32>(Round(h / 2.0f));

            w = std::max<uint32>(w, 1);
            h = std::max<uint32>(h, 1);

            RenderTarget2D rt;
            rt.Initialize(device, w, h, DXGI_FORMAT_R16G16_UNORM);
            depthReductionTargets.push_back(rt);
        }
    }
}

// Computes the min and max depth from the depth buffer using a parallel reduction
void MeshRenderer::ReduceDepth(ID3D11DeviceContext* context, ID3D11ShaderResourceView* depthTarget,
                               const Camera& camera)
{
    PIXEvent event(L"Depth Reduction");
    ProfileBlock block(L"Depth Reduction");

    reductionConstants.Data.Projection = Float4x4::Transpose(camera.ProjectionMatrix());
    reductionConstants.Data.NearClip = camera.NearClip();
    reductionConstants.Data.FarClip = camera.FarClip();
    reductionConstants.ApplyChanges(context);

    if(UseComputeReduction)
    {
        reductionConstants.SetCS(context, 0);

        ID3D11RenderTargetView* rtvs[1] = { nullptr };
        context->OMSetRenderTargets(1, rtvs, nullptr);

        ID3D11UnorderedAccessView* uavs[1] = { depthReductionTargets[0].UAView };
        context->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

        ID3D11ShaderResourceView* srvs[1] = { depthTarget };
        context->CSSetShaderResources(0, 1, srvs);

        ID3D11SamplerState*  samplers[1] = { samplerStates.LinearClamp() };
        context->CSSetSamplers(0, 1, samplers);

        context->CSSetShader(depthReductionInitialCS, nullptr, 0);

        uint32 dispatchX = depthReductionTargets[0].Width;
        uint32 dispatchY = depthReductionTargets[0].Height;
        context->Dispatch(dispatchX, dispatchY, 1);

        uavs[0] = nullptr;
        context->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

        srvs[0] = nullptr;
        context->CSSetShaderResources(0, 1, srvs);

        context->CSSetShader(depthReductionCS, nullptr, 0);

        for(uint32 i = 1; i < depthReductionTargets.size(); ++i)
        {
            uavs[0] = depthReductionTargets[i].UAView;
            context->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

            srvs[0] = depthReductionTargets[i - 1].SRView;
            context->CSSetShaderResources(0, 1, srvs);

            dispatchX = depthReductionTargets[i].Width;
            dispatchY = depthReductionTargets[i].Height;
            context->Dispatch(dispatchX, dispatchY, 1);

            uavs[0] = nullptr;
            context->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

            srvs[0] = nullptr;
            context->CSSetShaderResources(0, 1, srvs);
        }
    }
    else
    {
        float blendFactor[4] = {1, 1, 1, 1};
        context->OMSetBlendState(blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
        context->RSSetState(rasterizerStates.NoCull());
        context->OMSetDepthStencilState(depthStencilStates.DepthDisabled(), 0);
        ID3D11Buffer* vbs[1] = { nullptr };
        uint32 strides[1] = { 0 };
        uint32 offsets[1] = { 0 };
        context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
        context->IASetInputLayout(nullptr);

        reductionConstants.SetPS(context, 0);

        context->VSSetShader(fullScreenVS, nullptr, 0);

        context->PSSetShader(depthReductionInitialPS, nullptr, 0);

        ID3D11RenderTargetView* rtvs[1] = { depthReductionTargets[0].RTView };
        context->OMSetRenderTargets(1, rtvs, nullptr);

        ID3D11ShaderResourceView* srvs[1] = { depthTarget };
        context->PSSetShaderResources(0, 1, srvs);

        ID3D11SamplerState*  samplers[1] = { samplerStates.LinearClamp() };
        context->PSSetSamplers(0, 1, samplers);

        D3D11_VIEWPORT vp;
        vp.TopLeftX = 0.0f;
        vp.TopLeftY = 0.0f;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.Width = static_cast<float>(depthReductionTargets[0].Width);
        vp.Height = static_cast<float>(depthReductionTargets[0].Height);
        context->RSSetViewports(1, &vp);

        context->Draw(3, 0);

        srvs[0] = nullptr;
        context->PSSetShaderResources(0, 1, srvs);

        context->PSSetShader(depthReductionPS, nullptr, 0);

        for(uint32 i = 1; i < depthReductionTargets.size(); ++i)
        {
            rtvs[0] = depthReductionTargets[i].RTView;
            context->OMSetRenderTargets(1, rtvs, nullptr);

            srvs[0] = depthReductionTargets[i - 1].SRView;
            context->PSSetShaderResources(0, 1, srvs);

            vp.Width = static_cast<float>(depthReductionTargets[i].Width);
            vp.Height = static_cast<float>(depthReductionTargets[i].Height);
            context->RSSetViewports(1, &vp);

            context->Draw(3, 0);

            srvs[0] = nullptr;
            context->PSSetShaderResources(0, 1, srvs);
        }
    }

    // Don't read back the depth when doing GPU batching, because that would default the purpose!
    if(AppSettings::GPUSceneSubmission)
        return;

    // Copy to a staging texture
    const uint32 latency = uint32(AppSettings::ReadbackLatency + 1);
    ID3D11Texture2D* lastTarget = depthReductionTargets[depthReductionTargets.size() - 1].Texture;
    context->CopyResource(reductionStagingTextures[currFrame % latency].Texture, lastTarget);

    ++currFrame;

    if(currFrame >= latency)
    {
        CPUProfileBlock cpuBlock(L"Depth Readback");

        StagingTexture2D& stagingTexture = reductionStagingTextures[currFrame % latency];

        uint32 pitch;
        const uint16* texData = reinterpret_cast<uint16*>(stagingTexture.Map(context, 0, pitch));
        reductionDepth.x = texData[0] / static_cast<float>(0xffff);
        reductionDepth.y = texData[1] / static_cast<float>(0xffff);

        stagingTexture.Unmap(context, 0);
    }
    else
    {
        reductionDepth = Float2(0.0f, 1.0f);
    }
}

// Convert to a VSM map
void MeshRenderer::ConvertToVSM(ID3D11DeviceContext* context, uint32 cascadeIdx,
                                Float3 cascadeScale, Float3 cascade0Scale)
{
    PIXEvent event(L"VSM Conversion");

    float blendFactor[4] = {1, 1, 1, 1};
    context->OMSetBlendState(blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
    context->RSSetState(rasterizerStates.NoCull());
    context->OMSetDepthStencilState(depthStencilStates.DepthDisabled(), 0);
    ID3D11Buffer* vbs[1] = { nullptr };
    uint32 strides[1] = { 0 };
    uint32 offsets[1] = { 0 };
    context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
    context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
    context->IASetInputLayout(nullptr);

    D3D11_VIEWPORT vp;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.Width = static_cast<float>(varianceShadowMap.Width);
    vp.Height = static_cast<float>(varianceShadowMap.Height);
    context->RSSetViewports(1, &vp);

    vsmConstants.Data.CascadeScale = Float4(cascadeScale, 1.0f);
    vsmConstants.Data.Cascade0Scale = Float4(cascade0Scale, 1.0f);
    vsmConstants.Data.ShadowMapSize = Float2(float(varianceShadowMap.Width), float(varianceShadowMap.Height));
    vsmConstants.ApplyChanges(context);
    vsmConstants.SetPS(context, 0);

    if(AppSettings::GPUSceneSubmission)
    {
        // Copy the cascade scale from the GPU buffer
        CopyBufferRegion(context, vsmConstants.Buffer, cascadeScaleBuffer.Buffer, 0,
                         sizeof(Float4) * cascadeIdx, sizeof(vsmConstants.Data.CascadeScale));

        CopyBufferRegion(context, vsmConstants.Buffer, cascadeScaleBuffer.Buffer, sizeof(Float4),
                         sizeof(Float4) * 0, sizeof(vsmConstants.Data.CascadeScale));
    }

    context->VSSetShader(fullScreenVS, nullptr, 0);

    ID3D11PixelShader* ps = vsmConvertPS[AppSettings::ShadowMode - uint32(ShadowMode::VSM)][AppSettings::ShadowMSAA];
    context->PSSetShader(ps, nullptr, 0);

    ID3D11RenderTargetView* rtvs[1] = { varianceShadowMap.RTVArraySlices[cascadeIdx] };
    context->OMSetRenderTargets(1, rtvs, nullptr);

    ID3D11ShaderResourceView* srvs[1] = { shadowMap.SRView };
    context->PSSetShaderResources(0, 1, srvs);

    context->Draw(3, 0);

    srvs[0] = nullptr;
    context->PSSetShaderResources(0, 1, srvs);

    float maxFilterSizeU = MaxKernelSize / std::abs(cascade0Scale.x);
    float maxFilterSizeV = MaxKernelSize / std::abs(cascade0Scale.y);
    const float FilterSizeU = Clamp(std::min<float>(AppSettings::FilterSize, maxFilterSizeU) * std::abs(cascadeScale.x), 1.0f, MaxKernelSize);
    const float FilterSizeV = Clamp(std::min<float>(AppSettings::FilterSize, maxFilterSizeV) * std::abs(cascadeScale.y), 1.0f, MaxKernelSize);

    // For GPU-driven submission, we always run the blur passes and use a dynamic loop
    // in the shader. For CPU submission, we figure out the minimum sample radius and
    // switch shader permutations. This could also be done with GPU submission,
    // by using DrawIndirect or something similar.
    if((FilterSizeU > 1.0f || FilterSizeV > 1.0f) || AppSettings::GPUSceneSubmission)
    {
        // Horizontal pass
        uint32 sampleRadiusU = static_cast<uint32>((FilterSizeU / 2) + 0.499f);

        rtvs[0] = tempVSM.RTView;
        context->OMSetRenderTargets(1, rtvs, nullptr);

        srvs[0] = varianceShadowMap.SRVArraySlices[cascadeIdx];
        context->PSSetShaderResources(0, 1, srvs);

        if(AppSettings::GPUSceneSubmission)
            context->PSSetShader(vsmBlurGPUH, nullptr, 0);
        else
            context->PSSetShader(vsmBlurH[sampleRadiusU], nullptr, 0);

        context->Draw(3, 0);

        srvs[0] = nullptr;
        context->PSSetShaderResources(0, 1, srvs);

        // Vertical pass
        uint32 sampleRadiusV = static_cast<uint32>((FilterSizeV / 2) + 0.499f);

        rtvs[0] = varianceShadowMap.RTVArraySlices[cascadeIdx];
        context->OMSetRenderTargets(1, rtvs, nullptr);

        srvs[0] = tempVSM.SRView;
        context->PSSetShaderResources(0, 1, srvs);

        if(AppSettings::GPUSceneSubmission)
            context->PSSetShader(vsmBlurGPUV, nullptr, 0);
        else
            context->PSSetShader(vsmBlurV[sampleRadiusV], nullptr, 0);

        context->Draw(3, 0);

        srvs[0] = nullptr;
        context->PSSetShaderResources(0, 1, srvs);
    }

    if(AppSettings::EnableShadowMips && cascadeIdx == NumCascades - 1)
        context->GenerateMips(varianceShadowMap.SRView);
}

// Renders the main pass for all meshes in both scenes (assumes shadow maps are already rendered)
void MeshRenderer::Render(ID3D11DeviceContext* context, const Camera& camera, const Float4x4& world,
                          const Float4x4& characterWorld)
{
    PIXEvent event(L"Mesh Rendering");
    ProfileBlock block(L"Mesh Rendering");

    DoFrustumTests(camera, false, scene);
    DoFrustumTests(camera, false, character);

    // Set states
    float blendFactor[4] = {1, 1, 1, 1};
    context->OMSetBlendState(blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(depthStencilStates.DepthEnabled(), 0);
    context->RSSetState(rasterizerStates.BackFaceCull());

    ID3D11SamplerState* sampStates[4] = {
        samplerStates.Anisotropic(),
        samplerStates.ShadowMap(),
        samplerStates.ShadowMapPCF(),
        evsmSamplers[AppSettings::ShadowAnisotropy],
    };

    context->PSSetSamplers(0, 4, sampStates);

    // Set shaders
    context->DSSetShader(nullptr, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->VSSetShader(meshVS , nullptr, 0);

    /*uint32 filterSize = (AppSettings::ShadowMode == ShadowMode::FixedSizePCF || AppSettings::ShadowMode == ShadowMode::OptimizedPCF) ? AppSettings::FixedFilterSize : 0;
    uint32 randomizeOffsets = AppSettings::ShadowMode == ShadowMode::RandomDiscPCF ? AppSettings::RandomizeDiscOffsets : 0;
    uint32 usePlaneBias = AppSettings::UseFilterableShadows() ? 0 : AppSettings::UsePlaneDepthBias;
    ID3D11PixelShader* ps = meshPS[AppSettings::VisualizeCascades][usePlaneBias][AppSettings::FilterAcrossCascades]
                                  [randomizeOffsets][filterSize][AppSettings::ShadowMode];*/
    context->PSSetShader(meshPS, nullptr, 0);

    {
        PIXEvent event_(L"Static Mesh Rendering");

        RenderModel(context, camera, world, scene);
    }

    {
        PIXEvent event_(L"Character Mesh Rendering");

        RenderModel(context, camera, characterWorld, character);
    }

    ID3D11ShaderResourceView* nullSRVs[3] = { nullptr };
    context->PSSetShaderResources(0, 3, nullSRVs);
}

// Renders one of the models, either the scene or the character
void MeshRenderer::RenderModel(ID3D11DeviceContext* context, const Camera& camera, const Float4x4& world,
                              MeshData& meshData)
{
    // Set constant buffers
    meshVSConstants.Data.World = Float4x4::Transpose(world);
    meshVSConstants.Data.ViewProjection = Float4x4::Transpose(camera.ViewProjectionMatrix());
    meshVSConstants.ApplyChanges(context);
    meshVSConstants.SetVS(context, 0);

    meshPSConstants.Data.CameraPosWS = camera.Position();
    meshPSConstants.ApplyChanges(context);
    meshPSConstants.SetPS(context, 0);

    if(AppSettings::GPUSceneSubmission)
    {
        // Copy the computed cascade info to the constant buffer
        D3D11_BOX srcBox;
        srcBox.left = 0;
        srcBox.right = sizeof(float) * NumCascades;
        srcBox.top = 0;
        srcBox.bottom = 1;
        srcBox.front = 0;
        srcBox.back = 1;
        uint32 dstOffset = offsetof(MeshPSConstants, CascadeSplits);
        context->CopySubresourceRegion(meshPSConstants.Buffer, 0, dstOffset, 0, 0,
                                       cascadeSplitBuffer.Buffer, 0, &srcBox);

        srcBox.right = sizeof(Float4) * NumCascades;
        dstOffset = offsetof(MeshPSConstants, CascadeOffsets);
        context->CopySubresourceRegion(meshPSConstants.Buffer, 0, dstOffset, 0, 0,
                                       cascadeOffsetBuffer.Buffer, 0, &srcBox);

        srcBox.right = sizeof(Float4) * NumCascades;
        dstOffset = offsetof(MeshPSConstants, CascadeScales);
        context->CopySubresourceRegion(meshPSConstants.Buffer, 0, dstOffset, 0, 0,
                                       cascadeScaleBuffer.Buffer, 0, &srcBox);
    }

    // Draw all meshes
    Model* model = meshData.Model;
    uint32 partCount = 0;
    for(uint32 meshIdx = 0; meshIdx < model->Meshes().size(); ++meshIdx)
    {
        Mesh& mesh = model->Meshes()[meshIdx];

        // Set the vertices and indices
        ID3D11Buffer* vertexBuffers[1] = { mesh.VertexBuffer() };
        uint32 vertexStrides[1] = { mesh.VertexStride() };
        uint32 offsets[1] = { 0 };
        context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, offsets);
        context->IASetIndexBuffer(mesh.IndexBuffer(), mesh.IndexBufferFormat(), 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Set the input layout
        context->IASetInputLayout(meshData.InputLayouts[meshIdx]);

        // Draw all parts
        for(uint64 partIdx = 0; partIdx < mesh.MeshParts().size(); ++partIdx)
        {
            if(meshData.FrustumTests[partCount++])
            {
                const MeshPart& part = mesh.MeshParts()[partIdx];
                const MeshMaterial& material = model->Materials()[part.MaterialIdx];

                // Set the textures
                ID3D11ShaderResourceView* psTextures[3] =
                {
                    material.DiffuseMap,
                    shadowMap.SRView,
                    randomRotations,
                };

                if(psTextures[0] == nullptr)
                    psTextures[0] = defaultTexture;

                if(AppSettings::UseFilterableShadows())
                    psTextures[1] = varianceShadowMap.SRView;

                context->PSSetShaderResources(0, 3, psTextures);
                context->DrawIndexed(part.IndexCount, part.IndexStart, 0);
            }
        }
    }
}

// Renders all meshes using depth-only rendering, using CPU-driven submission
void MeshRenderer::RenderDepthCPU(ID3D11DeviceContext* context, const Camera& camera,
                                  const Float4x4& world, const Float4x4& characterWorld,
                                  bool shadowRendering)
{
    PIXEvent event(L"Mesh Depth Rendering");

    DoFrustumTests(camera, shadowRendering, scene);
    DoFrustumTests(camera, shadowRendering, character);

    SetupRenderDepthState(context, shadowRendering);

    {
        PIXEvent event_(L"Static Mesh Rendering");
        RenderModelDepthCPU(context, camera, world, scene);
    }

    {
        PIXEvent event_(L"Character Mesh Rendering");
        RenderModelDepthCPU(context, camera, characterWorld, character);
    }
}

// Renders all meshes using depth-only rendering, using GPU-driven submission
void MeshRenderer::RenderDepthGPU(ID3D11DeviceContext* context, const Camera& camera, const Float4x4& world,
                                  const Float4x4& characterWorld, bool shadowRendering)
{
    tempViewProjBuffer.Data = Float4x4::Transpose(camera.ViewProjectionMatrix());
    tempViewProjBuffer.ApplyChanges(context);

    Frustum frustum;
    ComputeFrustum(camera, frustum);
    for(uint64 i = 0; i < 6; ++i)
        tempFrustumPlanesBuffer.Data.FrustumPlanes[i] = frustum.Planes[i];
    tempFrustumPlanesBuffer.ApplyChanges(context);

    RenderDepthGPU(context, world, characterWorld, shadowRendering, tempViewProjBuffer.Buffer, 0,
                   tempFrustumPlanesBuffer.Buffer, 0);
}

// Renders all meshes using depth-only rendering, using GPU-driven submission
void MeshRenderer::RenderDepthGPU(ID3D11DeviceContext* context, const Float4x4& world,
                                  const Float4x4& characterWorld, bool shadowRendering,
                                  ID3D11Buffer* viewProj, uint32 viewProjOffset,
                                  ID3D11Buffer* frustumPlanes, uint32 planesOffset)
{
    PIXEvent event(L"Mesh Depth Rendering");

    SetupRenderDepthState(context, shadowRendering);

    {
        PIXEvent event_(L"Static Mesh Rendering");
        RenderModelDepthGPU(context, scene, shadowRendering, world, viewProj, viewProjOffset,
                           frustumPlanes, planesOffset);
    }

    {
        PIXEvent event_(L"Character Mesh Rendering");
        RenderModelDepthGPU(context, character, shadowRendering, characterWorld, viewProj, viewProjOffset,
                           frustumPlanes, planesOffset);
    }
}

// Sets render states used for depth-only rendering
void MeshRenderer::SetupRenderDepthState(ID3D11DeviceContext* context, bool shadowRendering)
{
    // Set states
    float blendFactor[4] = {1, 1, 1, 1};
    context->OMSetBlendState(blendStates.ColorWriteDisabled(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(depthStencilStates.DepthWriteEnabled(), 0);

    if(shadowRendering)
        context->RSSetState(shadowRSState);
    else
        context->RSSetState(rasterizerStates.BackFaceCull());

    // Set shaders
    context->VSSetShader(meshDepthVS , nullptr, 0);
    context->PSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->DSSetShader(nullptr, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);
}

// Renders depth-only for a model using CPU-driven submission
void MeshRenderer::RenderModelDepthCPU(ID3D11DeviceContext* context, const Camera& camera, const Float4x4& world,
                                      MeshData& meshData)
{
    // Set constant buffers
    depthOnlyConstants.Data.World = Float4x4::Transpose(world);
    depthOnlyConstants.Data.ViewProjection = Float4x4::Transpose(camera.ViewProjectionMatrix());
    depthOnlyConstants.ApplyChanges(context);
    depthOnlyConstants.SetVS(context, 0);

    Model* model = meshData.Model;

    uint32 partCount = 0;
    for (uint64 meshIdx = 0; meshIdx < model->Meshes().size(); ++meshIdx)
    {
        Mesh& mesh = model->Meshes()[meshIdx];

        // Set the vertices and indices
        ID3D11Buffer* vertexBuffers[1] = { mesh.VertexBuffer() };
        uint32 vertexStrides[1] = { mesh.VertexStride() };
        uint32 offsets[1] = { 0 };
        context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, offsets);
        context->IASetIndexBuffer(mesh.IndexBuffer(), mesh.IndexBufferFormat(), 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Set the input layout
        context->IASetInputLayout(meshData.DepthInputLayouts[meshIdx]);

        // Draw all parts
        for (uint64 partIdx = 0; partIdx < mesh.MeshParts().size(); ++partIdx)
        {
            if(meshData.FrustumTests[partCount++])
            {
                const MeshPart& part = mesh.MeshParts()[partIdx];
                context->DrawIndexed(part.IndexCount, part.IndexStart, 0);
            }
        }
    }
}

// Renders depth-only for a model using GPU-driven submission
void MeshRenderer::RenderModelDepthGPU(ID3D11DeviceContext* context, MeshData& meshData, bool shadowRendering,
                                      const Float4x4& world, ID3D11Buffer* viewProj, uint32 viewProjOffset,
                                      ID3D11Buffer* frustumPlanes, uint32 planeOffset)
{
    // Prepare the constant buffer
    gpuBatchConstants.Data.NumDrawCalls = meshData.DrawCalls.NumElements;
    gpuBatchConstants.Data.CullNearZ = shadowRendering == false;
    gpuBatchConstants.ApplyChanges(context);
    gpuBatchConstants.SetCS(context, 0);

    CopyBufferRegion(context, gpuBatchConstants.Buffer, frustumPlanes, 0, planeOffset, sizeof(Float4) * 6);

    // Clear the IndirectDraw args buffer
    SetCSShader(context, clearArgsBuffer);
    SetCSOutputs(context, drawArgsBuffer.UAView);
    context->Dispatch(1, 1, 1);
    ClearCSOutputs(context);

    // Cull the draw calls
    SetCSShader(context, cullDrawCalls);
    SetCSInputs(context, meshData.DrawCalls.SRView);
    ID3D11UnorderedAccessView* uavs[2] = { drawArgsBuffer.UAView, meshData.CulledDraws.UAView };
    uint32 counts[2] = { 0, 0 };
    context->CSSetUnorderedAccessViews(0, 2, uavs, counts);
    context->Dispatch(DispatchSize(CullTGSize, meshData.DrawCalls.NumElements), 1, 1);
    ClearCSOutputs(context);
    ClearCSInputs(context);

    // Copy the number of culled draws to a DispatchIndirect args buffer.
    // We'll do 1 thread group per draw call
    context->CopyStructureCount(batchDispatchArgs.Buffer, 0, meshData.CulledDraws.UAView);

    // Copy culled indices to the batched index buffer
    SetCSShader(context, batchDrawCalls);
    SetCSInputs(context, meshData.CulledDraws.SRView, meshData.Indices.SRView);
    SetCSOutputs(context, meshData.CulledIndices.UAView);
    context->DispatchIndirect(batchDispatchArgs.Buffer, 0);
    ClearCSOutputs(context);
    ClearCSInputs(context);

    // Setup the constant buffer for mesh rendering
    depthOnlyConstants.Data.World = Float4x4::Transpose(world);
    depthOnlyConstants.ApplyChanges(context);
    depthOnlyConstants.SetVS(context, 0);

    CopyBufferRegion(context, depthOnlyConstants.Buffer, viewProj, sizeof(Float4x4), viewProjOffset, sizeof(Float4x4));

    // Set the vertices and indices
    ID3D11Buffer* vertexBuffers[1] = { meshData.PositionsVB };
    uint32 vertexStrides[1] = { sizeof(Float3) };
    uint32 offsets[1] = { 0 };
    context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, offsets);
    context->IASetIndexBuffer(meshData.CulledIndices.Buffer, DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set the input layout
    context->IASetInputLayout(depthGPUInputLayout);

    // Draw the batch
    context->DrawIndexedInstancedIndirect(drawArgsBuffer.Buffer, 0);
}

// Renders the shadow map for all cascades, and performs VSM conversion if necessary.
// This uses CPU-driven shadow map setup and scene submission
void MeshRenderer::RenderShadowMap(ID3D11DeviceContext* context, const Camera& camera,
                                    const Float4x4& world, const Float4x4& characterWorld)
{
    PIXEvent event(L"Mesh Shadow Map Rendering");
    ProfileBlock block(L"Shadow Map Rendering");

    const uint32 ShadowMapSize = AppSettings::ShadowMapResolution();
    const float sMapSize = static_cast<float>(ShadowMapSize);

    const float MinDistance = AppSettings::AutoComputeDepthBounds ? reductionDepth.x
                                                                  : AppSettings::MinCascadeDistance;
    const float MaxDistance = AppSettings::AutoComputeDepthBounds ? reductionDepth.y
                                                                  : AppSettings::MaxCascadeDistance;

    // Compute the split distances based on the partitioning mode
    float CascadeSplits[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    if(AppSettings::PartitionMode == PartitionMode::Manual)
    {
        CascadeSplits[0] = MinDistance + AppSettings::SplitDistance0 * MaxDistance;
        CascadeSplits[1] = MinDistance + AppSettings::SplitDistance1 * MaxDistance;
        CascadeSplits[2] = MinDistance + AppSettings::SplitDistance2 * MaxDistance;
        CascadeSplits[3] = MinDistance + AppSettings::SplitDistance3 * MaxDistance;
    }
    else if(AppSettings::PartitionMode == PartitionMode::Logarithmic
        || AppSettings::PartitionMode == PartitionMode::PSSM)
    {
        float lambda = 1.0f;
        if(AppSettings::PartitionMode == PartitionMode::PSSM)
            lambda = AppSettings::PSSMLambda;

        float nearClip = camera.NearClip();
        float farClip = camera.FarClip();
        float clipRange = farClip - nearClip;

        float minZ = nearClip + MinDistance * clipRange;
        float maxZ = nearClip + MaxDistance * clipRange;

        float range = maxZ - minZ;
        float ratio = maxZ / minZ;

        for(uint32 i = 0; i < NumCascades; ++i)
        {
            float p = (i + 1) / static_cast<float>(NumCascades);
            float log = minZ * std::pow(ratio, p);
            float uniform = minZ + range * p;
            float d = lambda * (log - uniform) + uniform;
            CascadeSplits[i] = (d - nearClip) / clipRange;
        }
    }

    Float4x4 globalShadowMatrix = MakeGlobalShadowMatrix(camera);
    meshPSConstants.Data.ShadowMatrix = Float4x4::Transpose(globalShadowMatrix);

    // Render the meshes to each cascade
    for(uint32 cascadeIdx = 0; cascadeIdx < NumCascades; ++cascadeIdx)
    {
        PIXEvent cascadeEvent((L"Rendering Shadow Map Cascade " + ToString(cascadeIdx)).c_str());

        // Set the viewport
        D3D11_VIEWPORT viewport;
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = sMapSize;
        viewport.Height = sMapSize;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context->RSSetViewports(1, &viewport);

        // Set the shadow map as the depth target
        ID3D11DepthStencilView* dsv = shadowMap.DSView;
        if(AppSettings::UseFilterableShadows() == false)
            dsv = shadowMap.ArraySlices[cascadeIdx];
        ID3D11RenderTargetView* nullRenderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { nullptr };
        context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, nullRenderTargets, dsv);
        context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

        // Get the 8 points of the view frustum in world space
        Float3 frustumCornersWS[8] =
        {
            Float3(-1.0f,  1.0f, 0.0f),
            Float3( 1.0f,  1.0f, 0.0f),
            Float3( 1.0f, -1.0f, 0.0f),
            Float3(-1.0f, -1.0f, 0.0f),
            Float3(-1.0f,  1.0f, 1.0f),
            Float3( 1.0f,  1.0f, 1.0f),
            Float3( 1.0f, -1.0f, 1.0f),
            Float3(-1.0f, -1.0f, 1.0f),
        };

        float prevSplitDist = cascadeIdx == 0 ? MinDistance : CascadeSplits[cascadeIdx - 1];
        float splitDist = CascadeSplits[cascadeIdx];

        Float4x4 invViewProj = CalculateInverseViewProj(camera);

        for(uint32 i = 0; i < 8; ++i)
            frustumCornersWS[i] = Float3::Transform(frustumCornersWS[i], invViewProj);

        // Get the corners of the current cascade slice of the view frustum
        for(uint32 i = 0; i < 4; ++i)
        {
            Float3 cornerRay = frustumCornersWS[i + 4] - frustumCornersWS[i];
            Float3 nearCornerRay = cornerRay * prevSplitDist;
            Float3 farCornerRay = cornerRay * splitDist;
            frustumCornersWS[i + 4] = frustumCornersWS[i] + farCornerRay;
            frustumCornersWS[i] = frustumCornersWS[i] + nearCornerRay;
        }

        // Calculate the centroid of the view frustum slice
        Float3 frustumCenter = 0.0f;
        for(uint32 i = 0; i < 8; ++i)
            frustumCenter = frustumCenter + frustumCornersWS[i];
        frustumCenter *=  1.0f / 8.0f;

        Float3 upDir = Float3(0.0f, 1.0f, 0.0f);

        Float3 minExtents;
        Float3 maxExtents;
        if(AppSettings::StabilizeCascades)
        {
            // Calculate the radius of a bounding sphere surrounding the frustum corners
            float sphereRadius = 0.0f;
            for(uint32 i = 0; i < 8; ++i)
            {
                float dist = Float3::Length(frustumCornersWS[i] - frustumCenter);
                sphereRadius = std::max(sphereRadius, dist);
            }

            sphereRadius = std::ceil(sphereRadius * 16.0f) / 16.0f;

            maxExtents = Float3(sphereRadius, sphereRadius, sphereRadius);
            minExtents = -maxExtents;
        }
        else
        {
            // Create a temporary view matrix for the light
            Float3 lightCameraPos = frustumCenter;
            Float3 lookAt = frustumCenter - AppSettings::LightDirection;
            Float4x4 lightView = XMMatrixLookAtLH(lightCameraPos.ToSIMD(), lookAt.ToSIMD(), upDir.ToSIMD());

            // Calculate an AABB around the frustum corners
            Float3 mins = FLT_MAX;
            Float3 maxes = -FLT_MAX;
            for(uint32 i = 0; i < 8; ++i)
            {
                Float3 corner = Float3::Transform(frustumCornersWS[i], lightView);
                mins = XMVectorMin(mins.ToSIMD(), corner.ToSIMD());
                maxes = XMVectorMax(maxes.ToSIMD(), corner.ToSIMD());
            }

            minExtents = mins;
            maxExtents = maxes;

            // Adjust the min/max to accommodate the filtering size
            float scale = (ShadowMapSize + AppSettings::FixedFilterKernelSize()) / static_cast<float>(ShadowMapSize);
            minExtents.x *= scale;
            minExtents.y *= scale;
            maxExtents.x *= scale;
            maxExtents.y *= scale;
        }

        Float3 cascadeExtents = maxExtents - minExtents;

        // Get position of the shadow camera
        Float3 shadowCameraPos = frustumCenter + AppSettings::LightDirection.Value() * -minExtents.z;

        // Come up with a new orthographic camera for the shadow caster
        OrthographicCamera shadowCamera(minExtents.x, minExtents.y, maxExtents.x,
                                        maxExtents.y, 0.0f, cascadeExtents.z);
        shadowCamera.SetLookAt(shadowCameraPos, frustumCenter, upDir);

        if(AppSettings::StabilizeCascades)
        {
            // Create the rounding matrix, by projecting the world-space origin and determining
            // the fractional offset in texel space
            XMMATRIX shadowMatrix = shadowCamera.ViewProjectionMatrix().ToSIMD();
            XMVECTOR shadowOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
            shadowOrigin = XMVector4Transform(shadowOrigin, shadowMatrix);
            shadowOrigin = XMVectorScale(shadowOrigin, sMapSize / 2.0f);

            XMVECTOR roundedOrigin = XMVectorRound(shadowOrigin);
            XMVECTOR roundOffset = XMVectorSubtract(roundedOrigin, shadowOrigin);
            roundOffset = XMVectorScale(roundOffset, 2.0f / sMapSize);
            roundOffset = XMVectorSetZ(roundOffset, 0.0f);
            roundOffset = XMVectorSetW(roundOffset, 0.0f);

            XMMATRIX shadowProj = shadowCamera.ProjectionMatrix().ToSIMD();
            shadowProj.r[3] = XMVectorAdd(shadowProj.r[3], roundOffset);
            shadowCamera.SetProjection(shadowProj);
        }

        // Draw the mesh with depth only, using the new shadow camera
        RenderDepthCPU(context, shadowCamera, world, characterWorld, true);

        // Apply the scale/offset matrix, which transforms from [-1,1]
        // post-projection space to [0,1] UV space
        XMMATRIX texScaleBias;
        texScaleBias.r[0] = XMVectorSet(0.5f,  0.0f, 0.0f, 0.0f);
        texScaleBias.r[1] = XMVectorSet(0.0f, -0.5f, 0.0f, 0.0f);
        texScaleBias.r[2] = XMVectorSet(0.0f,  0.0f, 1.0f, 0.0f);
        texScaleBias.r[3] = XMVectorSet(0.5f,  0.5f, 0.0f, 1.0f);
        XMMATRIX shadowMatrix = shadowCamera.ViewProjectionMatrix().ToSIMD();
        invCascadeMats[cascadeIdx] = XMMatrixInverse(nullptr, shadowMatrix);
        shadowMatrix = XMMatrixMultiply(shadowMatrix, texScaleBias);

        // Store the split distance in terms of view space depth
        const float clipDist = camera.FarClip() - camera.NearClip();
        meshPSConstants.Data.CascadeSplits[cascadeIdx] = camera.NearClip() + splitDist * clipDist;

        // Calculate the position of the lower corner of the cascade partition, in the UV space
        // of the first cascade partition
        Float4x4 invCascadeMat = Float4x4::Invert(shadowMatrix);
        Float3 cascadeCorner = Float3::Transform(Float3(0.0f, 0.0f, 0.0f), invCascadeMat);
        cascadeCorner = Float3::Transform(cascadeCorner, globalShadowMatrix);

        // Do the same for the upper corner
        Float3 otherCorner = Float3::Transform(Float3(1.0f, 1.0f, 1.0f), invCascadeMat);
        otherCorner = Float3::Transform(otherCorner, globalShadowMatrix);

        // Calculate the scale and offset
        Float3 cascadeScale = Float3(1.0f, 1.0f, 1.0f) / (otherCorner - cascadeCorner);
        meshPSConstants.Data.CascadeOffsets[cascadeIdx] = Float4(-cascadeCorner, 0.0f);
        meshPSConstants.Data.CascadeScales[cascadeIdx] = Float4(cascadeScale, 1.0f);

        if(AppSettings::UseFilterableShadows())
            ConvertToVSM(context, cascadeIdx, meshPSConstants.Data.CascadeScales[cascadeIdx].To3D(),
                         meshPSConstants.Data.CascadeScales[0].To3D());
    }
}

// Renders the shadow map for all cascades using GPU batching, and performs VSM conversion if necessary
void MeshRenderer::RenderShadowMapGPU(ID3D11DeviceContext* context, const Camera& camera,
                                      const Float4x4& world, const Float4x4& characterWorld)
{
    PIXEvent event(L"Mesh Shadow Map Rendering(GPU)");
    ProfileBlock block(L"Shadow Map Rendering/Setup");

    Float4x4 shadowMatrix = MakeGlobalShadowMatrix(camera);

    meshPSConstants.Data.ShadowMatrix = Float4x4::Transpose(shadowMatrix);

    // Run the cascade setup shader on the GPU
    shadowSetupConstants.Data.GlobalShadowMatrix = Float4x4::Transpose(shadowMatrix);
    shadowSetupConstants.Data.ViewProjInv = Float4x4::Transpose(CalculateInverseViewProj(camera));
    shadowSetupConstants.Data.CameraRight = camera.WorldMatrix().Right();
    shadowSetupConstants.Data.CameraNearClip = camera.NearClip();
    shadowSetupConstants.Data.CameraFarClip = camera.FarClip();
    shadowSetupConstants.ApplyChanges(context);
    shadowSetupConstants.SetCS(context, 0);

    SetCSShader(context, setupCascades);
    SetCSInputs(context, depthReductionTargets[depthReductionTargets.size() - 1].SRView);
    SetCSOutputs(context, cascadeMatrixBuffer.UAView, cascadeSplitBuffer.UAView,
                 cascadeOffsetBuffer.UAView, cascadeScaleBuffer.UAView,
                 cascadePlanesBuffer.UAView);
    context->Dispatch(1, 1, 1);
    ClearCSInputs(context);
    ClearCSOutputs(context);

    const uint32 ShadowMapSize = AppSettings::ShadowMapResolution();
    const float sMapSize = static_cast<float>(ShadowMapSize);

    // Set the viewport
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = sMapSize;
    viewport.Height = sMapSize;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // Render each cascade
    for(uint32 cascadeIdx = 0; cascadeIdx < NumCascades; ++cascadeIdx)
    {
        ID3D11DepthStencilView* dsv = shadowMap.DSView;
        if(AppSettings::UseFilterableShadows() == false)
            dsv = shadowMap.ArraySlices[cascadeIdx];
        ID3D11RenderTargetView* nullRenderTargets[8] = { nullptr };
        context->OMSetRenderTargets(8, nullRenderTargets, dsv);
        context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

        RenderDepthGPU(context, world, characterWorld, true, cascadeMatrixBuffer.Buffer, sizeof(Float4x4) * cascadeIdx,
                       cascadePlanesBuffer.Buffer, sizeof(Float4) * 6 * cascadeIdx);

        if(AppSettings::UseFilterableShadows())
            ConvertToVSM(context, cascadeIdx, Float3(1.0f, 1.0f, 1.0f), Float3(1.0f, 1.0f, 1.0f));
    }
}

void MeshRenderer::RenderCascadeDebug(ID3D11DeviceContext* context, const Camera& camera, const Camera& cameraForShadows)
{
    PIXEvent event(L"Render Cascade Debug");
    ProfileBlock block(L"Render Cascade Debug");

    // Set states
    float blendFactor[4] = {1, 1, 1, 1};
    context->OMSetBlendState(blendStates.AlphaBlend(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(depthStencilStates.DepthWriteEnabled(), 0);
    context->RSSetState(rasterizerStates.NoCull());

    // Set shaders
    context->DSSetShader(nullptr, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->VSSetShader(drawFrustumVS , nullptr, 0);
    context->PSSetShader(drawFrustumPS, nullptr, 0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    context->IASetInputLayout(nullptr);
    context->IASetIndexBuffer(frustumLineIB, DXGI_FORMAT_R16_UINT, 0);

    drawFrustumConstants.Data.ViewProjection = Float4x4::Transpose(camera.ViewProjectionMatrix());
    drawFrustumConstants.Data.InvFrustumViewProj = Float4x4::Transpose(Float4x4::Invert(cameraForShadows.ViewProjectionMatrix()));
    drawFrustumConstants.Data.Color = Float4(1.0f, 1.0f, 1.0f, 1.0f);
    drawFrustumConstants.ApplyChanges(context);
    drawFrustumConstants.SetVS(context, 0);
    drawFrustumConstants.SetPS(context, 0);

    context->DrawIndexed(frustumLineIndexCount, 0, 0);

    const Float4 CascadeColors[NumCascades] =
    {
        Float4(1.0f, 0.0, 0.0f, 1.0f),
        Float4(0.0f, 1.0f, 0.0f, 1.0f),
        Float4(0.0f, 0.0f, 1.0f, 1.0f),
        Float4(1.0f, 1.0f, 0.0, 1.0f)
    };
    for(uint32 cascadeIdx = 0; cascadeIdx < NumCascades; ++cascadeIdx)
    {
        drawFrustumConstants.Data.InvFrustumViewProj = Float4x4::Transpose(invCascadeMats[cascadeIdx]);
        drawFrustumConstants.Data.Color = CascadeColors[cascadeIdx];
        drawFrustumConstants.ApplyChanges(context);

        context->DrawIndexed(frustumLineIndexCount, 0, 0);
    }
}