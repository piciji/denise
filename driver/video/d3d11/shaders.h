
static const std::string D3D11outputShader = R"(
    uniform sampler s0;
    uniform Texture2D <float4> t0;

    struct UBO {
        float4x4 projection;
    };
    uniform UBO ubo;

    struct VSInput {
        float2 pos : POSITION;
        float2 tex : TEXCOORD0;
    };

    struct PSInput {
        float4 pos : SV_POSITION;
        float2 tex : TEXCOORD0;
    };

    PSInput VS(VSInput input) {
        PSInput output;
        output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
        output.tex = input.tex;
        return output;
    }

    float4 PS(PSInput input) : SV_TARGET {
        return t0.Sample(s0, input.tex);
    };
)";

static const std::string D3D11messageShader = R"(
    uniform sampler s0;
    uniform Texture2D <float4> t0;

    cbuffer scene {
        float4 fragColor : packoffset(c0);
        float4 fragBgColor : packoffset(c1);
    };

    struct VSInput {
        float2 pos : POSITION;
        float2 tex : TEXCOORD0;
    };

    struct PSInput {
        float4 pos : SV_POSITION;
        float2 tex : TEXCOORD0;
    };

    PSInput VS(VSInput input) {
        PSInput output;
        output.pos = float4(input.pos.xy, 0.f, 1.f);
        output.tex = input.tex;
        return output;
    }

    float4 PS(PSInput input) : SV_TARGET {
        if(fragBgColor.a == 0.0)
            return float4(1.0, 1.0, 1.0, t0.Sample(s0, input.tex).a) * fragColor;

        return float4( lerp(fragBgColor.rgb, fragColor.rgb, t0.Sample(s0, input.tex).a), fragBgColor.a);
    };
)";

static const std::string D3D11overlayShader = R"(
    uniform sampler s0;
    uniform Texture2D <float4> t0;

    struct VSInput {
        float2 pos : POSITION;
        float2 tex : TEXCOORD0;
    };

    struct PSInput {
        float4 pos : SV_POSITION;
        float2 tex : TEXCOORD0;
    };

    PSInput VS(VSInput input) {
        PSInput output;
        output.pos = float4(input.pos.xy, 0.f, 1.f);
        output.tex = input.tex;
        return output;
    }

    float4 PS(PSInput input) : SV_TARGET {
         return t0.Sample(s0, input.tex);
    };
)";

static const std::string D3D11progressShader = R"(
    #define PI 3.14159265358979323846
    uniform sampler s0;
    uniform Texture2D <float4> t0;
    uniform int degree;

    struct VSInput {
        float2 pos : POSITION;
        float2 tex : TEXCOORD0;
    };

    struct PSInput {
        float4 pos : SV_POSITION;
        float2 tex : TEXCOORD0;
    };

    float2 rotateUV(float2 uv, float2 pivot, float rotation) {
        float2x2 rotation_matrix=float2x2(
            float2(sin(rotation), -cos(rotation)),
            float2(cos(rotation), sin(rotation))
        );

        uv -= pivot;
        uv = mul(rotation_matrix,uv);
        uv += pivot;

        return uv;
    }

    PSInput VS(VSInput input) {
        PSInput output;
        output.pos = float4(input.pos.xy, 0.f, 1.f);
        output.tex = input.tex;
        return output;
    }

    float4 PS(PSInput input) : SV_TARGET {
        return t0.Sample(s0, rotateUV(input.tex, float2(0.5, 0.5), (float)degree * (PI / 180.0) ) );
    };
)";

