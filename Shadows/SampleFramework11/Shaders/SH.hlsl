struct SH4Color
{
	float3 c[4];
};

struct SH9Color
{
	float3 c[9];
};

typedef float4 H4;

struct H4Color
{
    float3 c[4];
};

struct H6Color
{
    float3 c[6];
};

// Cosine kernel for SH
static const float CosineA0 = 1.0f;
static const float CosineA1 = 2.0f / 3.0f;
static const float CosineA2 = 0.25f;

// == SH4 =========================================================================================

//-------------------------------------------------------------------------------------------------
// Projects a direction onto SH and convolves with a given kernel
//-------------------------------------------------------------------------------------------------
SH4Color ProjectOntoSH4(in float3 n, in float3 color, in float A0, in float A1)
{
    SH4Color sh;

    // Band 0
    sh.c[0] = 0.282095f * A0 * color;

    // Band 1
    sh.c[1] = 0.488603f * n.y * A1 * color;
    sh.c[2] = 0.488603f * n.z * A1 * color;
    sh.c[3] = 0.488603f * n.x * A1 * color;

    return sh;
}

SH4Color ProjectOntoSH4(in float3 n, in float3 color)
{
    return ProjectOntoSH4(n, color, 1.0f, 1.0f);
}

//-------------------------------------------------------------------------------------------------
// Computes the dot project of two SH4 RGB vectors
//-------------------------------------------------------------------------------------------------
float3 SHDotProduct(in SH4Color a, in SH4Color b)
{
	float3 result = 0.0f;

	[unroll]
	for(uint i = 0; i < 4; ++i)
		result += a.c[i] * b.c[i];

	return result;
}

//-------------------------------------------------------------------------------------------------
// Projects a direction onto SH4 and dots it with another SH4 vector
//-------------------------------------------------------------------------------------------------
float3 EvalSH4(in float3 dir, in SH4Color sh)
{
	SH4Color dirSH = ProjectOntoSH4(dir, float3(1.0f, 1.0f, 1.0f));
	return SHDotProduct(dirSH, sh);
}

//-------------------------------------------------------------------------------------------------
// Projects a direction onto SH4, convolves with a cosine kernel, and dots it with another
// SH4 vector
//-------------------------------------------------------------------------------------------------
float3 EvalSH4Cosine(in float3 dir, in SH4Color sh)
{
	SH4Color dirSH = ProjectOntoSH4(dir, float3(1.0f, 1.0f, 1.0f), CosineA0, CosineA1);
	return SHDotProduct(dirSH, sh);
}

//-------------------------------------------------------------------------------------------------
// Converts from 3-band to 2-band SH
//-------------------------------------------------------------------------------------------------
SH4Color ConvertToSH4(in SH9Color sh9)
{
    SH4Color sh4;
    [unroll]
    for(uint i = 0; i < 4; ++i)
        sh4.c[i] = sh9.c[i];
    return sh4;
}

//-------------------------------------------------------------------------------------------------
// Computes the "optimal linear direction" for a set of SH coefficients
//-------------------------------------------------------------------------------------------------
float3 OptimalLinearDirection(in SH4Color sh)
{
    float x = dot(sh.c[3], 1.0f / 3.0f);
    float y = dot(sh.c[1], 1.0f / 3.0f);
    float z = dot(sh.c[2], 1.0f / 3.0f);
    return normalize(float3(x, y, z));
}

//-------------------------------------------------------------------------------------------------
// Computes the direction and color of a directional light that approximates a set of SH
// coefficients. Uses Peter Pike-Sloan's method from "Stupid SH Tricks"
//-------------------------------------------------------------------------------------------------
void ApproximateDirectionalLight(in SH4Color sh, out float3 direction, out float3 color)
{
    direction = OptimalLinearDirection(sh);
    SH4Color dirSH = ProjectOntoSH4(direction, 1.0f);
    dirSH.c[0] = 0.0f;
    sh.c[0] = 0.0f;
    color = SHDotProduct(dirSH, sh) * 867.0f / (316.0f * Pi);
}

// == SH9 =========================================================================================

//-------------------------------------------------------------------------------------------------
// Projects a direction onto SH and convolves with a given kernel
//-------------------------------------------------------------------------------------------------
SH9Color ProjectOntoSH9(in float3 n, in float3 color, in float A0, in float A1, in float A2)
{
    SH9Color sh;

    // Band 0
    sh.c[0] = 0.282095f * A0 * color;

    // Band 1
    sh.c[1] = 0.488603f * n.y * A1 * color;
    sh.c[2] = 0.488603f * n.z * A1 * color;
    sh.c[3] = 0.488603f * n.x * A1 * color;

    // Band 2
    sh.c[4] = 1.092548f * n.x * n.y * A2 * color;
    sh.c[5] = 1.092548f * n.y * n.z * A2 * color;
    sh.c[6] = 0.315392f * (3.0f * n.z * n.z - 1.0f) * A2 * color;
    sh.c[7] = 1.092548f * n.x * n.z * A2 * color;
    sh.c[8] = 0.546274f * (n.x * n.x - n.y * n.y) * A2 * color;

    return sh;
}

