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

#include "SampleFramework11/ShaderCompilation.h"
#include "SampleFramework11/Profiler.h"

#include "PostProcessor.h"
#include "SharedConstants.h"

// Constants
static const uint32 TGSize = 16;
static const uint32 LumMapSize = 1024;

void PostProcessor::Initialize(ID3D11Device* device)
{
    PostProcessorBase::Initialize(device);

    constantBuffer.Initialize(device);

    // Load the shaders
    bloomThreshold = CompilePSFromFile(device, L"PostProcessing.hlsl", "Threshold");
    bloomBlurH = CompilePSFromFile(device, L"PostProcessing.hlsl", "BloomBlurH");
    bloomBlurV = CompilePSFromFile(device, L"PostProcessing.hlsl", "BloomBlurV");
    luminanceMap = CompilePSFromFile(device, L"PostProcessing.hlsl", "LuminanceMap");
    composite = CompilePSFromFile(device, L"PostProcessing.hlsl", "Composite");
    scale = CompilePSFromFile(device, L"PostProcessing.hlsl", "Scale");
    adaptLuminance = CompilePSFromFile(device, L"PostProcessing.hlsl", "AdaptLuminance");
    drawDepth = CompilePSFromFile(device, L"PostProcessing.hlsl", "DrawDepth");
    drawDepthMSAA = CompilePSFromFile(device, L"PostProcessing.hlsl", "DrawDepthMSAA");

    // Create average luminance calculation targets
    currLumTarget = 0;
    adaptedLuminance[0].Initialize(device, 1, 1, DXGI_FORMAT_R32_FLOAT);
    adaptedLuminance[1].Initialize(device, 1, 1, DXGI_FORMAT_R32_FLOAT);
}

void PostProcessor::AfterReset(uint32 width, uint32 height)
{
    PostProcessorBase::AfterReset(width, height);
}

void PostProcessor::SetConstants(const Constants& constants)
{
    constantBuffer.Data = constants;
}

void PostProcessor::Render(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* input,
                            ID3D11RenderTargetView* output)
{
    PostProcessorBase::Render(deviceContext, input, output);

    constantBuffer.ApplyChanges(deviceContext);
    constantBuffer.SetPS(deviceContext, 1);

    // Calculate the average luminance first
    CalcAvgLuminance(input);

    // Now do the bloom
    TempRenderTarget* bloom = NULL;
    Bloom(input, bloom);

    // Apply tone mapping
    ToneMap(input, bloom->SRView, output);

    bloom->InUse = false;

    currLumTarget = !currLumTarget;
}

void PostProcessor::CalcAvgLuminance(ID3D11ShaderResourceView* input)
{
    TempRenderTarget* initialLuminance = GetTempRenderTarget(LumMapSize, LumMapSize, DXGI_FORMAT_R32_FLOAT, 1, 0, 0, TRUE);

    // Luminance mapping
    PostProcess(input, initialLuminance->RTView, luminanceMap, L"Luminance Map Generation");

    // Generate the mip chain
    context->GenerateMips(initialLuminance->SRView);

    // Adaptation
    inputs.push_back(adaptedLuminance[!currLumTarget].SRView);
    inputs.push_back(initialLuminance->SRView);
    outputs.push_back(adaptedLuminance[currLumTarget].RTView);
    PostProcess(adaptLuminance, L"Luminance Adaptation");

    initialLuminance->InUse = FALSE;
}

void PostProcessor::Bloom(ID3D11ShaderResourceView* input, TempRenderTarget*& bloomTarget)
{
    // Downscale
    TempRenderTarget* bloomSrc = GetTempRenderTarget(inputWidth, inputHeight, DXGI_FORMAT_R11G11B10_FLOAT);

    inputs.push_back(input);
    inputs.push_back(adaptedLuminance[currLumTarget].SRView);
    outputs.push_back(bloomSrc->RTView);
    PostProcess(bloomThreshold, L"Bloom Threshold");

    TempRenderTarget* downscale1 = GetTempRenderTarget(inputWidth / 2, inputHeight / 2, DXGI_FORMAT_R11G11B10_FLOAT);
    PostProcess(bloomSrc->SRView, downscale1->RTView, scale, L"Bloom Downscale");
    bloomSrc->InUse = false;

    TempRenderTarget* downscale2 = GetTempRenderTarget(inputWidth / 4, inputHeight / 4, DXGI_FORMAT_R11G11B10_FLOAT);
    PostProcess(downscale1->SRView, downscale2->RTView, scale, L"Bloom Downscale");

    TempRenderTarget* downscale3 = GetTempRenderTarget(inputWidth / 8, inputHeight / 8, DXGI_FORMAT_R11G11B10_FLOAT);
    PostProcess(downscale2->SRView, downscale3->RTView, scale, L"Bloom Downscale");

    // Blur it
    for (uint64 i = 0; i < 4; ++i)
    {
        TempRenderTarget* blurTemp = GetTempRenderTarget(inputWidth / 8, inputHeight / 8, DXGI_FORMAT_R11G11B10_FLOAT);
        PostProcess(downscale3->SRView, blurTemp->RTView, bloomBlurH, L"Horizontal Bloom Blur");

        PostProcess(blurTemp->SRView, downscale3->RTView, bloomBlurV, L"Vertical Bloom Blur");
        blurTemp->InUse = false;
    }

    PostProcess(downscale3->SRView, downscale2->RTView, scale, L"Bloom Upscale");
    downscale3->InUse = false;

    PostProcess(downscale2->SRView, downscale1->RTView, scale, L"Bloom Upscale");
    downscale2->InUse = false;

    bloomTarget = downscale1;
}

void PostProcessor::ToneMap(ID3D11ShaderResourceView* input, ID3D11ShaderResourceView* bloom,
                            ID3D11RenderTargetView* output)
{
    // Composite the bloom with the original image, and apply tone-mapping
    inputs.push_back(input);
    inputs.push_back(adaptedLuminance[currLumTarget].SRView);
    inputs.push_back(bloom);
    outputs.push_back(output);
    PostProcess(composite, L"Tone Map And Composite");
}

void PostProcessor::DrawDepthBuffer(DepthStencilBuffer& depthBuffer, ID3D11RenderTargetView* rt) {
    Assert_(depthBuffer.SRView != nullptr);
    if(depthBuffer.MultiSamples > 1)
        PostProcess(depthBuffer.SRView, rt, drawDepthMSAA, L"Draw Depth");
    else
        PostProcess(depthBuffer.SRView, rt, drawDepth, L"Draw Depth");
}