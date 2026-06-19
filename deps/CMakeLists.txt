
# LHA
add_library(lha STATIC)

set(srcs
        lha/lha_reader.cpp
        lha/lha_basic_reader.cpp
        lha/lha_input_stream.cpp
        lha/lha_file_header.cpp
        lha/lha_decoder.cpp
        lha/macbinary.cpp
        lha/ext_header.cpp
        lha/lha_endian.cpp
        lha/null_decoder.cpp
        lha/lh1_decoder.cpp
        lha/lh5_decoder.cpp
        lha/lh6_decoder.cpp
        lha/lh7_decoder.cpp
        lha/pm1_decoder.cpp
        lha/pm2_decoder.cpp
        lha/lz5_decoder.cpp
        lha/lzs_decoder.cpp
        lha/lhx_decoder.cpp
        lha/lk7_decoder.cpp
        lha/crc16.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(srcs ${srcs} lha/lha_arch_win32.cpp )
else()
    set(srcs ${srcs} lha/lha_arch_unix.cpp )
endif()

target_sources(lha PRIVATE ${srcs})

# GIF
add_library(gif STATIC)

target_sources(gif PRIVATE
        cgif/cgif.cpp
        cgif/cgif_raw.cpp
)

# PICO
if(LIBUSB_FOUND)
    add_library(pico STATIC)

    set(srcs USBSID-Pico/USBSID.cpp)

    SET_SOURCE_FILES_PROPERTIES(${srcs}
            PROPERTIES INCLUDE_DIRECTORIES "${LIBUSB_INCLUDE_DIRS};")

    #if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        #SET_SOURCE_FILES_PROPERTIES(${srcs} PROPERTIES COMPILE_FLAGS "-DHAVE_ALIGNED_ALLOC")
    #elseif(${CMAKE_OSX_ARCHITECTURES} STREQUAL "arm64")
        #SET_SOURCE_FILES_PROPERTIES(${srcs} PROPERTIES COMPILE_FLAGS "-DHAVE_ALIGNED_ALLOC")
    #endif()

    target_sources(pico PRIVATE ${srcs})

    target_link_directories(pico PRIVATE ${LIBUSB_LIBRARY_DIRS})
    target_link_libraries(pico PRIVATE ${LIBUSB_LIBRARIES})
endif()

# glslang
add_library(glslang STATIC)

set(srcs
        glslang/SPIRV/GlslangToSpv.cpp
        glslang/SPIRV/InReadableOrder.cpp
        glslang/SPIRV/Logger.cpp
        glslang/SPIRV/SpvBuilder.cpp
        glslang/SPIRV/SpvPostProcess.cpp
        glslang/glslang/GenericCodeGen/CodeGen.cpp
        glslang/glslang/GenericCodeGen/Link.cpp
        glslang/glslang/MachineIndependent/attribute.cpp
        glslang/glslang/MachineIndependent/Constant.cpp
        glslang/glslang/MachineIndependent/glslang_tab.cpp
        glslang/glslang/MachineIndependent/InfoSink.cpp
        glslang/glslang/MachineIndependent/Initialize.cpp
        glslang/glslang/MachineIndependent/Intermediate.cpp
        glslang/glslang/MachineIndependent/intermOut.cpp
        glslang/glslang/MachineIndependent/IntermTraverse.cpp
        glslang/glslang/MachineIndependent/iomapper.cpp
        glslang/glslang/MachineIndependent/limits.cpp
        glslang/glslang/MachineIndependent/linkValidate.cpp
        glslang/glslang/MachineIndependent/parseConst.cpp
        glslang/glslang/MachineIndependent/ParseContextBase.cpp
        glslang/glslang/MachineIndependent/ParseHelper.cpp
        glslang/glslang/MachineIndependent/PoolAlloc.cpp
        glslang/glslang/MachineIndependent/propagateNoContraction.cpp
        glslang/glslang/MachineIndependent/reflection.cpp
        glslang/glslang/MachineIndependent/RemoveTree.cpp
        glslang/glslang/MachineIndependent/Scan.cpp
        glslang/glslang/MachineIndependent/ShaderLang.cpp
        glslang/glslang/MachineIndependent/SymbolTable.cpp
        glslang/glslang/MachineIndependent/Versions.cpp
        glslang/glslang/MachineIndependent/SpirvIntrinsics.cpp
        glslang/glslang/ResourceLimits/ResourceLimits.cpp
        glslang/glslang/MachineIndependent/preprocessor/Pp.cpp
        glslang/glslang/MachineIndependent/preprocessor/PpAtom.cpp
        glslang/glslang/MachineIndependent/preprocessor/PpContext.cpp
        glslang/glslang/MachineIndependent/preprocessor/PpScanner.cpp
        glslang/glslang/MachineIndependent/preprocessor/PpTokens.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(srcs ${srcs}
        glslang/glslang/HLSL/hlslAttributes.cpp
        glslang/glslang/HLSL/hlslGrammar.cpp
        glslang/glslang/HLSL/hlslOpMap.cpp
        glslang/glslang/HLSL/hlslParseables.cpp
        glslang/glslang/HLSL/hlslParseHelper.cpp
        glslang/glslang/HLSL/hlslScanContext.cpp
        glslang/glslang/HLSL/hlslTokenStream.cpp
    )
    SET_SOURCE_FILES_PROPERTIES(${srcs} PROPERTIES COMPILE_FLAGS "-DENABLE_HLSL")
endif()

SET_SOURCE_FILES_PROPERTIES(${srcs}
        PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/glslang;")

target_sources(glslang PRIVATE ${srcs})

# spirv
add_library(spirv STATIC)

set(srcs
        SPIRV-Cross/spirv_cross.cpp
        SPIRV-Cross/spirv_cfg.cpp
        SPIRV-Cross/spirv_glsl.cpp
        SPIRV-Cross/spirv_parser.cpp
        SPIRV-Cross/spirv_cross_parsed_ir.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(srcs ${srcs} SPIRV-Cross/spirv_hlsl.cpp)

elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(srcs ${srcs} SPIRV-Cross/spirv_msl.cpp)
endif()

target_sources(spirv PRIVATE ${srcs})

# libresidfp
add_library(libresidfp STATIC)

set(srcs
        libresidfp/src/Dac.cpp
        libresidfp/src/EnvelopeGenerator.cpp
        libresidfp/src/ExternalFilter.cpp
        libresidfp/src/Filter.cpp
        libresidfp/src/Filter6581.cpp
        libresidfp/src/Filter8580.cpp
        libresidfp/src/FilterModelConfig.cpp
        libresidfp/src/FilterModelConfig6581.cpp
        libresidfp/src/FilterModelConfig8580.cpp
        libresidfp/src/Integrator6581.cpp
        libresidfp/src/Integrator8580.cpp
        libresidfp/src/OpAmp.cpp
        libresidfp/src/SID.cpp
        libresidfp/src/Spline.cpp
        libresidfp/src/WaveformCalculator.cpp
        libresidfp/src/WaveformGenerator.cpp
        libresidfp/src/State.cpp
        libresidfp/src/residfp/residfp.cpp

        libresidfp/src/resample/SincResampler.cpp
)

SET_SOURCE_FILES_PROPERTIES(${srcs}
        PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/libresidfp/src;")

if(ltoSupported)
    set_target_properties(libresidfp PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_sources(libresidfp PRIVATE ${srcs})