//-------------------------------------------------------------------------------------------------
// Projects a direction onto SH and convolves with a given kernel
//-------------------------------------------------------------------------------------------------
SH9Color ProjectOntoSH9(in float3 dir, in float3 color)
{
	return ProjectOntoSH9(dir, color, 1.0f, 1.0f, 1.0f);
}

//-------------------------------------------------------------------------------------------------
// Projects a direction onto H-basis
//-------------------------------------------------------------------------------------------------
H4 ProjectOntoH4(in float3 dir, in float value)
{
    H4 hBasis;

    // Band 0
    hBasis.x = value * (1.0f / sqrt(2.0f * 3.14159f));

    // Band 1
    hBasis.y = value * -sqrt(1.5f / 3.14159f) * dir.y;
    hBasis.z = value * sqrt(1.5f / 3.14159f) * (2 * dir.z - 1.0f);
    hBasis.w = value * -sqrt(1.5f / 3.14159f) * dir.x;

    return hBasis;
}

//-------------------------------------------------------------------------------------------------
// Computes the dot project of two SH9 RGB vectors
//-------------------------------------------------------------------------------------------------
float3 SHDotProduct(in SH9Color a, in SH9Color b)
{
	float3 result = 0.0f;

	[unroll]
	for(uint i = 0; i < 9; ++i)
		result += a.c[i] * b.c[i];

	return result;
}

//-------------------------------------------------------------------------------------------------
// Projects a direction onto SH9 and dots it with another SH9 vector
//-------------------------------------------------------------------------------------------------
float3 EvalSH9(in float3 dir, in SH9Color sh)
{
	SH9Color dirSH = ProjectOntoSH9(dir, float3(1.0f, 1.0f, 1.0f));
	return SHDotProduct(dirSH, sh);
}

//-------------------------------------------------------------------------------------------------
// Projects a direction onto SH9, convolves with a cosine kernel, and dots it with another
// SH9 vector
//-------------------------------------------------------------------------------------------------
float3 EvalSH9Cosine(in float3 dir, in SH9Color sh)
{
	SH9Color dirSH = ProjectOntoSH9(dir, float3(1.0f, 1.0f, 1.0f), CosineA0, CosineA1, CosineA2);
	return SHDotProduct(dirSH, sh);
}

//-------------------------------------------------------------------------------------------------
// Convolves 3-band SH coefficients with a cosine kernel
//-------------------------------------------------------------------------------------------------
SH9Color ConvolveWithCosineKernel(in SH9Color sh)
{
    sh.c[0] *= CosineA0;

    sh.c[1] *= CosineA1;
    sh.c[2] *= CosineA1;
    sh.c[3] *= CosineA1;

    sh.c[4] *= CosineA2;
    sh.c[5] *= CosineA2;
    sh.c[6] *= CosineA2;
    sh.c[7] *= CosineA2;
    sh.c[8] *= CosineA2;

    return sh;
}

//-------------------------------------------------------------------------------------------------
// Computes the "optimal linear direction" for a set of SH coefficients
//-------------------------------------------------------------------------------------------------
float3 OptimalLinearDirection(in SH9Color sh)
{
    SH4Color sh4 = ConvertToSH4(sh);
    return OptimalLinearDirection(sh4);
}

//-------------------------------------------------------------------------------------------------
// Computes the direction and color of a directional light that approximates a set of SH
// coefficients. Uses Peter Pike-Sloan's method from "Stupid SH Tricks"
//-------------------------------------------------------------------------------------------------
void ApproximateDirectionalLight(in SH9Color sh, out float3 direction, out float3 color)
{
    direction = OptimalLinearDirection(sh);
    SH9Color dirSH = ProjectOntoSH9(direction, 1.0f);
    dirSH.c[0] = 0.0f;
    sh.c[0] = 0.0f;
    color = SHDotProduct(dirSH, sh) * 867.0f / (316.0f * Pi);
}

// == H4 =========================================================================================

//-------------------------------------------------------------------------------------------------
// Converts from 3-band SH to 2-band H-Basis. See "Efficient Irradiance Normal Mapping" by
// Ralf Habel and Michael Wimmer for the derivations.
//-------------------------------------------------------------------------------------------------
H4Color ConvertToH4(in SH9Color sh)
{
	const float rt2 = sqrt(2.0f);
	const float rt32 = sqrt(3.0f / 2.0f);
	const float rt52 = sqrt(5.0f / 2.0f);
	const float rt152 = sqrt(15.0f / 2.0f);
	const float convMatrix[4][9] =
	{
		{ 1.0f / rt2, 0, 0.5f * rt32, 0, 0, 0, 0, 0, 0 },
		{ 0, 1.0f / rt2, 0, 0, 0, (3.0f / 8.0f) * rt52, 0, 0, 0 },
		{ 0, 0, 1.0f / (2.0f * rt2), 0, 0, 0, 0.25f * rt152, 0, 0 },
		{ 0, 0, 0, 1.0f / rt2, 0, 0, 0, (3.0f / 8.0f) * rt52, 0 }
	};

    H4Color hBasis;

	[unroll]
	for(uint row = 0; row < 4; ++row)
	{
		hBasis.c[row] = 0.0f;

		[unroll]
		for(uint col = 0; col < 9; ++col)
			hBasis.c[row] += convMatrix[row][col] * sh.c[col];
	}

    return hBasis;
}

