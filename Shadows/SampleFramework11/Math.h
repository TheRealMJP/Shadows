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
#include "Assert.h"

namespace SampleFramework11
{

// Forward declarations
struct Quaternion;
struct Float3x3;
struct Float4x4;

// Extension classes for XMFLOAT* classes

struct Float2 : public XMFLOAT2
{
    Float2();
    Float2(float x);
    Float2(float x, float y);
    Float2(const XMFLOAT2& xy);
    Float2(FXMVECTOR xy);

    Float2& operator+=(const Float2& other);
    Float2 operator+(const Float2& other) const;

    Float2& operator-=(const Float2& other);
    Float2 operator-(const Float2& other) const;

    Float2& operator*=(const Float2& other);
    Float2 operator*(const Float2& other) const;

    Float2& operator/=(const Float2& other);
    Float2 operator/(const Float2& other) const;

    bool operator==(const Float2& other) const;
    bool operator!=(const Float2& other) const;

    Float2 operator-() const;

    XMVECTOR ToSIMD() const;

    static Float2 Clamp(const Float2& val, const Float2& min, const Float2& max);
    static float Length(const Float2& val);
};

struct Float3 : public XMFLOAT3
{
    Float3();
    Float3(float x);
    Float3(float x, float y, float z);
    Float3(const XMFLOAT3& xyz);
    Float3(FXMVECTOR xyz);

    Float3& operator+=(const Float3& other);
    Float3 operator+(const Float3& other) const;

    Float3& operator+=(float other);
    Float3 operator+(float other) const;

    Float3& operator-=(const Float3& other);
    Float3 operator-(const Float3& other) const;

    Float3& operator-=(float other);
    Float3 operator-(float other) const;

    Float3& operator*=(const Float3& s);
    Float3 operator*(const Float3& s) const;

    Float3& operator*=(float other);
    Float3 operator*(float other) const;

    Float3& operator/=(const Float3& other);
    Float3 operator/(const Float3& other) const;

    Float3& operator/=(float other);
    Float3 operator/(float other) const;

    bool operator==(const Float3& other) const;
    bool operator!=(const Float3& other) const;

    Float3 operator-() const;

    XMVECTOR ToSIMD() const;

    float Length() const;

    static float Dot(const Float3& a, const Float3& b);
    static Float3 Cross(const Float3& a, const Float3& b);
    static Float3 Normalize(const Float3& a);
    static Float3 Transform(const Float3& v, const Float3x3& m);
    static Float3 Transform(const Float3& v, const Float4x4& m);
    static Float3 TransformDirection(const Float3&v, const Float4x4& m);
    static Float3 Transform(const Float3& v, const Quaternion& q);
    static Float3 Clamp(const Float3& val, const Float3& min, const Float3& max);
    static Float3 Perpendicular(const Float3& v);
    static float Distance(const Float3& a, const Float3& b);
    static float Length(const Float3& v);
};

// Non-member operators of Float3
Float3 operator*(float a, const Float3& b);

struct Float4 : public XMFLOAT4
{
    Float4();
    Float4(float x);
    Float4(float x, float y, float z, float w);
    Float4(const XMFLOAT3& xyz, float w = 0.0f);
    Float4(const XMFLOAT4& xyzw);
    Float4(FXMVECTOR xyzw);

    Float4& operator+=(const Float4& other);
    Float4 operator+(const Float4& other) const;

    Float4& operator-=(const Float4& other);
    Float4 operator-(const Float4& other) const;

    Float4& operator*=(const Float4& other);
    Float4 operator*(const Float4& other) const;

    Float4& operator/=(const Float4& other);
    Float4 operator/(const Float4& other) const;

    bool operator==(const Float4& other) const;
    bool operator!=(const Float4& other) const;

    Float4 operator-() const;

    XMVECTOR ToSIMD() const;
    Float3 To3D() const;

    static Float4 Clamp(const Float4& val, const Float4& min, const Float4& max);
};

struct Quaternion : public XMFLOAT4
{
    Quaternion();
    Quaternion(float x, float y, float z, float w);
    Quaternion(const Float3& axis, float angle);
    Quaternion(const Float3x3& m);
    Quaternion(const XMFLOAT4& q);
    Quaternion(FXMVECTOR q);

