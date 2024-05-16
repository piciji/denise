cmake_minimum_required(VERSION 3.16)

project(denise LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
find_package(Freetype REQUIRED)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(SDL2 REQUIRED)
    pkg_check_modules(GTK REQUIRED gtk+-3.0)
endif()

find_package(OpenAL REQUIRED)
pkg_check_Modules(FREETYPE2 REQUIRED freetype2)

set(AppName "Denise")
set(translationFolder "translation")
set(dataFolder "data")
set(fontFolder "fonts")
set(shaderFolder "shader")
set(imgFolder "img")
set(soundFolder "sounds")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_CXX_COMPILER clang++)
    if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "ARM")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11")
    else()
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
    endif()
else()
    set(CMAKE_CXX_COMPILER g++)
endif()

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release)
    add_link_options( -s )
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3") # prevents NDEBUG, which slightly decrease execution speed
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Setting Install Folder to ${CMAKE_INSTALL_PREFIX}")
endif()
message(STATUS "compile for ${CMAKE_SYSTEM_PROCESSOR}")

set(srcs
        guikit/tools/decode/images.cpp
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

        emulation/cia/base.cpp
        emulation/cia/m6526.cpp
        emulation/cia/new/cia.cpp

        emulation/libami/cpu/m68000/m68000.cpp
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
        emulation/libami/drive/diskStructure.cpp
        emulation/libami/drive/sectorBlock.cpp
        emulation/libami/drive/filesystem.cpp
        emulation/libami/system/rtc.cpp
        emulation/libami/drive/dms/pfile.cpp
        emulation/tools/DLLoader.cpp
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
        emulation/libc64/system/clipboard.cpp
        emulation/libc64/sid/sid.cpp
        emulation/libc64/system/sidManager.cpp
        emulation/libc64/sid/filter/chamberlin.cpp
        emulation/libc64/tape/tape.cpp
        emulation/libc64/tape/structure.cpp
        emulation/libc64/prg/prg.cpp
        emulation/libc64/input/input.cpp
        emulation/libc64/input/controlPort/controlPort.cpp
        emulation/libc64/expansionPort/acia/acia.cpp
        emulation/libc64/disk/wd177x/wd1770.cpp

        emulation/libc64/traps/traps.cpp
        emulation/libc64/disk/virtual/virtualDrive.cpp
        emulation/libc64/disk/via/via.cpp
        emulation/tools/pia.cpp
        emulation/libc64/disk/iec.cpp
        emulation/libc64/disk/drive/drive.cpp
        emulation/libc64/disk/structure/structure.cpp
        emulation/tools/m93c86.cpp
        emulation/tools/mx29lv640eb.cpp
        data/icons/icons.cpp
        data/icons/logos.cpp
        data/fonts/fonts.cpp
        emulation/tools/socket.cpp
        emulation/tools/fpaq0.cpp

        program/program.cpp
        program/input/manager.cpp
        program/view/view.cpp
        program/view/status.cpp
        program/config/config.cpp
        program/emuconfig/config.cpp
        program/emuconfig/layouts/model.cpp
        program/media/media.cpp
        program/media/autoloader.cpp
        program/media/fileloader.cpp
        program/config/archiveViewer.cpp
        program/states/states.cpp
        program/audio/manager.cpp
        program/audio/dsp/bass.cpp
        program/audio/dsp/reverb.cpp
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
)

set(depSrcs)
add_subdirectory(deps)

SET_SOURCE_FILES_PROPERTIES(${depSrcs}
    PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/glslang")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    SET_SOURCE_FILES_PROPERTIES(guikit/api.cpp
        PROPERTIES COMPILE_FLAGS "${GTK_CFLAGS_OTHER} -DGUIKIT_GTK -DINSTALL_FOLDER=\"\\\"${CMAKE_INSTALL_PREFIX}\\\"\" ")

    SET_SOURCE_FILES_PROPERTIES(guikit/api.cpp
        PROPERTIES INCLUDE_DIRECTORIES "${GTK_INCLUDE_DIRS}")
else()
    if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "ARM")
        SET_SOURCE_FILES_PROPERTIES(${depSrcs}
            PROPERTIES COMPILE_FLAGS "-DMACOSX_DEPLOYMENT_TARGET11")
    else()
        SET_SOURCE_FILES_PROPERTIES(${depSrcs}
            PROPERTIES COMPILE_FLAGS "-DMACOSX_DEPLOYMENT_TARGET109")
    endif()

    SET_SOURCE_FILES_PROPERTIES(guikit/api.cpp
        PROPERTIES COMPILE_FLAGS "-x objective-c++ -DGUIKIT_COCOA")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    SET_SOURCE_FILES_PROPERTIES(driver/driver.cpp
        PROPERTIES COMPILE_FLAGS "${GTK_CFLAGS_OTHER} -DDRV_GLX -DDRV_OPENAL -DDRV_PULSEAUDIO -DDRV_XLIB -DDRV_UDEV -DDRV_FREETYPE -DDRV_SDLINPUT")

    SET_SOURCE_FILES_PROPERTIES(driver/driver.cpp
        PROPERTIES INCLUDE_DIRECTORIES "${GTK_INCLUDE_DIRS};${SDL2_INCLUDE_DIRS};${FREETYPE2_INCLUDE_DIRS};${CMAKE_SOURCE_DIR}/deps/glslang")