//-------------------------------------------------------------------------------------------------
// Evalutes the 2-band H-Basis coefficients in the given direction
//-------------------------------------------------------------------------------------------------
float EvalH4(in float3 n, in H4 hBasis)
{
	float result = 0.0f;

    // Band 0
    result += hBasis.x * (1.0f / sqrt(2.0f * 3.14159f));

    // Band 1
    result += hBasis.y * sqrt(1.5f / 3.14159f) * n.y;
    result += hBasis.z * sqrt(1.5f / 3.14159f) * (2 * n.z - 1.0f);
    result += hBasis.w * sqrt(1.5f / 3.14159f) * n.x;

	return result;
}

//-------------------------------------------------------------------------------------------------
// Evalutes the 2-band H-Basis coefficients in the given direction
//-------------------------------------------------------------------------------------------------
float3 EvalH4(in float3 n, in H4Color hBasis)
{
	float3 color = 0.0f;

    // Band 0
    color += hBasis.c[0] * (1.0f / sqrt(2.0f * 3.14159f));

    // Band 1
    color += hBasis.c[1] * sqrt(1.5f / 3.14159f) * n.y;
    color += hBasis.c[2] * sqrt(1.5f / 3.14159f) * (2 * n.z - 1.0f);
    color += hBasis.c[3] * sqrt(1.5f / 3.14159f) * n.x;

	return color;
}

//-------------------------------------------------------------------------------------------------
// Evalutes the 2-band H-Basis coefficients in the given direction
//-------------------------------------------------------------------------------------------------
float3 EvalH4(in float3 n, in float3 H0, in float3 H1, in float3 H2, in float3 H3)
{
    H4Color h4Clr;
    h4Clr.c[0] = H0;
    h4Clr.c[1] = H1;
    h4Clr.c[2] = H2;
    h4Clr.c[3] = H3;
    return EvalH4(n, h4Clr);
}

// == H6 =========================================================================================

//-------------------------------------------------------------------------------------------------
// Converts from 3-band SH to 3-band H-Basis. See "Efficient Irradiance Normal Mapping" by
// Ralf Habel and Michael Wimmer for the derivations.
//-------------------------------------------------------------------------------------------------
H6Color ConvertToH6(in SH9Color sh)
{
	const float rt2 = sqrt(2.0f);
	const float rt32 = sqrt(3.0f / 2.0f);
	const float rt52 = sqrt(5.0f / 2.0f);
	const float rt152 = sqrt(15.0f / 2.0f);
	const float convMatrix[6][9] =
	{
		{ 1.0f / rt2, 0, 0.5f * rt32, 0, 0, 0, 0, 0, 0 },
		{ 0, 1.0f / rt2, 0, 0, 0, (3.0f / 8.0f) * rt52, 0, 0, 0 },
		{ 0, 0, 1.0f / (2.0f * rt2), 0, 0, 0, 0.25f * rt152, 0, 0 },
		{ 0, 0, 0, 1.0f / rt2, 0, 0, 0, (3.0f / 8.0f) * rt52, 0 },
        { 0, 0, 0, 0, 1.0f / rt2, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 1.0f / rt2 }
	};

    H6Color hBasis;

	[unroll]
	for(uint row = 0; row < 6; ++row)
	{
		hBasis.c[row] = 0.0f;

		[unroll]
		for(uint col = 0; col < 9; ++col)
			hBasis.c[row] += convMatrix[row][col] * sh.c[col];
	}

    return hBasis;
}

//-------------------------------------------------------------------------------------------------
// Evalutes the 3-band H-Basis coefficients in the given direction
//-------------------------------------------------------------------------------------------------
float3 EvalH6(in float3 n, in H6Color hBasis)
{
	float3 color = 0.0f;

    // Band 0
    color += hBasis.c[0] * (1.0f / sqrt(2.0f * 3.14159f));

    // Band 1
    color += hBasis.c[1] * sqrt(1.5f / 3.14159f) * n.y;
    color += hBasis.c[2] * sqrt(1.5f / 3.14159f) * (2 * n.z - 1.0f);
    color += hBasis.c[3] * sqrt(1.5f / 3.14159f) * n.x;

    // Band 3
    color += hBasis.c[4] * 0.5f * sqrt(7.5f / 3.14159f) * n.x * n.y;
    color += hBasis.c[5] * 0.5f * sqrt(7.5f / 3.14159f) * (n.x * n.x - n.y * n.y);

	return color;
}
