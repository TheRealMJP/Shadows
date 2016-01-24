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

#include "SampleFramework11/App.h"
#include "SampleFramework11/InterfacePointers.h"
#include "SampleFramework11/Camera.h"
#include "SampleFramework11/Model.h"
#include "SampleFramework11/SpriteFont.h"
#include "SampleFramework11/SpriteRenderer.h"
#include "SampleFramework11/Skybox.h"
#include "SampleFramework11/GraphicsTypes.h"
#include "SampleFramework11/Slider.h"
#include "SampleFramework11/SH.h"

#include "PostProcessor.h"
#include "MeshRenderer.h"

using namespace SampleFramework11;

class ShadowsApp : public App
{

protected:

    FirstPersonCamera camera;

    Skybox skybox;

    PostProcessor postProcessor;
    DepthStencilBuffer depthBuffer;
    RenderTarget2D colorTarget;
    RenderTarget2D resolveTarget;

    // Model
    Model models[uint64(Scene::NumValues)];
    Model characterMesh;
    MeshRenderer meshRenderer;

    virtual void Initialize() override;
    virtual void Render(const Timer& timer) override;
    virtual void Update(const Timer& timer) override;
    virtual void BeforeReset() override;
    virtual void AfterReset() override;

    void CreateRenderTargets();

    void RenderMainPass();
    void RenderHUD();

public:

    ShadowsApp();
};

