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

#include "InterfacePointers.h"
#include "GUIObject.h"
#include "SpriteFont.h"
#include "Utility.h"

namespace SampleFramework11
{

class Slider : public GUIObject
{

public:

    Slider();
    ~Slider();

    void Initialize(ID3D11Device* device, float minVal, float maxVal, float value, const wchar* name);

    void Update(const KeyboardState& kbState, const MouseState& mouseState);
    void Render(SpriteRenderer& renderer, SpriteFont& defaultFont);

    float& MinVal() { return minVal; }
    float& MaxVal() { return maxVal; }
    UINT& NumSteps() { return numSteps; }
    std::wstring& Name() { return name; }

    float MinVal() const { return minVal; }
    float MaxVal() const { return maxVal; }
    UINT NumSteps() const { return numSteps; }
    float Value() const { return value; }
    std::wstring Name() const { return name; }
    bool Changed() const { return changed; }

    void SetValue(float newValue) { value = Clamp(newValue, minVal, maxVal); }

    operator float() const { return Value(); }

    uint32 AsUint() const { return static_cast<uint32>(Round(Value())); }

protected:

    static const UINT TextureWidth = 512;
    static const UINT TextureHeight = 64;

    Float2 size;
    float minVal;
    float maxVal;
    UINT numSteps;
    float value;
    std::wstring name;
    bool dragging;
    float dragValue;
    bool hover;
    bool mouseDown;
    Float2 dragPos;
    bool changed;

    Float4 GetKnobBounds();
    void CalcValue(float normalizedValue);
};

}