else()
    SET_SOURCE_FILES_PROPERTIES(driver/driver.cpp
        PROPERTIES COMPILE_FLAGS "-x objective-c++ -DDRV_CGL -DDRV_OPENAL -DDRV_COREAUDIO -DDRV_IOKIT -DDRV_FREETYPE")

    SET_SOURCE_FILES_PROPERTIES(driver/driver.cpp
        PROPERTIES INCLUDE_DIRECTORIES "${FREETYPE2_INCLUDE_DIRS};${CMAKE_SOURCE_DIR}/deps/glslang")
endif()

SET_SOURCE_FILES_PROPERTIES(emulation/libc64/sid/filter/chamberlin.cpp
        PROPERTIES COMPILE_FLAGS "-ffast-math -fno-exceptions")

SET_SOURCE_FILES_PROPERTIES(program/program.cpp
        PROPERTIES COMPILE_FLAGS "-DAPP_NAME=\"\\\"${AppName}\\\"\" -DTRANSLATION_FOLDER=\"\\\"${translationFolder}/\\\"\" -DDATA_FOLDER=\"\\\"${dataFolder}/\\\"\" -DSHADER_FOLDER=\"\\\"${shaderFolder}/\\\"\" -DIMG_FOLDER=\"\\\"${imgFolder}/\\\"\" -DSOUND_FOLDER=\"\\\"${soundFolder}/\\\"\" ")


set(srcs ${srcs} ${depSrcs})
add_executable( ${PROJECT_NAME} ${srcs} )

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(${PROJECT_NAME} "-lpthread -lfontconfig -lX11 -lXrandr -lrt")
    target_link_libraries(${PROJECT_NAME} ${GTK_LIBRARIES})
    target_link_libraries(${PROJECT_NAME} ${OPENAL_LIBRARY})
    target_link_libraries(${PROJECT_NAME} ${FREETYPE2_LIBRARIES})
    target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
    target_link_libraries(${PROJECT_NAME} "-lXext -ldl -lXfixes -lGL -ludev -lpulse")
else()
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa -framework CoreVideo -framework OpenAL -framework IOKit -framework Carbon -framework OpenGL -framework CoreAudio -framework AudioToolbox -framework AudioUnit")
    target_link_libraries(${PROJECT_NAME} ${OPENAL_LIBRARY})
    target_link_libraries(${PROJECT_NAME} ${FREETYPE2_LIBRARIES})
# target_link_libraries(${PROJECT_NAME} "-lc++ -lobjc")
    target_compile_options(${PROJECT_NAME} PUBLIC -Wno-shift-count-overflow)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
install(DIRECTORY data/${translationFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
install(DIRECTORY data/${dataFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
install(DIRECTORY data/${fontFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
install(DIRECTORY data/${soundFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
install(DIRECTORY data/${shaderFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
install(DIRECTORY data/${imgFolder}/bundle/ DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/${imgFolder})

install(FILES data/img/${PROJECT_NAME}.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons)
install(FILES data/${PROJECT_NAME}.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE)

if(INSTALL_FILE_ASSOCIATIONS STREQUAL 1)
    install(FILES data/application-x-${PROJECT_NAME}.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/share/mime/packages)
    install(CODE "execute_process(COMMAND update-mime-database ${CMAKE_INSTALL_PREFIX}/share/mime)")
    install(CODE "execute_process(COMMAND update-desktop-database ${CMAKE_INSTALL_PREFIX}/share/applications)")
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
else()
install(DIRECTORY data/${translationFolder} DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources)
install(DIRECTORY data/${dataFolder} DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources)
install(DIRECTORY data/${fontFolder} DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources)
install(DIRECTORY data/${shaderFolder} DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources)
install(DIRECTORY data/${imgFolder}/bundle/ DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources/${imgFolder})
install(DIRECTORY data/${soundFolder} DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources)

install(FILES data/Info.plist DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents)
install(FILES data/txt/licence.md DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources)
install(FILES readme.md DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources)
install(FILES data/img/${PROJECT_NAME}.icns DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources)

install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME} DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/MacOS PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE)

install(CODE "execute_process(COMMAND sh ${CMAKE_SOURCE_DIR}/data/bundle_dylib.sh ${FREETYPE2_LIBRARY_DIRS} lib${FREETYPE2_LIBRARIES}.dylib ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app ${PROJECT_NAME})")

install(CODE "execute_process(COMMAND codesign --force --deep -s - ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app)")
endif()

add_custom_target("uninstall" COMMENT "Uninstall installed files")
add_custom_command(
        TARGET "uninstall"
        POST_BUILD
        COMMENT "Uninstall files with install_manifest.txt"
        COMMAND xargs rm -vf < install_manifest.txt || echo Nothing in
        install_manifest.txt to be uninstalled!
)
