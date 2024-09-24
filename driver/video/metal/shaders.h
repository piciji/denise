
static const std::string MTLOutputShader = R"(
    #include <metal_stdlib>
    #include <simd/simd.h>

    struct VSInput {
        vector_float2 position [[attribute(0)]];
        vector_float2 texCoord [[attribute(1)]];
    };

    struct PSInput {
        vector_float4 position [[position]];
        vector_float2 texCoord;
    };

    struct UBO {
        matrix_float4x4 projectionMatrix;
    };

    using namespace metal;

    vertex PSInput _vertex(const VSInput input [[ stage_in ]],
                           const device UBO &ubo [[ buffer(1) ]]) {
        PSInput output;
        output.position = ubo.projectionMatrix * float4(input.position, 0.0, 1.0);
        output.texCoord = input.texCoord;
        return output;
    }

    fragment float4 _fragment(PSInput input [[stage_in]],
                                texture2d<half> tex [[ texture(0) ]],
                                sampler s0 [[ sampler(0) ]]) {

        half4 col = tex.sample(s0, input.texCoord.xy);
        return float4(col);
    }
)";

static const std::string MTLMessageShader = R"(
    #include <metal_stdlib>
    #include <simd/simd.h>

    struct VSInput {
        vector_float2 position [[attribute(0)]];
        vector_float2 texCoord [[attribute(1)]];
    };

    struct PSInput {
        vector_float4 position [[position]];
        vector_float2 texCoord;
    };

    struct Push {
        vector_float4 col;
        vector_float4 colBg;
    };

    using namespace metal;

    vertex PSInput _vertex(const VSInput input [[ stage_in ]]) {
        PSInput output;
        output.position = float4(input.position, 0.0, 1.0);
        output.texCoord = input.texCoord;
        return output;
    }

    fragment float4 _fragment(PSInput input [[stage_in]],
                                texture2d<half> tex [[ texture(0) ]],
                                sampler s0 [[ sampler(0) ]],
                                constant Push& params [[ buffer(1) ]]) {

        float _pix = tex.sample(s0, input.texCoord.xy).a;
        
        if(params.colBg.a == 0.0)
            return float4(1.0, 1.0, 1.0, _pix) * params.col;

        return float4( mix(params.colBg.rgb, params.col.rgb, float3(_pix)), params.colBg.a);
    }
)";

static const std::string MTLDndOverlayShader = R"(
    #include <metal_stdlib>
    #include <simd/simd.h>

    struct VSInput {
        vector_float2 position [[attribute(0)]];
        vector_float2 texCoord [[attribute(1)]];
    };

    struct PSInput {
        vector_float4 position [[position]];
        vector_float2 texCoord;
    };

    using namespace metal;

    vertex PSInput _vertex(const VSInput input [[ stage_in ]]) {
        PSInput output;
        output.position = float4(input.position, 0.0, 1.0);
        output.texCoord = input.texCoord;
        return output;
    }

    fragment float4 _fragment(PSInput input [[stage_in]],
                                texture2d<half> tex [[ texture(0) ]],
                                sampler s0 [[ sampler(0) ]]) {

        half4 col = tex.sample(s0, input.texCoord.xy);
        return float4(col);
    }
)";

static const std::string MTLprogressShader = R"(
    #include <metal_stdlib>
    #include <simd/simd.h>

    struct VSInput {
        vector_float2 position [[attribute(0)]];
        vector_float2 texCoord [[attribute(1)]];
    };

    struct PSInput {
        vector_float4 position [[position]];
        vector_float2 texCoord;
        float degree;
    };

    struct UBO {
        int degree;
    };

    constant float PI = 3.1415926535897932384626433832795;

    using namespace metal;

    vector_float2 rotateUV(vector_float2 uv, vector_float2 pivot, float rotation) {
        matrix_float2x2 rotation_matrix=float2x2(
            vector_float2(sin(rotation), -cos(rotation)),
            vector_float2(cos(rotation), sin(rotation))
        );

        uv -= pivot;
        uv = uv * rotation_matrix;
        uv += pivot;

        return uv;
    }

    vertex PSInput _vertex(const VSInput input [[ stage_in ]],
                           const device UBO &ubo [[ buffer(1) ]]) {
        PSInput output;
        output.position = float4(input.position, 0.0, 1.0);
        output.texCoord = input.texCoord;
        output.degree = ubo.degree;
        return output;
    }

    fragment float4 _fragment(PSInput input [[stage_in]],
                                texture2d<half> tex [[ texture(0) ]],
                                sampler s0 [[ sampler(0) ]]) {

        half4 col =  tex.sample(s0, rotateUV(input.texCoord, float2(0.5, 0.5), input.degree * (PI / 180.0) ) );

        return float4(col);
    }
)";
