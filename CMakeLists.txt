cmake_minimum_required(VERSION 3.14)

project(denise LANGUAGES CXX)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_package(OpenAL)
elseif(CMAKE_GENERATOR STREQUAL "MSYS Makefiles")
    find_package(Freetype)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FREETYPE2 freetype2)
    pkg_check_modules(LIBUSB libusb-1.0)
    pkg_check_modules(LIBLAME lame)
elseif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    find_package(Freetype)
    find_package(PkgConfig REQUIRED)
    find_package(OpenAL)
    pkg_check_modules(FREETYPE2 freetype2)
    include(data/FindLame.cmake)
    find_package(PulseAudio REQUIRED)
    pkg_check_modules(GTK REQUIRED gtk+-3.0)
    pkg_check_modules(X11 REQUIRED x11 xfixes xext)
    pkg_check_modules(XCB REQUIRED xcb xcb-xinput xcb-xfixes xkbcommon)
    pkg_check_modules(LIBUSB libusb-1.0)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

set(AppName "Denise")
set(translationFolder "translation")
set(dataFolder "data")
set(fontFolder "fonts")
set(shaderFolder "shader")
set(imagesFolder "images")
set(soundFolder "sounds")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_NO_SYSTEM_FROM_IMPORTED YES)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES ${CMAKE_HOST_SYSTEM_PROCESSOR})
    endif()

    if (${CMAKE_OSX_ARCHITECTURES} STREQUAL "arm64")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11")
        set(macArchFolder "macArm")
    else()
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
        set(macArchFolder "macIntel")
    endif()
endif()

message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_link_options( -s )
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3") # prevents NDEBUG, which slightly decrease execution speed
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")


if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE ${CMAKE_BINARY_DIR}/out)
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE /Applications)
    endif()
endif()

message(STATUS "Setting Install Folder to ${CMAKE_INSTALL_PREFIX}")
#message(STATUS "LIBUSB ${LIBUSB_INCLUDE_DIRS}")

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "compile for architecture ${CMAKE_OSX_ARCHITECTURES}")
endif()

set(architecture)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(architecture "x86")
    message(STATUS "32 bit build")
else()
    set(architecture "x64")
endif()

