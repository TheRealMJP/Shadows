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

#include "Slider.h"

#include "Utility.h"
#include "Input.h"
#include "SpriteRenderer.h"

namespace SampleFramework11
{

Slider::Slider() :  size(200.0f, 25.0f),
                    minVal(0.0f),
                    maxVal(100.0f),
                    numSteps(100),
                    value(0),
                    dragging(false),
                    hover(false),
                    dragValue(0),
                    mouseDown(false),
                    changed(false)
{
}

Slider::~Slider()
{
}

void Slider::Initialize(ID3D11Device* device, float minVal, float maxVal, float value, const wchar* name)
{
    enabled = true;

    this->minVal = minVal;
    this->maxVal = maxVal;
    this->value = value;
    this->name = name;
}

void Slider::CalcValue(float normalizedValue)
{
    float stepsValue = normalizedValue * numSteps;
    stepsValue = std::floorf(stepsValue);
    normalizedValue = stepsValue / numSteps;
    value = minVal + ((maxVal - minVal) * normalizedValue);
}

void Slider::Update(const KeyboardState& kbState, const MouseState& mouseState)
{
    changed = false;

    if(!enabled)
    {
        dragging = false;
        mouseDown = false;
        hover = false;
        return;
    }

    // Figure out the x value of the mouse relative to the slider
    float mouseX = mouseState.X - position.x;
    float mouseY = mouseState.Y - position.y;

    // Figure out if we're over the knob
    XMFLOAT4 knobBounds = GetKnobBounds();
    float left = knobBounds.x - position.x;
    float top = knobBounds.y - position.y;
    float right = left + knobBounds.z * TextureHeight;
    float bottom = top + knobBounds.w * TextureHeight;
    bool knobHover = mouseX >= left && mouseX <= right && mouseY >= top && mouseY <= bottom;
    hover = knobHover;

    float oldValue = value;

    if (mouseState.LButton.Pressed)
    {
        bool clicked = !mouseDown;
        mouseDown = true;

        if (clicked && knobHover && !dragging)
        {
            dragPos = XMFLOAT2(mouseX, mouseY);
            dragging = true;
            dragValue = value;
        }
        else if (dragging)
        {
            float offset = (dragValue - minVal) / (maxVal - minVal);
            float normalizedValue = Clamp<float>(offset + ((mouseX - dragPos.x) / size.x), 0, 1);
            CalcValue(normalizedValue);
        }
    }
    else
    {
        mouseDown = false;
        dragging = false;
    }

    if(oldValue != value)
        changed = true;
}

Float4 Slider::GetKnobBounds()
{
    float normalizedValue = (value - minVal) / (maxVal - minVal);
    float scaleX = size.y / TextureHeight;
    float scaleY = size.y / TextureHeight;
    float posX = position.x + (normalizedValue * size.x) - (size.y / 2.0f);
    float posY = position.y - (size.y / 2.5f);
    return Float4(posX, posY, scaleX, scaleY);
}

void Slider::Render(SpriteRenderer& renderer, SpriteFont& defaultFont)
{
    value = Clamp(value, minVal, maxVal);

    float alpha = enabled ? 1.0f : 0.35f;

    // Render the bar
    Float4x4 transform = Float4x4::ScaleMatrix(Float3(size.x / TextureWidth, size.y / TextureHeight, 1.0f));
    transform._41 = position.x;
    transform._42 = position.y;
    renderer.Render(barTexture, transform, Float4(1, 1, 1, alpha));

    // Render the knob
    Float4 knobBounds = GetKnobBounds();
    transform = Float4x4::ScaleMatrix(Float3(knobBounds.w, knobBounds.z, 1.0f));
    transform._41 = knobBounds.x;
    transform._42 = knobBounds.y;
    Float4 knobColor = hover ? Float4(1.5f, 1.5f, 1.5f, alpha) : Float4(1, 1, 1, alpha);
    renderer.Render(knobTexture, transform, knobColor);

    transform = Float4x4::TranslationMatrix(Float3(position.x, position.y + size.y - 12.0f, 0));
    renderer.RenderText(font, name.c_str(), transform, Float4(1, 1, 1, alpha));

    std::wstring valString = ToString(value);
    transform = Float4x4::TranslationMatrix(Float3(position.x + size.x + 16.0f, position.y - 4.0f, 0));
    renderer.RenderText(font, valString.c_str(), transform, Float4(1, 1, 1, alpha));
}

}
