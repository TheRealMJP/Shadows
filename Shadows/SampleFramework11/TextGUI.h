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

#include "GUIObject.h"
#include "Input.h"

namespace SampleFramework11
{

class TextGUI : public GUIObject
{

public:

    void Update(const KeyboardState& kbState, const MouseState& mouseState);
    void Render(SpriteRenderer& renderer, SpriteFont& defaultFont);

    operator uint32() const { return value; }
    bool Changed() const { return changed; }
    void SetValue(uint32 newValue) { value = std::min(newValue, numValues - 1); }
    const std::wstring& ValueName() const { return valueNames[value]; }

protected:

    TextGUI(const wchar* name, uint32 value, uint32 numValues, const wchar** valueNames, KeyboardState::Keys toggleKey);
    TextGUI(const wchar* name, uint32 value, uint32 numValues, KeyboardState::Keys toggleKey);
    void SetNames(const wchar** valueNames);

    std::wstring name;
    uint32 value;
    uint32 numValues;
    std::vector<std::wstring> valueNames;
    KeyboardState::Keys toggleKey;
    bool changed;
};

class BoolGUI : public TextGUI
{

public:

    BoolGUI(const wchar* name, bool value, KeyboardState::Keys toggleKey);
};

}