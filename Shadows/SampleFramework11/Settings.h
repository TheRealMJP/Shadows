//=================================================================================================
//
//	MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#pragma once

#include "PCH.h"
#include "Math.h"

namespace SampleFramework11
{

class FloatSetting;
class IntSetting;
class BoolSetting;
class EnumSetting;
class DirectionSetting;
class OrientationSetting;
class ColorSetting;

enum class SettingType
{
    Float = 0,
    Int = 1,
    Bool = 2,
    Enum = 3,
    Direction = 4,
    Orientation = 5,
    Color = 6,

    Invalid,
    NumTypes = Invalid
};

// Base class for all setting types
class Setting
{

protected:

    TwBar* tweakBar;
    SettingType type;
    void* data;
    std::string name;
    std::string group;
    std::string label;
    std::string helpText;
    bool changed;

    void Initialize(TwBar* tweakBar, SettingType type, void* data, const char* name,
                    const char* group, const char* label, const char* helpText,
                    ETwType twType = TW_TYPE_UNDEF);

public:

    Setting();

    virtual void Update() = 0;

    void SetReadOnly(bool readOnly);
    void SetEditable(bool editable);

    FloatSetting& AsFloat();
    IntSetting& AsInt();
    BoolSetting& AsBool();
    EnumSetting& AsEnum();
    DirectionSetting& AsDirection();
    OrientationSetting& AsOrientation();
    ColorSetting& AsColor();

    bool Changed() const;
    const std::string& Name() const;
};

// 32-bit float setting
class FloatSetting : public Setting
{

private:

    float val;
    float oldVal;
    float minVal;
    float maxVal;
    float step;

public:

    FloatSetting();

    void Initialize(TwBar* tweakBar, const char* name, const char* group,
                    const char* label, const char* helpText, float initialVal,
                    float minVal, float maxVal, float step);

    virtual void Update() override;

    float Value() const;
    void SetValue(float newVal);
    operator float();

    float MinValue() const { return minVal; };
    float MaxValue() const { return maxVal; };
};

// 32-bit integer setting
class IntSetting : public Setting
{

private:

    int32 val;
    int32 oldVal;
    int32 minVal;
    int32 maxVal;

public:

    IntSetting();

    void Initialize(TwBar* tweakBar, const char* name, const char* group,
                    const char* label, const char* helpText, int32 initialVal,
                    int32 minVal, int32 maxVal);

    virtual void Update() override;

    int32 Value() const;
    void SetValue(int32 newVal);
    operator int32();
};

// Boolean setting
class BoolSetting : public Setting
{

private:

    bool32 val;
    int32 oldVal;

public:

    BoolSetting();

    void Initialize(TwBar* tweakBar, const char* name, const char* group,
                    const char* label, const char* helpText, bool32 initialVal);

    virtual void Update() override;

    bool32 Value() const;
    void SetValue(bool32 newVal);
    operator bool32();
};

// Enumeration setting
class EnumSetting : public Setting
{

protected:

    uint32 val;
    uint32 oldVal;
    uint32 numValues;

public:

    EnumSetting();

    void Initialize(TwBar* tweakBar, const char* name, const char* group,
                    const char* label, const char* helpText, uint32 initialVal,
                    uint32 numValues, const char* const* valueLabels);

    virtual void Update() override;

    uint32 Value() const;
    void SetValue(uint32 newVal);

    operator uint32();
};

// Templated enumeration setting
template<typename T> class EnumSettingT : public EnumSetting
{

public:

    EnumSettingT()
    {
    }

    void Initialize(TwBar* tweakBar, const char* name, const char* group,
                    const char* label, const char* helpText, T initialVal,
                    uint32 numValues, const char* const* valueLabels)
    {
        EnumSetting::Initialize(tweakBar, name, group, label, helpText,
                                uint32(initialVal), numValues, valueLabels);
    }

    operator T()
    {
        return T(val);
    }

    void SetValue(T newVal)
    {
        SetValue(uint32(newVal));
    }
};

// 3D direction setting
class DirectionSetting : public Setting
{

private:

    Float3 val;
    Float3 oldVal;

public:

    DirectionSetting();

    void Initialize(TwBar* tweakBar, const char* name, const char* group,
                    const char* label, const char* helpText, Float3 initialVal);

    virtual void Update() override;

    Float3 Value() const;
    void SetValue(Float3 newVal);
    operator Float3();
};

// Quaternion orientation setting
class OrientationSetting : public Setting
{

private:

    Quaternion val;
    Quaternion oldVal;

public:

    OrientationSetting();

    void Initialize(TwBar* tweakBar, const char* name, const char* group,
                    const char* label, const char* helpText, Quaternion initialVal);

    virtual void Update() override;

    Quaternion Value() const;
    void SetValue(Quaternion newVal);
    operator Quaternion();
};

// RGB color setting
class ColorSetting : public Setting
{

private:

    Float3 val;
    Float3 oldVal;

    FloatSetting intensity;
    bool hdr;

public:

    ColorSetting();

    void Initialize(TwBar* tweakBar, const char* name, const char* group,
                    const char* label, const char* helpText, Float3 initialVal,
                    bool hdr, float minIntensity, float maxIntensity,  float step);

    virtual void Update() override;

    Float3 Value() const;
    void SetValue(Float3 newVal);
    operator Float3();
};

// Container for settings (essentially wraps a tweak bar)
class SettingsContainer
{

private:

    TwBar* tweakBar;
    std::map<std::string, Setting*> settings;
    std::vector<Setting*> allocatedSettings;

public:

    SettingsContainer();
    ~SettingsContainer();

    void Initialize(TwBar* tweakBar);

    void Update();

    // Access settings by name
    Setting& operator[](const std::string& name);

    // Set container properties
    void SetGroupOpened(const char* groupName, bool opened);

    // Add new settings
    void AddFloatSetting(const char* name, const char* label, const char* group,
                         float initialVal, float minVal = 0.0f, float maxVal = 1.0f,
                         float step = 0.01f, const char* helpText = "");

    void AddIntSetting(const char* name, const char* label, const char* group,
                       int32 initialVal, int32 minVal = INT_MIN, int32 maxVal = INT_MAX,
                       const char* helpText = "");

    void AddBoolSetting(const char* name, const char* label, const char* group,
                        bool32 initialVal, const char* helpText = "");

    void AddDirectionSetting(const char* name, const char* label, const char* group,
                             Float3 initialVal, const char* helpText = "");

    void AddOrientationSetting(const char* name, const char* label, const char* group,
                               Quaternion initialVal, const char* helpText = "");

    void AddEnumSetting(const char* name, const char* label, const char* group,
                        uint32 initialVal, uint32 numValues, const char* const* valueLabels,
                        const char* helpText = "");

    void AddColorSetting(const char* name, const char* label, const char* group,
                         Float3 initialVal, bool hdr = false, float minIntensity = 0.0f,
                         float maxIntensity = 1.0f, float step = 0.01f,
                         const char* helpText = "");

    // Add existing setting
    void AddSetting(Setting* setting);

    TwBar* TweakBar() { return tweakBar; }
};

extern SettingsContainer Settings;

}