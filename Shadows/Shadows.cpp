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

#include "Shadows.h"
#include "resource.h"
#include "SharedConstants.h"
#include "AppSettings.h"

#include "SampleFramework11/InterfacePointers.h"
#include "SampleFramework11/Window.h"
#include "SampleFramework11/DeviceManager.h"
#include "SampleFramework11/Input.h"
#include "SampleFramework11/SpriteRenderer.h"
#include "SampleFramework11/Model.h"
#include "SampleFramework11/Utility.h"
#include "SampleFramework11/Camera.h"
#include "SampleFramework11/ShaderCompilation.h"
#include "SampleFramework11/Profiler.h"
#include "SampleFramework11/Settings.h"
#include "SampleFramework11/TwHelper.h"

using namespace SampleFramework11;
using std::wstring;

const uint32 WindowWidth = 1280;
const uint32 WindowHeight = 720;
const float WindowWidthF = static_cast<float>(WindowWidth);
const float WindowHeightF = static_cast<float>(WindowHeight);

static const float NearClip = 0.25f;
static const float FarClip = 250.0f;

// Mesh filenames
static const wstring MeshFileNames[] =
{
    L"Powerplant\\Powerplant.sdkmesh",
    L"Tower\\Tower.sdkmesh",
    L"Columns\\Columns.sdkmesh",
};

// Scale values applied to the mesh
static const float MeshScales[] =
{
    0.5f,
    0.025f,
    0.25f,
};

static const float CharacterScale = 1.0f;
static const Float3 CharacterPos = Float3(25.0f, 0.0f, 3.0f);

ShadowsApp::ShadowsApp() :  App(L"Shadows", MAKEINTRESOURCEW(IDI_ICON1)),
                                camera(WindowWidthF / WindowHeightF, XM_PIDIV4 * 0.75f, NearClip, FarClip)
{
    deviceManager.SetBackBufferWidth(WindowWidth);
    deviceManager.SetBackBufferHeight(WindowHeight);
    deviceManager.SetMinFeatureLevel(D3D_FEATURE_LEVEL_11_0);

    window.SetClientArea(WindowWidth, WindowHeight);
}

void ShadowsApp::BeforeReset()
{
    App::BeforeReset();
}

void ShadowsApp::AfterReset()
{
    App::AfterReset();

    float aspect = static_cast<float>(deviceManager.BackBufferWidth()) / deviceManager.BackBufferHeight();
    camera.SetAspectRatio(aspect);

    CreateRenderTargets();

    postProcessor.AfterReset(deviceManager.BackBufferWidth(), deviceManager.BackBufferHeight());
}

void ShadowsApp::Initialize()
{
    App::Initialize();

    AppSettings::Initialize(deviceManager.Device());

    TwHelper::SetGlobalHelpText("Shadows Sample - implements various cascaded shadow map techniques, "
                                "as well as several methods for filtering shadow maps");

    ID3D11DevicePtr device = deviceManager.Device();
    ID3D11DeviceContextPtr deviceContext = deviceManager.ImmediateContext();

    // Camera setup
    camera.SetPosition(Float3(40.0f, 5.0f, 5.0f));
    camera.SetYRotation(-XM_PIDIV2);

    // Load the meshes
    for(uint32 i = 0; i < uint32(Scene::NumValues); ++i)
    {
        wstring path(L"..\\Content\\Models\\");
        path += MeshFileNames[i];
        models[i].CreateFromSDKMeshFile(device, path.c_str());
    }

    wstring characterPath(L"..\\Content\\Models\\Soldier\\Soldier.sdkmesh");
    characterMesh.CreateFromSDKMeshFile(device, characterPath.c_str());

    meshRenderer.Initialize(device, deviceManager.ImmediateContext());

    ID3D11DeviceContext* context = deviceManager.ImmediateContext();

    Float4x4 meshWorld = Float4x4::ScaleMatrix(MeshScales[(int)AppSettings::CurrentScene]);
    meshRenderer.SetSceneMesh(context, &models[(int)AppSettings::CurrentScene], meshWorld);

    Float4x4 characterWorld = Float4x4::ScaleMatrix(CharacterScale);
    Float4x4 characterOrientation = Quaternion::ToFloat4x4(AppSettings::CharacterOrientation);
    characterWorld = characterWorld * characterOrientation;
    characterWorld.SetTranslation(CharacterPos);
    meshRenderer.SetCharacterMesh(context, &characterMesh, characterWorld);

    skybox.Initialize(device);

    // Init the post processor
    postProcessor.Initialize(device);
}

