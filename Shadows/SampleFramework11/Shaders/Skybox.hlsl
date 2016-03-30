//=================================================================================================
//
//  MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

//=================================================================================================
// Constant buffers
//=================================================================================================

cbuffer VSConstants : register (b0)
{
    float4x4 View : packoffset(c0);
    float4x4 Projection : packoffset(c4);
    float3 Bias : packoffset(c8);
}

cbuffer GSConstants : register(b0)
{
  uint RTIndex : packoffset(c0);
}

cbuffer PSConstants : register (b0)
{
    float3 SunDirection : packoffset(c0);
  uint EnableSun : packoffset(c1);
}

//=================================================================================================
// Samplers
//=================================================================================================

TextureCube  EnvironmentMap : register(t0);
SamplerState LinearSampler : register(s0);


//=================================================================================================
// Input/Output structs
//=================================================================================================

struct VSInput
{
    float3 PositionOS : POSITION;
};

struct VSOutput
{
    float4 PositionCS   : SV_Position;
    float3 TexCoord   : TEXCOORD;
    float3 Bias     : BIAS;
};

struct GSOutput
{
  VSOutput VertexShaderOutput;

  uint RTIndex     : SV_RenderTargetArrayIndex;
  uint VPIndex     : SV_ViewportArrayIndex;
};

//=================================================================================================
// Vertex Shader
//=================================================================================================
VSOutput SkyboxVS(in VSInput input)
{
    VSOutput output;

    // Rotate into view-space, centered on the camera
    float3 positionVS = mul(input.PositionOS.xyz, (float3x3)View);

    // Transform to clip-space
    output.PositionCS = mul(float4(positionVS, 1.0f), Projection);

    // Make a texture coordinate
    output.TexCoord = input.PositionOS;

    // Pass along the bias
    output.Bias = Bias;

    return output;
}

//=================================================================================================
// Geometry Shader
//=================================================================================================
[maxvertexcount(3)]
void SkyboxGS(triangle VSOutput Input[3], inout TriangleStream<GSOutput> TriStream)
{
  [unroll]
  for(uint i = 0; i < 3; ++i)
  {
    GSOutput output;
    output.VertexShaderOutput = Input[i];
    output.RTIndex = RTIndex;
    output.VPIndex = RTIndex;

    TriStream.Append(output);
  }

  TriStream.RestartStrip();
}

//=================================================================================================
// Pixel Shader
//=================================================================================================
float4 SkyboxPS(in VSOutput input) : SV_Target
{
    // Sample the skybox
    float3 texColor = EnvironmentMap.Sample(LinearSampler, normalize(input.TexCoord)).rgb;

    // Apply the bias
    texColor = texColor * input.Bias;

    return float4(texColor, 1.0f);
}

//-------------------------------------------------------------------------------------------------
// Calculates the angle between two vectors
//-------------------------------------------------------------------------------------------------
float AngleBetween(in float3 dir0, in float3 dir1)
{
  return acos(dot(dir0, dir1));
}

//-------------------------------------------------------------------------------------------------
// Uses the CIE Clear Sky model to compute a color for a pixel, given a direction + sun direction
//-------------------------------------------------------------------------------------------------
float3 CIEClearSky(in float3 dir, in float3 sunDir)
{
  float3 skyDir = float3(dir.x, abs(dir.y), dir.z);
  float gamma = AngleBetween(skyDir, sunDir);
  float S = AngleBetween(sunDir, float3(0, 1, 0));
  float theta = AngleBetween(skyDir, float3(0, 1, 0));

  float cosTheta = cos(theta);
  float cosS = cos(S);
  float cosGamma = cos(gamma);

  float num = (0.91f + 10 * exp(-3 * gamma) + 0.45 * cosGamma * cosGamma) * (1 - exp(-0.32f / cosTheta));
  float denom = (0.91f + 10 * exp(-3 * S) + 0.45 * cosS * cosS) * (1 - exp(-0.32f));

  float lum = num / max(denom, 0.0001f);

  // Clear Sky model only calculates luminance, so we'll pick a strong blue color for the sky
  const float3 SkyColor = float3(0.2f, 0.5f, 1.0f) * 1;
  const float3 SunColor = float3(1.0f, 0.8f, 0.3f) * 1500;
  const float SunWidth = 0.015f;

  float3 color = SkyColor;

  // Draw a circle for the sun
  [flatten]
  if (EnableSun)
  {
    float sunGamma = AngleBetween(dir, sunDir);
    color = lerp(SunColor, SkyColor, saturate(abs(sunGamma) / SunWidth));
  }

  return max(color * lum, 0);
}

//=================================================================================================
// Sky Model Pixel Shader
//=================================================================================================
float4 SkyModelPS(in VSOutput input) : SV_Target
{
  float3 dir = normalize(input.TexCoord);

  return float4(CIEClearSky(dir, SunDirection) * input.Bias, 1.0f);
}
