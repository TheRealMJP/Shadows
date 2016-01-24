//=================================================================================================
//
//  MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#pragma once

#include "PCH.h"

#include "SpriteFont.h"

namespace SampleFramework11
{

class KeyboardState;
class MouseState;
class SpriteRenderer;

class GUIObject
{

public:

    virtual void Update(const KeyboardState& kbState, const MouseState& mouseState) = 0;
    virtual void Render(SpriteRenderer& renderer, SpriteFont& defaultFont) = 0;

    XMFLOAT2& Position() { return position; };
    XMFLOAT2 Position() const { return position; };

    bool& Enabled() { return enabled; };
    bool Enabled() const { return enabled; };

    static void InitGlobalResources(ID3D11Device* device);

protected:

    XMFLOAT2 position;
    bool enabled;

    // Global shared resources
    static SpriteFont font;
    static ID3D11ShaderResourceViewPtr barTexture;
    static ID3D11ShaderResourceViewPtr knobTexture;
};

}