// Creates all required render targets
void ShadowsApp::CreateRenderTargets()
{
    ID3D11Device* device = deviceManager.Device();
    uint32 width = deviceManager.BackBufferWidth();
    uint32 height = deviceManager.BackBufferHeight();

    const uint32 MSAALevel = 4;
    colorTarget.Initialize(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, MSAALevel, 0);
    depthBuffer.Initialize(device, width, height, DXGI_FORMAT_D24_UNORM_S8_UINT, true, MSAALevel, 0);

    if(MSAALevel > 1)
        resolveTarget.Initialize(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, 1, 0);
    else
        resolveTarget = colorTarget;

    meshRenderer.CreateReductionTargets(width, height);
}

void ShadowsApp::Update(const Timer& timer)
{
    MouseState mouseState = MouseState::GetMouseState(window);
    KeyboardState kbState = KeyboardState::GetKeyboardState(window);

    if (kbState.IsKeyDown(KeyboardState::Escape))
        window.Destroy();

    float CamMoveSpeed = 5.0f * timer.DeltaSecondsF();
    const float CamRotSpeed = 0.180f * timer.DeltaSecondsF();
    const float MeshRotSpeed = 0.180f * timer.DeltaSecondsF();

    // Move the camera with keyboard input
    if (kbState.IsKeyDown(KeyboardState::LeftShift))
        CamMoveSpeed *= 0.25f;

    Float3 camPos = camera.Position();
    if (kbState.IsKeyDown(KeyboardState::W))
        camPos += camera.Forward() * CamMoveSpeed;
    else if (kbState.IsKeyDown(KeyboardState::S))
        camPos += camera.Back() * CamMoveSpeed;
    if (kbState.IsKeyDown(KeyboardState::A))
        camPos += camera.Left() * CamMoveSpeed;
    else if (kbState.IsKeyDown(KeyboardState::D))
        camPos += camera.Right() * CamMoveSpeed;
    if (kbState.IsKeyDown(KeyboardState::Q))
        camPos += camera.Up() * CamMoveSpeed;
    else if (kbState.IsKeyDown(KeyboardState::E))
        camPos += camera.Down() * CamMoveSpeed;

    camera.SetPosition(camPos);

    // Rotate the camera with the mouse
    if(mouseState.RButton.Pressed && mouseState.IsOverWindow)
    {
        float xRot = camera.XRotation();
        float yRot = camera.YRotation();
        xRot += mouseState.DY * CamRotSpeed;
        yRot += mouseState.DX * CamRotSpeed;
        camera.SetXRotation(xRot);
        camera.SetYRotation(yRot);
    }

    if(AppSettings::AnimateLight)
    {
        float rotY = XMScalarModAngle(timer.DeltaSecondsF() * 0.25f);
        Quaternion rotation = Quaternion::FromAxisAngle(Float3(0.0f, 1.0f, 0.0f), rotY);
        Float3 lightDir = AppSettings::LightDirection;
        lightDir = Float3::Transform(lightDir, rotation);
        AppSettings::LightDirection.SetValue(lightDir);
    }

    // Toggle VSYNC
    if(kbState.RisingEdge(KeyboardState::V))
        deviceManager.SetVSYNCEnabled(!deviceManager.VSYNCEnabled());

    if(AppSettings::CurrentScene.Changed())
    {
        float scale = MeshScales[AppSettings::CurrentScene];
        meshRenderer.SetSceneMesh(deviceManager.ImmediateContext(), &models[AppSettings::CurrentScene],
                             XMMatrixScaling(scale, scale, scale));
    }

    AppSettings::Update();

    meshRenderer.Update();
}

