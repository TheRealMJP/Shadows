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

#include "TextGUI.h"

#include "Utility.h"
#include "SpriteRenderer.h"
#include "Input.h"

namespace SampleFramework11
{

// == TextGUI =====================================================================================

TextGUI::TextGUI(const wchar* name, uint32 value, uint32 numValues, const wchar** valueNames, KeyboardState::Keys toggleKey)
    : name(name), value(value), numValues(numValues), toggleKey(toggleKey), changed(false)
{
    _ASSERT(numValues > 0);
    _ASSERT(value < numValues);
    SetNames(valueNames);
}

TextGUI::TextGUI(const wchar* name, uint32 value, uint32 numValues, KeyboardState::Keys toggleKey)
    : name(name), value(value), numValues(numValues), toggleKey(toggleKey), changed(false)
{
    _ASSERT(numValues > 0);
    _ASSERT(value < numValues);
}

void TextGUI::SetNames(const wchar** valueNames)
{
    for(uint32 i = 0; i < numValues; ++i)
        this->valueNames.push_back(valueNames[i]);
}

void TextGUI::Update(const KeyboardState& kbState, const MouseState& mouseState)
{
    changed = false;

    if(kbState.RisingEdge(toggleKey))
    {
        if(kbState.IsKeyDown(KeyboardState::RightShift))
        {
            INT32 value_ = value;
            value_ -= 1;
            if(value_ == -1)
                value_ = numValues - 1;
            value = value_;
        }
        else
            value = (value + 1) % numValues;
        changed = true;
    }
}

void TextGUI::Render(SpriteRenderer& renderer, SpriteFont& defaultFont)
{
    XMMATRIX translation = XMMatrixTranslation(position.x, position.y, 0.0f);
    std::wstring text = name;
    text += L"(";
    text += static_cast<wchar>(toggleKey);
    text += L"): ";
    text += valueNames[value];
    renderer.RenderText(defaultFont, text.c_str(), translation, XMFLOAT4(1, 1, 0, 1));
}

// == BoolGUI =====================================================================================

static const wchar* BoolNames[2] = { L"Disabled", L"Enabled" };

BoolGUI::BoolGUI(const wchar* name, bool value, KeyboardState::Keys toggleKey)
    : TextGUI(name, value, 2, BoolNames, toggleKey)
{
}


}