set(srcs
        guikit/tools/decode/lha.cpp
        guikit/tools/decode/images.cpp
        guikit/tools/encode/images.cpp
        guikit/api.cpp

        driver/driver.cpp
        driver/tools/spirvReflection.cpp
        driver/tools/ShaderCache.cpp

        driver/video/thread/renderThread.cpp
        program/thread/emuThread.cpp
        emulation/libami/interface.cpp
        emulation/libc64/interface.cpp
        emulation/libc64/m6510/m6510.cpp
        emulation/libc64/disk/cpu/m6502.cpp
        emulation/libc64/m6510/dasmHandler.cpp
        emulation/tools/watcher.cpp

        emulation/cia/base.cpp
        emulation/cia/m6526.cpp
        emulation/cia/new/cia.cpp

        emulation/libami/cpu/m68000/m68000.cpp
        emulation/libami/cpu/m68000/dasmHandler.cpp
        emulation/libami/cpu/m68000.cpp
        emulation/libami/agnus/agnus.cpp
        emulation/libami/agnus/blitter.cpp
        emulation/libami/agnus/copper.cpp
        emulation/libami/video/denise.cpp
        emulation/libami/paula/paula.cpp
        emulation/libami/system/system.cpp
        emulation/libami/input/input.cpp
        emulation/libami/input/controlPort/controlPort.cpp
        emulation/libami/input/keyboard/keyboard.cpp
        emulation/libami/drive/diskDrive.cpp
        emulation/libami/drive/hardDrive.cpp
        emulation/libami/drive/hardDiskStructure.cpp
        emulation/libami/drive/diskStructure.cpp
        emulation/libami/drive/sectorBlock.cpp
        emulation/libami/drive/filesystem.cpp
        emulation/libami/drive/filesystemExt.cpp
        emulation/libami/drive/hunk.cpp
        emulation/libami/drive/scp/scp.cpp
        emulation/libami/system/rtc.cpp
        emulation/libami/drive/dms/pfile.cpp
        emulation/libami/expansionPort/expansionPort.cpp
        emulation/libami/expansionPort/fastMem.cpp
        emulation/libami/expansionPort/hdController.cpp
        emulation/libami/expansionPort/mtecAT500.cpp
        emulation/libami/expansionPort/builtinHD.cpp
        emulation/libami/system/firmware.cpp
        emulation/tools/DLLoader.cpp
        emulation/tools/uuid.cpp
        emulation/libc64/vicII/base.cpp
        emulation/libc64/vicII/vicII.cpp
        emulation/libc64/vicII/fast/vicIIFast.cpp
        emulation/libc64/system/system.cpp
        emulation/libc64/system/gluelogic.cpp
        emulation/libc64/system/firmware.cpp
        emulation/libc64/expansionPort/cart/cart.cpp
        emulation/libc64/system/keyBuffer.cpp
        emulation/libc64/expansionPort/gameCart/gameCart.cpp
        emulation/libc64/expansionPort/freezer/freezer.cpp
        emulation/libc64/expansionPort/reu/reu.cpp
        emulation/libc64/expansionPort/geoRam/geoRam.cpp
        emulation/libc64/expansionPort/easyFlash/easyFlash.cpp
        emulation/libc64/expansionPort/easyFlash/easyFlash3.cpp
        emulation/libc64/expansionPort/retroReplay/retroReplay.cpp
        emulation/libc64/expansionPort/gmod/gmod2.cpp
        emulation/libc64/expansionPort/fastloader/fastloader.cpp
        emulation/libc64/expansionPort/finalChessCard/chessCard.cpp
        emulation/libc64/system/clipboard.cpp
        emulation/libc64/sid/sid.cpp
        emulation/libc64/sid/usbSidPico.cpp
        emulation/libc64/system/sidManager.cpp
        emulation/libc64/sid/filter/chamberlin.cpp
        emulation/libc64/tape/tape.cpp
        emulation/libc64/tape/structure.cpp
        emulation/libc64/prg/prg.cpp
        emulation/libc64/input/input.cpp
        emulation/libc64/input/controlPort/controlPort.cpp
        emulation/libc64/expansionPort/acia/acia.cpp
        emulation/libc64/disk/wd177x/wd1770.cpp
        emulation/wdc/65816/w65816.cpp
        emulation/wdc/65816/dasmHandler.cpp
        emulation/wdc/65C02/w65C02.cpp
        emulation/libc64/expansionPort/superCpu/superCpu.cpp
        emulation/libc64/traps/traps.cpp
        emulation/libc64/disk/virtual/virtualDrive.cpp
        emulation/libc64/disk/via/via.cpp
        emulation/tools/pia.cpp
        emulation/libc64/disk/iec.cpp
        emulation/libc64/disk/drive/drive.cpp
        emulation/libc64/disk/structure/structure.cpp
        emulation/tools/m93c86.cpp
        emulation/tools/mx29lv640eb.cpp
        emulation/tools/error.cpp
        data/icons/icons.cpp
        data/icons/logos.cpp
        emulation/tools/socket.cpp
        emulation/tools/fpaq0.cpp
        emulation/tools/expressionParser.cpp

        program/program.cpp
        program/input/manager.cpp
        program/view/view.cpp
        program/debugger/debugger.cpp
        program/debugger/cpuDebugger.cpp
        program/debugger/memDebugger.cpp
        program/debugger/ciaDebugger.cpp
        program/debugger/videoDebugger.cpp
        program/debugger/audioDebugger.cpp
        program/debugger/dmaDebugger.cpp
        program/view/status.cpp
        program/config/config.cpp
        program/emuconfig/config.cpp
        program/emuconfig/layouts/model.cpp
        program/emuconfig/layouts/presentation.cpp
        program/emuconfig/layouts/audio.cpp
        program/emuconfig/layouts/configurations.cpp
        program/emuconfig/layouts/firmware.cpp
        program/emuconfig/layouts/geometry.cpp
        program/emuconfig/layouts/input.cpp
        program/emuconfig/layouts/misc.cpp
        program/emuconfig/layouts/palette.cpp
        program/emuconfig/layouts/system.cpp
        program/media/media.cpp
        program/media/autoloader.cpp
        program/media/fileloader.cpp
        program/media/recentFiles.cpp
        program/config/archiveViewer.cpp
        program/states/states.cpp
        program/audio/manager.cpp
        program/audio/dsp/bass.cpp
        program/audio/dsp/reverb.cpp
        program/audio/dsp/echo.cpp
        program/audio/dsp/panning.cpp
        program/audio/resampler/cosine.cpp
        program/audio/resampler/cosineSSE.cpp
        program/audio/resampler/sinc.cpp
        program/audio/record/handler.cpp
        program/audio/record/wavWriter.cpp
        program/audio/mixer/drive.cpp
        program/firmware/manager.cpp
        program/cmd/cmd.cpp
        program/video/palette.cpp
        program/video/manager.cpp
        program/video/shaderParser.cpp
        program/view/imageViewer.cpp
        program/tools/httpClient.cpp
        program/tools/diskGuesser.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(srcs ${srcs}
            driver/input/dinput/v5.cpp
            driver/input/dinput/v7.cpp
            driver/input/dinput/v8.cpp
            driver/audio/xaudio2/xaudio27.cpp
            driver/audio/xaudio2/xaudio28.cpp
            driver/audio/xaudio2/xaudio29.cpp
            data/resource_mingw.rc
    )
endif()

SET_SOURCE_FILES_PROPERTIES(guikit/tools/decode/lha.cpp PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/lha")
SET_SOURCE_FILES_PROPERTIES(guikit/tools/decode/images.cpp PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/stb")
SET_SOURCE_FILES_PROPERTIES(guikit/tools/encode/images.cpp PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/stb;${CMAKE_SOURCE_DIR}/deps/cgif")
SET_SOURCE_FILES_PROPERTIES(emulation/libc64/sid/usbSidPico.cpp PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/USBSID-Pico")

set(headerSrcs)
set(depSrcs)
set(usblibSrcs)
add_subdirectory(deps)
add_subdirectory(data)

set_source_files_properties(${headerSrcs} PROPERTIES HEADER_FILE_ONLY TRUE)

SET_SOURCE_FILES_PROPERTIES(${depSrcs}
    PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/glslang;")

SET_SOURCE_FILES_PROPERTIES(driver/driver.cpp
    PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/glslang;")

set(APP_INCLUDE "${CMAKE_SOURCE_DIR}/data/libs/include")

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    SET_SOURCE_FILES_PROPERTIES(${depSrcs} PROPERTIES COMPILE_FLAGS "-DENABLE_HLSL")

    SET_SOURCE_FILES_PROPERTIES(guikit/api.cpp PROPERTIES COMPILE_FLAGS "-DGUIKIT_WINAPI")

    SET_SOURCE_FILES_PROPERTIES(driver/driver.cpp PROPERTIES COMPILE_FLAGS "-DDRV_WGL -DDRV_DIRECT3D9 -DDRV_DIRECT3D11 -DDRV_DSOUND -DDRV_DINPUT5 -DDRV_DINPUT7 -DDRV_DINPUT8 -DDRV_XAUDIO27 -DDRV_XAUDIO28 -DDRV_XAUDIO29 -DDRV_WASAPI -DDRV_RAWINPUT")

    if(NOT CMAKE_GENERATOR MATCHES "MSYS Makefiles")
        set(FREETYPE2_FOUND true)
        set(FREETYPE2_INCLUDE_DIRS "${APP_INCLUDE}")

        set(LIBUSB_FOUND true)
        set(LIBUSB_INCLUDE_DIRS "${APP_INCLUDE}")

        set(LIBLAME_FOUND true)
        set(LIBLAME_INCLUDE_DIRS "${APP_INCLUDE}")
    endif()

    SET_SOURCE_FILES_PROPERTIES(driver/input/dinput/v5.cpp PROPERTIES COMPILE_FLAGS "-DDRV_DINPUT5")
    SET_SOURCE_FILES_PROPERTIES(driver/input/dinput/v7.cpp PROPERTIES COMPILE_FLAGS "-DDRV_DINPUT7")
    SET_SOURCE_FILES_PROPERTIES(driver/input/dinput/v8.cpp PROPERTIES COMPILE_FLAGS "-DDRV_DINPUT8")

    SET_SOURCE_FILES_PROPERTIES(driver/audio/xaudio2/xaudio27.cpp PROPERTIES COMPILE_FLAGS "-DDRV_XAUDIO27 -DDRV_XAUDIO28 -DDRV_XAUDIO29")
    SET_SOURCE_FILES_PROPERTIES(driver/audio/xaudio2/xaudio28.cpp PROPERTIES COMPILE_FLAGS "-DDRV_XAUDIO27 -DDRV_XAUDIO28 -DDRV_XAUDIO29 -Wno-attributes")
    SET_SOURCE_FILES_PROPERTIES(driver/audio/xaudio2/xaudio29.cpp PROPERTIES COMPILE_FLAGS "-DDRV_XAUDIO27 -DDRV_XAUDIO28 -DDRV_XAUDIO29 -Wno-attributes")

elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    SET_SOURCE_FILES_PROPERTIES(guikit/api.cpp
            PROPERTIES COMPILE_FLAGS "-x objective-c++ -DGUIKIT_COCOA")

    SET_SOURCE_FILES_PROPERTIES(driver/driver.cpp
            PROPERTIES COMPILE_FLAGS "-x objective-c++ -DDRV_METAL -DDRV_CGL -DDRV_COREAUDIO -DDRV_COREAUDIO3 -DDRV_IOKIT -DDRV_COCOA_IOKIT")

    set(FREETYPE2_FOUND true)
    set(FREETYPE2_INCLUDE_DIRS "${APP_INCLUDE}")

    set(LIBUSB_FOUND true)
    set(LIBUSB_INCLUDE_DIRS "${APP_INCLUDE}")

    set(LIBLAME_FOUND true)
    set(LIBLAME_INCLUDE_DIRS "${APP_INCLUDE}")
else()
    SET_SOURCE_FILES_PROPERTIES(guikit/api.cpp
            PROPERTIES COMPILE_FLAGS "-DGUIKIT_GTK")

    SET_SOURCE_FILES_PROPERTIES(guikit/api.cpp
            PROPERTIES INCLUDE_DIRECTORIES "${GTK_INCLUDE_DIRS}")

    if (CMAKE_SYSTEM_NAME STREQUAL "NetBSD" OR CMAKE_SYSTEM_NAME STREQUAL "OpenBSD" OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD") # no udev
        SET_SOURCE_FILES_PROPERTIES(driver/driver.cpp
                PROPERTIES COMPILE_FLAGS "-DDRV_GLX -DDRV_PULSEAUDIO -DDRV_XLIB -DDRV_XCB -DDRV_UHID")
    else()
        SET_SOURCE_FILES_PROPERTIES(driver/driver.cpp
                PROPERTIES COMPILE_FLAGS "-DDRV_GLX -DDRV_PULSEAUDIO -DDRV_XLIB -DDRV_XCB -DDRV_UDEV")
    endif()

    set_property(SOURCE driver/driver.cpp APPEND_STRING PROPERTY INCLUDE_DIRECTORIES "${GTK_INCLUDE_DIRS};")

    if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD") # libusb_dev_mem_ stuff undeclared
        set(LIBUSB_FOUND false)
    endif()
endif()

if(LIBLAME_FOUND)
    set(srcs ${srcs} program/audio/record/mp3Writer.cpp)

    SET_SOURCE_FILES_PROPERTIES(program/audio/record/mp3Writer.cpp
        PROPERTIES INCLUDE_DIRECTORIES "${LIBLAME_INCLUDE_DIRS};")
    SET_SOURCE_FILES_PROPERTIES(program/audio/record/handler.cpp
        PROPERTIES INCLUDE_DIRECTORIES "${LIBLAME_INCLUDE_DIRS};")

    set_property(SOURCE program/audio/record/handler.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " -DLIBLAME ")
endif()

if(FREETYPE2_FOUND)
    set_property(SOURCE driver/driver.cpp APPEND_STRING PROPERTY INCLUDE_DIRECTORIES "${FREETYPE2_INCLUDE_DIRS};")

    set_property(SOURCE driver/driver.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " -DDRV_FREETYPE ")
endif()

if(LIBUSB_FOUND)
    SET_SOURCE_FILES_PROPERTIES(${usblibSrcs}
            PROPERTIES INCLUDE_DIRECTORIES "${LIBUSB_INCLUDE_DIRS};")

    if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        SET_SOURCE_FILES_PROPERTIES(${usblibSrcs} PROPERTIES COMPILE_FLAGS "-DHAVE_ALIGNED_ALLOC")
    elseif(${CMAKE_OSX_ARCHITECTURES} STREQUAL "arm64")
        SET_SOURCE_FILES_PROPERTIES(${usblibSrcs} PROPERTIES COMPILE_FLAGS "-DHAVE_ALIGNED_ALLOC")
    endif()

    SET_SOURCE_FILES_PROPERTIES(emulation/libc64/sid/usbSidPico.cpp PROPERTIES COMPILE_FLAGS "-DLIBUSB")
else()
    set(usblibSrcs)
endif()

if(OPENAL_FOUND)
    set_property(SOURCE driver/driver.cpp APPEND_STRING PROPERTY INCLUDE_DIRECTORIES "${OPENAL_INCLUDE_DIRS};")
    set_property(SOURCE driver/driver.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " -DDRV_OPENAL ")
endif()

SET_SOURCE_FILES_PROPERTIES(emulation/libc64/sid/filter/chamberlin.cpp
        PROPERTIES COMPILE_FLAGS "-ffast-math -fno-exceptions")

SET_SOURCE_FILES_PROPERTIES(program/program.cpp
        PROPERTIES COMPILE_FLAGS "-DAPP_NAME=\"\\\"${AppName}\\\"\" -DTRANSLATION_FOLDER=\"\\\"${translationFolder}/\\\"\" -DDATA_FOLDER=\"\\\"${dataFolder}/\\\"\" -DSHADER_FOLDER=\"\\\"${shaderFolder}/\\\"\" -DIMAGES_FOLDER=\"\\\"${imagesFolder}/\\\"\" -DSOUND_FOLDER=\"\\\"${soundFolder}/\\\"\" ")

SET_SOURCE_FILES_PROPERTIES(guikit/tools/decode/images.cpp
        PROPERTIES COMPILE_FLAGS "-Wno-stringop-overflow")

set(srcs ${srcs} ${depSrcs} ${usblibSrcs} ${headerSrcs})
add_executable( ${PROJECT_NAME} ${srcs} )
target_compile_options(${PROJECT_NAME} PUBLIC -Wno-shift-count-overflow)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(${PROJECT_NAME} "-static -lws2_32 -lgdi32 -lcomdlg32 -lole32 -lcomctl32 -lshlwapi -lmsimg32 -lwinmm -lopengl32 -ldwmapi -luxtheme -lgdiplus")
    if(CMAKE_BUILD_TYPE STREQUAL Release)
        target_link_libraries(${PROJECT_NAME} "-mwindows")
    else()
        target_link_libraries(${PROJECT_NAME} "-mconsole")
    endif()    

    if(CMAKE_GENERATOR STREQUAL "MSYS Makefiles")
        if(FREETYPE2_FOUND)
            target_link_libraries(${PROJECT_NAME} ${FREETYPE2_STATIC_LIBRARIES})
        endif()

        if(LIBUSB_FOUND)
            target_link_libraries(${PROJECT_NAME} ${LIBUSB_STATIC_LIBRARIES})
        endif()

        if(LIBLAME_FOUND)
            target_link_libraries(${PROJECT_NAME} ${LIBLAME_STATIC_LIBRARIES})
        endif()
    else()
        if(FREETYPE2_FOUND)
            set(FREETYPE_LIBS libfreetype.a libpng16.a libz.a libbz2.a libbrotlidec.a libbrotlicommon.a)
            foreach(_lib IN LISTS FREETYPE_LIBS)
                target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/data/libs/static/${architecture}/${_lib}")
            endforeach()
        endif()

        if(LIBUSB_FOUND)
            target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/data/libs/static/${architecture}/libusb-1.0.a")
        endif()

        if(LIBLAME_FOUND)
            target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/data/libs/static/${architecture}/libmp3lame.a")
        endif()
    endif()

elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa -framework CoreVideo -framework IOKit -framework Carbon -framework Metal -framework MetalKit -framework OpenGL -framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework AVFoundation")
    target_link_libraries(${PROJECT_NAME} ${FREETYPE_LIBRARY})

    if(LIBUSB_FOUND)
        set(LIBUSB_LIBRARY "${CMAKE_SOURCE_DIR}/data/libs/shared/${macArchFolder}/libusb-1.0.0.dylib")
        target_link_libraries(${PROJECT_NAME} ${LIBUSB_LIBRARY})
    endif()

    if(LIBLAME_FOUND)
        set(LIBLAME_LIBRARY "${CMAKE_SOURCE_DIR}/data/libs/shared/${macArchFolder}/libmp3lame.0.dylib")
        target_link_libraries(${PROJECT_NAME} ${LIBLAME_LIBRARY})
    endif()

    if(FREETYPE2_FOUND)
        set(FREETYPE_LIBS libbrotlicommon.1.dylib libbrotlidec.1.dylib libbz2.1.0.dylib libpng16.16.dylib libz.1.dylib libfreetype.6.dylib)
        set(FREETYPE2_LIBRARIES)
        foreach(_lib IN LISTS FREETYPE_LIBS)
           set(FREETYPE2_LIBRARIES ${FREETYPE2_LIBRARIES} ${CMAKE_SOURCE_DIR}/data/libs/shared/${macArchFolder}/${_lib})
        endforeach()

        target_link_libraries(${PROJECT_NAME} ${FREETYPE2_LIBRARIES})
    endif()

    set_target_properties(${PROJECT_NAME} PROPERTIES OSX_ARCHITECTURES ${CMAKE_OSX_ARCHITECTURES})
    set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_RPATH "@executable_path/../Frameworks")
else()
    target_link_libraries(${PROJECT_NAME} "-lpthread -lfontconfig -lX11 -lXrandr")
    target_link_directories(${PROJECT_NAME} PUBLIC ${GTK_LIBRARY_DIRS})    
    target_link_directories(${PROJECT_NAME} PUBLIC ${X11_LIBRARY_DIRS})
    target_link_directories(${PROJECT_NAME} PUBLIC ${XCB_LIBRARY_DIRS})    
    target_link_libraries(${PROJECT_NAME} ${GTK_LIBRARIES})    
    target_link_libraries(${PROJECT_NAME} ${X11_LIBRARIES})
    target_link_libraries(${PROJECT_NAME} ${XCB_LIBRARIES})

    if(FREETYPE2_FOUND)
        target_link_directories(${PROJECT_NAME} PUBLIC ${FREETYPE2_LIBRARY_DIRS})
        target_link_libraries(${PROJECT_NAME} ${FREETYPE2_LIBRARIES})
    endif()

    if(LIBUSB_FOUND)
        target_link_directories(${PROJECT_NAME} PUBLIC ${LIBUSB_LIBRARY_DIRS})
        target_link_libraries(${PROJECT_NAME} ${LIBUSB_LIBRARIES})
    endif()

    if(LIBLAME_FOUND)
        target_link_directories(${PROJECT_NAME} PUBLIC ${LIBLAME_LIBRARY_DIRS})
        target_link_libraries(${PROJECT_NAME} ${LIBLAME_LIBRARIES})
    endif()

    # use -Wl,--verbose for detailed linker problems
    if (CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
        target_link_libraries(${PROJECT_NAME} "-luuid -lrt -lGL -lpulse -lusbhid")
        # otherwise CMAKE removes runtime paths when installing to final location, check binary with ldd command to see assigned shared libs
        set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
        target_link_libraries(${PROJECT_NAME} "-lGL -lpulse -lusbhid")
        set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        target_link_libraries(${PROJECT_NAME} "-lrt -ldl -lGL -lpulse -lusbhid")
    else() # linux
        target_link_libraries(${PROJECT_NAME} "-luuid -no-pie -lrt -ldl -lGL -ludev -lpulse")
    endif()
endif()

if(OPENAL_FOUND)
    target_link_directories(${PROJECT_NAME} PUBLIC ${OPENAL_LIBRARY_DIRS})
    target_link_libraries(${PROJECT_NAME} ${OPENAL_LIBRARY})
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(DIRECTORY data/${translationFolder} DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(DIRECTORY data/${dataFolder} DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(DIRECTORY data/${fontFolder} DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.ttf")
    install(DIRECTORY data/${soundFolder} DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(DIRECTORY data/${shaderFolder} DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(DIRECTORY data/${imagesFolder} DESTINATION ${CMAKE_INSTALL_PREFIX})

    install(FILES readme.md DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(FILES licence.md DESTINATION ${CMAKE_INSTALL_PREFIX})

elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    install(DIRECTORY data/${translationFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)
    install(DIRECTORY data/${dataFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)
    install(DIRECTORY data/${fontFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources FILES_MATCHING PATTERN "*.ttf")
    install(DIRECTORY data/${shaderFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)
    install(DIRECTORY data/${imagesFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)
    install(DIRECTORY data/${soundFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)

    install(FILES data/Info.plist DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents)
    install(FILES licence.md DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)
    install(FILES readme.md DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)
    install(FILES data/img/${PROJECT_NAME}.icns DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)

    install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/MacOS)

    if(LIBUSB_FOUND)
        install(FILES ${LIBUSB_LIBRARY} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Frameworks)
    endif()

    if(LIBLAME_FOUND)
        install(FILES ${LIBLAME_LIBRARY} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Frameworks)
    endif()

    if(FREETYPE2_FOUND)
        install(FILES ${FREETYPE2_LIBRARIES} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Frameworks)   
    endif()

    install(CODE "execute_process(COMMAND codesign --force --deep -s - ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app)")

else()
    install(DIRECTORY data/${translationFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
    install(DIRECTORY data/${dataFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
    install(DIRECTORY data/${fontFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME} FILES_MATCHING PATTERN "*.ttf")
    install(DIRECTORY data/${soundFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
    install(DIRECTORY data/${shaderFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
    install(DIRECTORY data/${imagesFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})

    install(FILES data/${PROJECT_NAME}.appdata.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/share/appdata)
    install(FILES data/img/${PROJECT_NAME}.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps)
    install(FILES data/img/${PROJECT_NAME}_48.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps RENAME ${PROJECT_NAME}.png)
    install(FILES data/img/${PROJECT_NAME}_48.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons RENAME ${PROJECT_NAME}.png)
    install(FILES data/img/${PROJECT_NAME}_32.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/32x32/apps RENAME ${PROJECT_NAME}.png)
    install(FILES data/${PROJECT_NAME}.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE)
    install(CODE "execute_process(COMMAND update-icon-caches ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor)")

    if(INSTALL_FILE_ASSOCIATIONS STREQUAL 1)
        install(FILES data/application-x-${PROJECT_NAME}.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/share/mime/packages)
        install(CODE "execute_process(COMMAND update-mime-database ${CMAKE_INSTALL_PREFIX}/share/mime)")
        install(CODE "execute_process(COMMAND update-desktop-database ${CMAKE_INSTALL_PREFIX}/share/applications)")
    endif()

    install(TARGETS ${PROJECT_NAME} DESTINATION bin)
endif()

add_custom_target(install_${PROJECT_NAME}
        "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}"
        DEPENDS ${PROJECT_NAME}
        COMMENT "Installing ${PROJECT_NAME}")

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_custom_target("uninstall" COMMENT "Uninstall installed files")
    add_custom_command(
            TARGET "uninstall"
            POST_BUILD
            COMMENT "Uninstall files with install_manifest.txt"
            COMMAND xargs rm -vf < install_manifest.txt || echo Nothing to be uninstalled!
    )
elseif (${CMAKE_GENERATOR} STREQUAL "MSYS Makefiles")
    add_custom_target("uninstall" COMMENT "Uninstall installed files")
    add_custom_command(
            TARGET "uninstall"
            POST_BUILD
            COMMENT "Uninstall files with install_manifest.txt"
            COMMAND tr -d '\\r' < install_manifest.txt > im2.txt
            COMMAND xargs rm -vf < im2.txt || echo Nothing to be uninstalled!
            COMMAND rm im2.txt
    )
endif()