void ShadowsApp::Render(const Timer& timer)
{
    ID3D11DeviceContextPtr context = deviceManager.ImmediateContext();

    AppSettings::UpdateCBuffer(context);

    RenderMainPass();

    if(colorTarget.MultiSamples > 1)
        context->ResolveSubresource(resolveTarget.Texture, 0, colorTarget.Texture, 0, colorTarget.Format);

    // Kick off post-processing
    D3DPERF_BeginEvent(0xFFFFFFFF, L"Post Processing");
    PostProcessor::Constants constants;
    constants.BloomThreshold = AppSettings::BloomThreshold;
    constants.BloomMagnitude = AppSettings::BloomMagnitude;
    constants.BloomBlurSigma = AppSettings::BloomBlurSigma;
    constants.Tau = AppSettings::AdaptationRate;
    constants.KeyValue = AppSettings::KeyValue;
    constants.TimeDelta = timer.DeltaSecondsF();

    postProcessor.SetConstants(constants);
    postProcessor.Render(context, resolveTarget.SRView, deviceManager.BackBuffer());
    D3DPERF_EndEvent();

    // postProcessor.DrawDepthBuffer(depthBuffer, deviceManager.BackBuffer());

    ID3D11RenderTargetView* renderTargets[1] = { deviceManager.BackBuffer() };
    context->OMSetRenderTargets(1, renderTargets, NULL);

    D3D11_VIEWPORT vp;
    vp.Width = static_cast<float>(deviceManager.BackBufferWidth());
    vp.Height = static_cast<float>(deviceManager.BackBufferHeight());
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    context->RSSetViewports(1, &vp);

    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    /*spriteRenderer.Begin(context, SpriteRenderer::Linear);

    ID3D11ShaderResourceView* srv =  meshRenderer.ShadowMap().SRView;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srv->GetDesc(&srvDesc);
    srvDesc.Texture2DArray.ArraySize = 1;
    ID3D11ShaderResourceViewPtr newSrv;
    deviceManager.Device()->CreateShaderResourceView(meshRenderer.ShadowMap().Texture, &srvDesc, &newSrv);

    spriteRenderer.Render(newSrv, Float4x4());

    spriteRenderer.End();*/

    RenderHUD();
}

void ShadowsApp::RenderMainPass()
{
    PIXEvent event(L"Main Pass");

    ID3D11DeviceContextPtr context = deviceManager.ImmediateContext();

    ID3D11RenderTargetView* renderTargets[1] = { NULL };
    ID3D11DepthStencilView* ds = depthBuffer.DSView;

    context->OMSetRenderTargets(1, renderTargets, ds);

    D3D11_VIEWPORT vp;
    vp.Width = static_cast<float>(colorTarget.Width);
    vp.Height = static_cast<float>(colorTarget.Height);
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    context->RSSetViewports(1, &vp);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context->ClearRenderTargetView(colorTarget.RTView, clearColor);
    context->ClearDepthStencilView(ds, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    Float4x4 meshWorld = Float4x4::ScaleMatrix(MeshScales[AppSettings::CurrentScene]);
    Float4x4 characterWorld = Float4x4::ScaleMatrix(CharacterScale);
    Float4x4 characterOrientation = Quaternion::ToFloat4x4(AppSettings::CharacterOrientation);
    characterWorld = characterWorld * characterOrientation;
    characterWorld.SetTranslation(CharacterPos);

    {
        ProfileBlock block(L"Depth Prepass");
        if(AppSettings::GPUSceneSubmission)
            meshRenderer.RenderDepthGPU(context, camera, meshWorld, characterWorld, false);
        else
            meshRenderer.RenderDepthCPU(context, camera, meshWorld, characterWorld, false);
    }

    if(AppSettings::AutoComputeDepthBounds)
        meshRenderer.ReduceDepth(context, depthBuffer.SRView, camera);

    if(AppSettings::GPUSceneSubmission)
        meshRenderer.RenderShadowMapGPU(context, camera, meshWorld, characterWorld);
    else
        meshRenderer.RenderShadowMap(context, camera, meshWorld, characterWorld);

    renderTargets[0] = colorTarget.RTView;
    context->OMSetRenderTargets(1, renderTargets, ds);

    context->RSSetViewports(1, &vp);

    Float3 lightDir = AppSettings::LightDirection;
    meshRenderer.Render(context, camera, meshWorld, characterWorld);

    skybox.RenderSky(context, lightDir, true, camera.ViewMatrix(), camera.ProjectionMatrix());
}

void ShadowsApp::RenderHUD()
{
    PIXEvent event(L"HUD Pass");

    spriteRenderer.Begin(deviceManager.ImmediateContext(), SpriteRenderer::Point);

    Float4x4 transform = Float4x4::TranslationMatrix(Float3(25.0f, 25.0f, 0.0f));
    wstring fpsText(L"FPS: ");
    fpsText += ToString(fps) + L" (" + ToString(1000.0f / fps) + L"ms)";
    spriteRenderer.RenderText(font, fpsText.c_str(), transform, XMFLOAT4(1, 1, 0, 1));

    transform._42 += 25.0f;
    wstring vsyncText(L"VSYNC (V): ");
    vsyncText += deviceManager.VSYNCEnabled() ? L"Enabled" : L"Disabled";
    spriteRenderer.RenderText(font, vsyncText.c_str(), transform, XMFLOAT4(1, 1, 0, 1));

    spriteRenderer.End();
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    ShadowsApp app;
    app.Run();
}