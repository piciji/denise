
#include "SPIRV-Cross/spirv_cross.cpp"
#include "SPIRV-Cross/spirv_cfg.cpp"
#include "SPIRV-Cross/spirv_glsl.cpp"
#include "SPIRV-Cross/spirv_parser.cpp"
#include "SPIRV-Cross/spirv_cross_parsed_ir.cpp"

#ifdef __APPLE__
#include "SPIRV-Cross/spirv_msl.cpp"
#endif

#ifdef _WIN32
#include "SPIRV-Cross/spirv_hlsl.cpp"
#endif


#ifdef _WIN32
#include "glslang/glslang/HLSL/hlslAttributes.cpp"
#include "glslang/glslang/HLSL/hlslGrammar.cpp"
#include "glslang/glslang/HLSL/hlslOpMap.cpp"
#include "glslang/glslang/HLSL/hlslParseables.cpp"
#include "glslang/glslang/HLSL/hlslParseHelper.cpp"
#include "glslang/glslang/HLSL/hlslScanContext.cpp"
#include "glslang/glslang/HLSL/hlslTokenStream.cpp"
#endif