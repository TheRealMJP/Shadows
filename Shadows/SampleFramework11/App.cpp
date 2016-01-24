//=================================================================================================
//
//  MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "PCH.h"

#include "App.h"
#include "Exceptions.h"
#include "Profiler.h"
#include "GUIObject.h"
#include "Math.h"
#include "LodePNG/lodepng.h"
#include "FileIO.h"
#include "Settings.h"
#include "TwHelper.h"

namespace SampleFramework11
{

App* GlobalApp = nullptr;

App::App(const wchar* appName, const wchar* iconResource) :   window(NULL, appName, WS_OVERLAPPEDWINDOW,
                                                                     WS_EX_APPWINDOW, 1280, 720, iconResource, iconResource),
                                                              currentTimeDeltaSample(0),
                                                              fps(0), tweakBar(nullptr), applicationName(appName)
{
    GlobalApp = this;
    for(uint32 i = 0; i < NumTimeDeltaSamples; ++i)
        timeDeltaBuffer[i] = 0;
}

App::~App()
{

}

void App::Run()
{
    try
    {
        Win32Call(AllocConsole());
        Win32Call(SetConsoleTitle(applicationName.c_str()));
        FILE* consoleFile = nullptr;
        freopen_s(&consoleFile, "CONOUT$", "wb", stdout);

        window.ShowWindow();

        deviceManager.Initialize(window);

        blendStates.Initialize(deviceManager.Device());
        rasterizerStates.Initialize(deviceManager.Device());
        depthStencilStates.Initialize(deviceManager.Device());
        samplerStates.Initialize(deviceManager.Device());

        // Create a font + SpriteRenderer
        font.Initialize(L"Arial", 18, SpriteFont::Regular, true, deviceManager.Device());
        spriteRenderer.Initialize(deviceManager.Device());

        Profiler::GlobalProfiler.Initialize(deviceManager.Device(), deviceManager.ImmediateContext());

        GUIObject::InitGlobalResources(deviceManager.Device());

        window.RegisterMessageCallback(WM_SIZE, OnWindowResized, this);

        // Initialize AntTweakBar
        TwCall(TwInit(TW_DIRECT3D11, deviceManager.Device()));

        // Create a tweak bar
        tweakBar = TwNewBar("Settings");
        TwCall(TwDefine(" GLOBAL help='MJPs sample framework for DX11' "));
        TwCall(TwDefine(" GLOBAL fontsize=3 "));

        Settings.Initialize(tweakBar);

        Initialize();

        AfterReset();

        while(window.IsAlive())
        {
            if(!window.IsMinimized())
            {
                timer.Update();
                Settings.Update();

                CalculateFPS();

                Update(timer);

                UpdateShaders(deviceManager.Device());

                Render(timer);

                // Render the profiler text
                spriteRenderer.Begin(deviceManager.ImmediateContext(), SpriteRenderer::Point);
                Profiler::GlobalProfiler.EndFrame(spriteRenderer, font);
                spriteRenderer.End();

                // Render the TweakBar UI
                TwCall(TwDraw());

                deviceManager.Present();
            }

            window.MessageLoop();
        }
    }
    catch (SampleFramework11::Exception exception)
    {
        exception.ShowErrorMessage();
    }

    ShutdownShaders();

    TwCall(TwTerminate());

    fclose(stdout);
    FreeConsole();
}

void App::CalculateFPS()
{
    timeDeltaBuffer[currentTimeDeltaSample] = timer.DeltaSecondsF();
    currentTimeDeltaSample = (currentTimeDeltaSample + 1) % NumTimeDeltaSamples;

    float averageDelta = 0;
    for(UINT i = 0; i < NumTimeDeltaSamples; ++i)
        averageDelta += timeDeltaBuffer[i];
    averageDelta /= NumTimeDeltaSamples;

    fps = static_cast<UINT>(std::floor((1.0f / averageDelta) + 0.5f));
}

LRESULT App::OnWindowResized(void* context, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    App* app = reinterpret_cast<App*>(context);

    if(!app->deviceManager.FullScreen() && wParam != SIZE_MINIMIZED)
    {
        int width, height;
        app->window.GetClientArea(width, height);

        if(width != app->deviceManager.BackBufferWidth() || height != app->deviceManager.BackBufferHeight())
        {
            app->BeforeReset();

            app->deviceManager.SetBackBufferWidth(width);
            app->deviceManager.SetBackBufferHeight(height);
            app->deviceManager.Reset();

            app->AfterReset();
        }
    }

    return 0;
}

void App::Exit()
{
    window.Destroy();
}

void App::Initialize()
{
}

void App::BeforeReset()
{
}

void App::AfterReset()
{
    const uint32 width = deviceManager.BackBufferWidth();
    const uint32 height = deviceManager.BackBufferHeight();
    captureTexture.Initialize(deviceManager.Device(), width, height, deviceManager.BackBufferFormat(), 1,
                              deviceManager.BackBufferMSCount(), deviceManager.BackBufferMSQuality(), 1);

    TwHelper::SetSize(tweakBar, 375, deviceManager.BackBufferHeight());
    TwHelper::SetPosition(tweakBar, deviceManager.BackBufferWidth() - 375, 0);
}

void App::ToggleFullScreen(bool fullScreen)
{
    if(fullScreen != deviceManager.FullScreen())
    {
        BeforeReset();

        deviceManager.SetFullScreen(fullScreen);
        deviceManager.Reset();

        AfterReset();
    }
}

void App::RenderText(const std::wstring& text, Float2 pos)
{
    ID3D11DeviceContext* context = deviceManager.ImmediateContext();

    // Set the backbuffer
    ID3D11RenderTargetView* rtvs[1] = { deviceManager.BackBuffer() };
    context->OMSetRenderTargets(1, rtvs, NULL);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context->ClearRenderTargetView(rtvs[0], clearColor);

    // Draw the text
    Float4x4 transform;
    transform.SetTranslation(Float3(pos.x, pos.y,0.0f));
    spriteRenderer.Begin(context, SpriteRenderer::Point);
    spriteRenderer.RenderText(font, text.c_str(), transform.ToSIMD());
    spriteRenderer.End();

    // Present
    deviceManager.SwapChain()->Present(0, 0);

    // Pump the message loop
    window.MessageLoop();
}

void App::RenderCenteredText(const std::wstring& text)
{

    // Measure the text
    Float2 textSize = font.MeasureText(text.c_str());

    // Position it in the middle
    Float2 textPos;
    textPos.x = Round((deviceManager.BackBufferWidth() / 2.0f) - (textSize.x / 2.0f));
    textPos.y = Round((deviceManager.BackBufferHeight() / 2.0f) - (textSize.y / 2.0f));

    RenderText(text, textPos);
}

void App::SaveScreenshot(const wchar* filePath)
{
    ID3D11DeviceContext* context = deviceManager.ImmediateContext();
    context->CopyResource(captureTexture.Texture, deviceManager.BackBufferTexture());

    const uint32 w = deviceManager.BackBufferWidth();
    const uint32 h = deviceManager.BackBufferHeight();
    std::vector<uint8> imageData(w * h * 4);

    uint32 pitch = 0;
    const uint8* srcData = reinterpret_cast<uint8*>(captureTexture.Map(context, 0, pitch));
    uint32* dstData = reinterpret_cast<uint32*>(imageData.data());
    for(uint32 y = 0; y < h; ++y)
    {
        memcpy(dstData, srcData, w * 4);

        // Set the alpha to 0xFF
        for(uint32 x = 0; x < w; ++x)
            dstData[x] |= 0xFF000000;

        srcData += pitch;
        dstData += w;
    }

    captureTexture.Unmap(context, 0);

    std::vector<uint8> fileData;
    uint32 result = lodepng::encode(fileData, imageData, w, h, LCT_RGBA, 8);
    if(result != 0)
        throw Exception(AnsiToWString(lodepng_error_text(result)));

    File pngFile(filePath, File::OpenWrite);
    pngFile.Write(fileData.size(), fileData.data());
}

}