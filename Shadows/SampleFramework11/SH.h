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
#include "GraphicsTypes.h"
#include "Math.h"

namespace SampleFramework11
{

// Constants
static const float CosineA0 = 1.0f;
static const float CosineA1 = 2.0f / 3.0f;
static const float CosineA2 = 0.25f;

template<typename T, uint64 N> class SH
{

protected:

    void Assign(const SH& other)
    {
        for(uint64 i = 0; i < N; ++i)
            Coefficients[i] = other.Coefficients[i];
    }

public:

    T Coefficients[N];

    SH()
    {
        for(uint64 i = 0; i < N; ++i)
            Coefficients[i] = 0.0f;
    }

    // Operator overloads
    T& operator[](uint64 idx)
    {
        return Coefficients[idx];
    }

    T operator[](uint64 idx) const
    {
        return Coefficients[idx];
    }

    SH& operator+=(const SH& other)
    {
        for(uint64 i = 0; i < N; ++i)
            Coefficients[i] += other.Coefficients[i];
        return *this;
    }

    SH operator+(const SH& other) const
    {
        SH result;
        for(uint64 i = 0; i < N; ++i)
            result.Coefficients[i] = Coefficients[i] + other.Coefficients[i];
        return result;
    }

    SH& operator-=(const SH& other)
    {
        for(uint64 i = 0; i < N; ++i)
            Coefficients[i] -= other.Coefficients[i];
        return *this;
    }

    SH operator-(const SH& other) const
    {
        SH result;
        for(uint64 i = 0; i < N; ++i)
            result.Coefficients[i] = Coefficients[i] - other.Coefficients[i];
        return result;
    }


    SH& operator*=(const T& scale)
    {
        for(uint64 i = 0; i < N; ++i)
            Coefficients[i] *= scale;
        return *this;
    }


    SH operator*(const SH& other) const
    {
        SH result;
        for(uint64 i = 0; i < N; ++i)
            result.Coefficients[i] = Coefficients[i] * other.Coefficients[i];
        return result;
    }

    SH& operator*=(const SH& other)
    {
        for(uint64 i = 0; i < N; ++i)
            Coefficients[i] *= other.Coefficients[i];
        return *this;
    }


    SH operator*(const T& scale) const
    {
        SH result;
        for(uint64 i = 0; i < N; ++i)
            result.Coefficients[i] = Coefficients[i] * scale;
        return result;
    }

    SH& operator/=(const T& scale)
    {
        for(uint32 i = 0; i < N; ++i)
            Coefficients[i] /= scale;
        return *this;
    }

    SH operator/(const T& scale) const
    {
        SH result;
        for(uint64 i = 0; i < N; ++i)
            result.Coefficients[i] = Coefficients[i] / scale;
        return result;
    }

    // Dot products
    T Dot(const SH& other) const
    {
        T result = 0.0f;
        for(uint64 i = 0; i < N; ++i)
            result += Coefficients[i] * other.Coefficients[i];
        return result;
    }

    static T Dot(const SH& a, const SH& b)
    {
        T result = 0.0f;
        for(uint64 i = 0; i < N; ++i)
            result += a.Coefficients[i] * b.Coefficients[i];
        return result;
    }

    // Convolution with cosine kernel
    void ConvolveWithCosineKernel()
    {
        Coefficients[0] *= CosineA0;

        for(uint64 i = 1; i < N; ++i)
            if(i < 4)
                Coefficients[i] *= CosineA1;
            else if(i < 9)
                Coefficients[i] *= CosineA2;
    }
};

// Spherical Harmonics
typedef SH<float, 4> SH4;
typedef SH<Float3, 4> SH4Color;
typedef SH<float, 9> SH9;
typedef SH<Float3, 9> SH9Color;

// H-basis
class H4 : public SH<float, 4>
{

public:

    H4() : SH<float, 4>()
    {
    }

    H4(float h0, float h1, float h2, float h3) : SH<float, 4>()
    {
        Coefficients[0] = h0;
        Coefficients[1] = h1;
        Coefficients[2] = h2;
        Coefficients[3] = h3;
    }

    H4(const SH<float, 4>& sh)
    {
        Assign(sh);
    }
};

typedef SH<Float3, 4> H4Color;

// For proper alignment with shader constant buffers
struct ShaderSH9Color
{
    Float4 Coefficients[9];

    ShaderSH9Color()
    {
    }

    ShaderSH9Color(const SH9Color& sh9Clr)
    {
        for(uint32 i = 0; i < 9; ++i)
            Coefficients[i] = Float4(sh9Clr.Coefficients[i], 0.0f);
    }
};

SH9 ProjectOntoSH9(const Float3& dir);
SH9Color ProjectOntoSH9Color(const Float3& dir, const Float3& color);

// H-basis functions
H4 ProjectOntoH4(const Float3& dir);
float EvalH4(const H4& h, const Float3& dir);
H4 ConvertToH4(const SH9& sh);

// Lighting environment generation functions
SH9Color ProjectCubemapToSH(ID3D11DeviceContext* context, ID3D11ShaderResourceView* cubeMap);

// Constants
static const H4 H4Identity = H4(std::sqrt(2.0f * 3.14159f), 0.0f, 0.0f, 0.0f);

}