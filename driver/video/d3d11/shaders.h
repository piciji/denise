
static const std::string D3D11outputShader = R"(
    uniform sampler s0;
    uniform Texture2D <float4> t0;

    struct UBO {
        float4x4 projection;
    };
    uniform UBO ubo;

    struct VSInput {
        float2 pos : POSITION;
        float4 col : COLOR;
        float2 tex : TEXCOORD0;
    };

    struct PSInput {
        float4 pos : SV_POSITION;
        float4 col : COLOR;
        float2 tex : TEXCOORD0;
    };

    PSInput VS(VSInput input) {
        PSInput output;
        output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
        output.col = input.col;
        output.tex = input.tex;
        return output;
    }

    float4 PS(PSInput input) : SV_TARGET {
        return input.col * t0.Sample(s0, input.tex);
    };
)";

static const std::string D3D11messageShader = R"(
    uniform sampler s0;
    uniform Texture2D <float4> t0;

    struct VSInput {
        float2 pos : POSITION;
        float4 col : COLOR;
        float2 tex : TEXCOORD0;
    };

    struct PSInput {
        float4 pos : SV_POSITION;
        float4 col : COLOR;
        float2 tex : TEXCOORD0;
    };

    PSInput VS(VSInput input) {
        PSInput output;
        output.pos = float4(input.pos.xy, 0.f, 1.f);
        output.col = input.col;
        output.tex = input.tex;
        return output;
    }

    float4 PS(PSInput input) : SV_TARGET {
         return float4(input.col.rgb, input.col.a * t0.Sample(s0, input.tex).a);
    };
)";

static const std::string D3D11overlayShader = R"(
    uniform sampler s0;
    uniform Texture2D <float4> t0;

    struct VSInput {
        float2 pos : POSITION;
        float4 col : COLOR;
        float2 tex : TEXCOORD0;
    };

    struct PSInput {
        float4 pos : SV_POSITION;
        float4 col : COLOR;
        float2 tex : TEXCOORD0;
    };

    PSInput VS(VSInput input) {
        PSInput output;
        output.pos = float4(input.pos.xy, 0.f, 1.f);
        output.col = input.col;
        output.tex = input.tex;
        return output;
    }

    float4 PS(PSInput input) : SV_TARGET {
         return input.col * t0.Sample(s0, input.tex);
    };
)";