// Copyright (c) RetroArch (licenced under GPLv3)
static const std::string D3D11hdrShader = R"(
    uniform sampler s0;
    uniform Texture2D <float4> t0;

    struct UBO {
        float4x4 projection;
        float contrast;
        float paper_white_nits;
        float max_nits;
        float expand_gamut;
        float inverse_tonemap;
        float hdr10;
    };
    uniform UBO ubo;

    struct VSInput {
        float2 pos : POSITION;
        float2 tex : TEXCOORD0;
    };

    struct PSInput {
        float4 pos : SV_POSITION;
        float2 tex : TEXCOORD0;
    };

    PSInput VS(VSInput input) {
        PSInput output;
        output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
        output.tex = input.tex;
        return output;
    }

    static const float kMaxNitsFor2084     = 10000.0f;  
    static const float kEpsilon            = 0.0001f;
    static const float kLumaChannelRatio   = 0.25f;

   static const float3x3 k709to2020 =
   {
      { 0.6274040f, 0.3292820f, 0.0433136f },
      { 0.0690970f, 0.9195400f, 0.0113612f },
      { 0.0163916f, 0.0880132f, 0.8955950f }
   };

   static const float3x3 kP3to2020 =
   {
      { 0.753845f, 0.198593f, 0.047562f },
      { 0.0457456f, 0.941777f, 0.0124772f },
      { -0.00121055f, 0.0176041f, 0.983607f }
   };

   /* START Converted from (Copyright (c) Microsoft Corporation - Licensed under the MIT License.)  https://github.com/microsoft/Xbox-ATG-Samples/tree/master/Kits/ATGTK/HDR */
   static const float3x3 kExpanded709to2020 =
   {
      { 0.6274040f, 0.3292820f, 0.0433136f },
      { 0.0457456, 0.941777, 0.0124772 },
      { -0.00121055, 0.0176041, 0.983607 }
   };

   float3 LinearToST2084(float3 normalizedLinearValue)
   {
      float3 ST2084 = pow((0.8359375f + 18.8515625f * pow(abs(normalizedLinearValue), 0.1593017578f)) / (1.0f + 18.6875f * pow(abs(normalizedLinearValue), 0.1593017578f)), 78.84375f);
      return ST2084;  /* Don't clamp between [0..1], so we can still perform operations on scene values higher than 10,000 nits */
   }
   /* END Converted from (Copyright (c) Microsoft Corporation - Licensed under the MIT License.)  https://github.com/microsoft/Xbox-ATG-Samples/tree/master/Kits/ATGTK/HDR */

   float3 SRGBToLinear(float3 color)
   {
      float3 scale = color / 12.92f;
      float3 gamma = pow(abs(color + 0.055f) / 1.055f, 2.4f);

      return float3( color.x < 0.04045f ? scale.x : gamma.x, 
                     color.y < 0.04045f ? scale.y : gamma.y,
                     color.z < 0.04045f ? scale.z : gamma.z);
   }

   float3 Hdr(float3 sdr)
   {
      float3 hdr;
      
      if(ubo.inverse_tonemap)
      {
         sdr = pow(abs(sdr), ubo.contrast / 2.2f );       /* Display Gamma - needs to be determined by calibration screen */

         float luma = dot(sdr, float3(0.2126, 0.7152, 0.0722));  /* Rec BT.709 luma coefficients - https://en.wikipedia.org/wiki/Luma_(video) */

         /* Inverse reinhard tonemap */
         float maxValue             = (ubo.max_nits / ubo.paper_white_nits) + kEpsilon;
         float elbow                = maxValue / (maxValue - 1.0f);                          
         float offset               = 1.0f - ((0.5f * elbow) / (elbow - 0.5f));              
         
         float hdrLumaInvTonemap    = offset + ((luma * elbow) / (elbow - luma));
         float sdrLumaInvTonemap    = luma / ((1.0f + kEpsilon) - luma);                     /* Convert the srd < 0.5 to 0.0 -> 1.0 range */

         float lumaInvTonemap       = (luma > 0.5f) ? hdrLumaInvTonemap : sdrLumaInvTonemap;
         float3 perLuma             = sdr / (luma + kEpsilon) * lumaInvTonemap;

         float3 hdrInvTonemap       = offset + ((sdr * elbow) / (elbow - sdr));         
         float3 sdrInvTonemap       = sdr / ((1.0f + kEpsilon) - sdr);               /* Convert the srd < 0.5 to 0.0 -> 1.0 range */

         float3 perChannel          = float3(sdr.x > 0.5f ? hdrInvTonemap.x : sdrInvTonemap.x,
                                             sdr.y > 0.5f ? hdrInvTonemap.y : sdrInvTonemap.y,
                                             sdr.z > 0.5f ? hdrInvTonemap.z : sdrInvTonemap.z);

         hdr = lerp(perLuma, perChannel, kLumaChannelRatio);
      }
      else
      {
         hdr = sdr;
      }

      float3 hdr10;

      if(ubo.hdr10)
      {
         /* Now convert into HDR10 */
         float3 rec2020 = mul(k709to2020, hdr);

         if(ubo.expand_gamut > 0.0f)
         {
            rec2020 = mul( kExpanded709to2020, hdr);
         }

         float3 linearColour  = rec2020 * (ubo.paper_white_nits / kMaxNitsFor2084);
         
         hdr10 = LinearToST2084(linearColour);
      }
      else
      {
         hdr10 = hdr;
      }

      return hdr10;
   }

    float4 PS(PSInput input) : SV_TARGET {
        float4 sdr = t0.Sample(s0, input.tex);

        return float4(Hdr(sdr.rgb), sdr.a);
    };
)";
