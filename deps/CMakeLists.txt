
set(_depSrcs
        deps/glslang/SPIRV/GlslangToSpv.cpp
        deps/glslang/SPIRV/InReadableOrder.cpp
        deps/glslang/SPIRV/Logger.cpp
        deps/glslang/SPIRV/SpvBuilder.cpp
        deps/glslang/SPIRV/SpvPostProcess.cpp
        deps/glslang/glslang/GenericCodeGen/CodeGen.cpp
        deps/glslang/glslang/GenericCodeGen/Link.cpp
        deps/glslang/glslang/MachineIndependent/attribute.cpp
        deps/glslang/glslang/MachineIndependent/Constant.cpp
        deps/glslang/glslang/MachineIndependent/glslang_tab.cpp
        deps/glslang/glslang/MachineIndependent/InfoSink.cpp
        deps/glslang/glslang/MachineIndependent/Initialize.cpp
        deps/glslang/glslang/MachineIndependent/Intermediate.cpp
        deps/glslang/glslang/MachineIndependent/intermOut.cpp
        deps/glslang/glslang/MachineIndependent/IntermTraverse.cpp
        deps/glslang/glslang/MachineIndependent/iomapper.cpp
        deps/glslang/glslang/MachineIndependent/limits.cpp
        deps/glslang/glslang/MachineIndependent/linkValidate.cpp
        deps/glslang/glslang/MachineIndependent/parseConst.cpp
        deps/glslang/glslang/MachineIndependent/ParseContextBase.cpp
        deps/glslang/glslang/MachineIndependent/ParseHelper.cpp
        deps/glslang/glslang/MachineIndependent/PoolAlloc.cpp
        deps/glslang/glslang/MachineIndependent/propagateNoContraction.cpp
        deps/glslang/glslang/MachineIndependent/reflection.cpp
        deps/glslang/glslang/MachineIndependent/RemoveTree.cpp
        deps/glslang/glslang/MachineIndependent/Scan.cpp
        deps/glslang/glslang/MachineIndependent/ShaderLang.cpp
        deps/glslang/glslang/MachineIndependent/SymbolTable.cpp
        deps/glslang/glslang/MachineIndependent/Versions.cpp
        deps/glslang/glslang/MachineIndependent/SpirvIntrinsics.cpp
        deps/glslang/glslang/ResourceLimits/ResourceLimits.cpp
        deps/glslang/glslang/MachineIndependent/preprocessor/Pp.cpp
        deps/glslang/glslang/MachineIndependent/preprocessor/PpAtom.cpp
        deps/glslang/glslang/MachineIndependent/preprocessor/PpContext.cpp
        deps/glslang/glslang/MachineIndependent/preprocessor/PpScanner.cpp
        deps/glslang/glslang/MachineIndependent/preprocessor/PpTokens.cpp

        deps/SPIRV-Cross/spirv_cross.cpp
        deps/SPIRV-Cross/spirv_cfg.cpp
        deps/SPIRV-Cross/spirv_glsl.cpp
        deps/SPIRV-Cross/spirv_parser.cpp
        deps/SPIRV-Cross/spirv_cross_parsed_ir.cpp

        deps/lha/lha_reader.cpp
        deps/lha/lha_basic_reader.cpp
        deps/lha/lha_input_stream.cpp
        deps/lha/lha_file_header.cpp
        deps/lha/lha_decoder.cpp
        deps/lha/macbinary.cpp
        deps/lha/ext_header.cpp
        deps/lha/lha_endian.cpp
        deps/lha/null_decoder.cpp
        deps/lha/lh1_decoder.cpp
        deps/lha/lh5_decoder.cpp
        deps/lha/lh6_decoder.cpp
        deps/lha/lh7_decoder.cpp
        deps/lha/pm1_decoder.cpp
        deps/lha/pm2_decoder.cpp
        deps/lha/lz5_decoder.cpp
        deps/lha/lzs_decoder.cpp
        deps/lha/lhx_decoder.cpp
        deps/lha/lk7_decoder.cpp
        deps/lha/crc16.cpp

        deps/USBSID-Pico/USBSID.cpp
        deps/USBSID-Pico/USBSIDInterface.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(_depSrcsHLSL
        deps/glslang/glslang/HLSL/hlslAttributes.cpp
        deps/glslang/glslang/HLSL/hlslGrammar.cpp
        deps/glslang/glslang/HLSL/hlslOpMap.cpp
        deps/glslang/glslang/HLSL/hlslParseables.cpp
        deps/glslang/glslang/HLSL/hlslParseHelper.cpp
        deps/glslang/glslang/HLSL/hlslScanContext.cpp
        deps/glslang/glslang/HLSL/hlslTokenStream.cpp
        deps/SPIRV-Cross/spirv_hlsl.cpp
    )
    set(_depSrcsMSL )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(_depSrcsMSL
        deps/SPIRV-Cross/spirv_msl.cpp
    )
    set(_depSrcsHLSL )
else()
    set(_depSrcsHLSL )
    set(_depSrcsMSL )
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(_depSrcs ${_depSrcs} deps/lha/lha_arch_win32.cpp )
else()
    set(_depSrcs ${_depSrcs} deps/lha/lha_arch_unix.cpp )
endif()

set(depSrcs ${_depSrcs} ${_depSrcsHLSL} ${_depSrcsMSL} PARENT_SCOPE)