    Quaternion& operator*=(const Quaternion& other);
    Quaternion operator*(const Quaternion& other) const;

    bool operator==(const Quaternion& other) const;
    bool operator!=(const Quaternion& other) const;

    Float3x3 ToFloat3x3() const;
    Float4x4 ToFloat4x4() const;

    static Quaternion Identity();
    static Quaternion Invert(const Quaternion& q);
    static Quaternion FromAxisAngle(const Float3& axis, float angle);
    static Quaternion Normalize(const Quaternion& q);
    static Float3x3 ToFloat3x3(const Quaternion& q);
    static Float4x4 ToFloat4x4(const Quaternion& q);

    XMVECTOR ToSIMD() const;
};

struct Float3x3 : public XMFLOAT3X3
{
    Float3x3();
    Float3x3(const XMFLOAT3X3& m);
    Float3x3(CXMMATRIX m);

    Float3x3& operator*=(const Float3x3& other);
    Float3x3 operator*(const Float3x3& other) const;

    Float3 Up() const;
    Float3 Down() const;
    Float3 Left() const;
    Float3 Right() const;
    Float3 Forward() const;
    Float3 Back() const;

    void SetXBasis(const Float3& x);
    void SetYBasis(const Float3& y);
    void SetZBasis(const Float3& z);

    static Float3x3 Transpose(const Float3x3& m);
    static Float3x3 Invert(const Float3x3& m);
    static Float3x3 ScaleMatrix(float s);
    static Float3x3 ScaleMatrix(const Float3& s);

    XMMATRIX ToSIMD() const;
};

struct Float4x4 : public XMFLOAT4X4
{
    Float4x4();
    Float4x4(const XMFLOAT4X4& m);
    Float4x4(CXMMATRIX m);

    Float4x4& operator*=(const Float4x4& other);
    Float4x4 operator*(const Float4x4& other) const;

    Float3 Up() const;
    Float3 Down() const;
    Float3 Left() const;
    Float3 Right() const;
    Float3 Forward() const;
    Float3 Back() const;

    Float3 Translation() const;
    void SetTranslation(const Float3& t);

    void SetXBasis(const Float3& x);
    void SetYBasis(const Float3& y);
    void SetZBasis(const Float3& z);

    static Float4x4 Transpose(const Float4x4& m);
    static Float4x4 Invert(const Float4x4& m);
    static Float4x4 ScaleMatrix(float s);
    static Float4x4 ScaleMatrix(const Float3& s);
    static Float4x4 TranslationMatrix(const Float3& t);

    XMMATRIX ToSIMD() const;

    std::string Print() const;
};

// Unsigned 32-bit integer vector classes
struct Uint2
{
    uint32 x;
    uint32 y;

    Uint2();
    Uint2(uint32 x, uint32 y);
};

struct Uint3
{
    uint32 x;
    uint32 y;
    uint32 z;

    Uint3();
    Uint3(uint32 x, uint32 y, uint32 z);
};

struct Uint4
{
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 w;

    Uint4();
    Uint4(uint32 x, uint32 y, uint32 z, uint32 w);
};

// General math functions

// Linear interpolation
template<typename T> T Lerp(const T& x, const T& y, float s)
{
    return x + (y - x) * s;
}

// Clamps a value to the specified range
template<typename T> T Clamp(T val, T min, T max)
{
    Assert_(max >= min);

    if(val < min)
        val = min;
    else if(val > max)
        val = max;
    return val;
}

// Clamps a value to [0, 1]
template<typename T> T Saturate(T val)
{
    return Clamp<T>(val, T(0.0f), T(1.0f));
}

// Rounds a float
inline float Round(float r)
{
    return (r > 0.0f) ? std::floorf(r + 0.5f) : std::ceilf(r - 0.5f);
}

// Returns a random float value between 0 and 1
inline float RandFloat()
{
    return rand() /  static_cast<float>(RAND_MAX);
}

}