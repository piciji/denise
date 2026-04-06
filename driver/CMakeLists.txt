
add_library(driver STATIC)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(headers
            audio/xaudio2/core.cpp
            audio/xaudio2/header27.h
            audio/xaudio2/header29.h
            audio/xaudio2/xaudio2.h
            audio/xaudio2/xaudio27.h
            audio/xaudio2/xaudio28.h
            audio/xaudio2/xaudio29.h
            audio/dsound.cpp
            audio/wasapi.cpp
            input/dinput/core.cpp
            input/dinput/dinput.h
            input/dinput/format.h
            input/dinput/v5.h
            input/dinput/v7.h
            input/dinput/v8.h
            input/rawinput/hid.cpp
            input/rawinput/joypad.cpp
            input/rawinput/keyboard.cpp
            input/rawinput/main.cpp
            input/rawinput/mouse.cpp
            video/d3d9/dragnDropOverlay.h
            video/d3d9/dvideo.cpp
            video/d3d9/screenText.h
            video/d3d9/symbols.h
            video/d3d11/d3d11.cpp
            video/d3d11/dxgiHandler.h
            video/d3d11/shaders.h
            video/d3d11/symbols.h
            video/d3d11/types.h
            video/d3d11/utility.h
            video/opengl3/wgl.cpp
            tools/win.h
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(headers
            audio/openal.cpp
            audio/coreaudio.cpp
            input/iokit/joypad.cpp
            input/iokit/keyboard.cpp
            input/iokit/keyNames.cpp
            input/iokit/main.cpp
            input/iokit/mouse.cpp
            video/opengl3/cgl.cpp
            video/metal/interface.h
            video/metal/metal.cpp
            video/metal/shaders.h
            video/metal/utility.h
    )
else()
    set(headers
            audio/openal.cpp
            audio/pulseaudio.cpp
            input/udev.cpp
            input/uhid.cpp
            input/xcb.cpp
            input/xlib.cpp
            video/opengl3/glx.cpp
    )
endif()

set(headers ${headers}
        driver.h
        video/opengl3/bind.h
        video/opengl3/dragndropOverlay.h
        video/opengl3/gl3.cpp
        video/opengl3/progress.h
        video/opengl3/screenText.h
        video/opengl3/shaders.h
        video/opengl3/types.h
        video/opengl3/utility.h
        video/thread/renderThread.h
        video/dragnDropOverlay.h
        video/freetype.h
        video/viewport.h
        tools/chronos.h
        tools/crc32.h
        tools/glslang.h
        tools/hid.h
        tools/image.h
        tools/md5.h
        tools/ShaderCache.h
        tools/shaderpass.h
        tools/spirvReflection.h
        tools/threadPriority.h
        tools/tools.h
        tools/utf8.h
)

set_source_files_properties(driver ${headers}
        PROPERTIES HEADER_FILE_ONLY TRUE)

set(srcs
        driver.cpp
        tools/spirvReflection.cpp
        tools/ShaderCache.cpp
        video/thread/renderThread.cpp
)

SET_SOURCE_FILES_PROPERTIES(driver.cpp
        PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/glslang;")

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(srcs ${srcs}
            input/dinput/v5.cpp
            input/dinput/v7.cpp
            input/dinput/v8.cpp
            audio/xaudio2/xaudio27.cpp
            audio/xaudio2/xaudio28.cpp
            audio/xaudio2/xaudio29.cpp
    )

    SET_SOURCE_FILES_PROPERTIES(driver.cpp PROPERTIES COMPILE_FLAGS "-DDRV_WGL -DDRV_DIRECT3D9 -DDRV_DIRECT3D11 -DDRV_DSOUND -DDRV_DINPUT5 -DDRV_DINPUT7 -DDRV_DINPUT8 -DDRV_XAUDIO27 -DDRV_XAUDIO28 -DDRV_XAUDIO29 -DDRV_WASAPI -DDRV_RAWINPUT")

    SET_SOURCE_FILES_PROPERTIES(input/dinput/v5.cpp PROPERTIES COMPILE_FLAGS "-DDRV_DINPUT5")
    SET_SOURCE_FILES_PROPERTIES(input/dinput/v7.cpp PROPERTIES COMPILE_FLAGS "-DDRV_DINPUT7")
    SET_SOURCE_FILES_PROPERTIES(input/dinput/v8.cpp PROPERTIES COMPILE_FLAGS "-DDRV_DINPUT8")

    SET_SOURCE_FILES_PROPERTIES(audio/xaudio2/xaudio27.cpp PROPERTIES COMPILE_FLAGS "-DDRV_XAUDIO27 -DDRV_XAUDIO28 -DDRV_XAUDIO29")
    SET_SOURCE_FILES_PROPERTIES(audio/xaudio2/xaudio28.cpp PROPERTIES COMPILE_FLAGS "-DDRV_XAUDIO27 -DDRV_XAUDIO28 -DDRV_XAUDIO29 -Wno-attributes")
    SET_SOURCE_FILES_PROPERTIES(audio/xaudio2/xaudio29.cpp PROPERTIES COMPILE_FLAGS "-DDRV_XAUDIO27 -DDRV_XAUDIO28 -DDRV_XAUDIO29 -Wno-attributes")

elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    SET_SOURCE_FILES_PROPERTIES(driver.cpp
            PROPERTIES COMPILE_FLAGS "-x objective-c++ -DDRV_METAL -DDRV_CGL -DDRV_COREAUDIO -DDRV_COREAUDIO3 -DDRV_IOKIT -DDRV_COCOA_IOKIT")

else()
    if (CMAKE_SYSTEM_NAME MATCHES "BSD") # no udev
        SET_SOURCE_FILES_PROPERTIES(driver.cpp
                PROPERTIES COMPILE_FLAGS "-DDRV_GLX -DDRV_PULSEAUDIO -DDRV_XLIB -DDRV_XCB -DDRV_UHID")
    else()
        SET_SOURCE_FILES_PROPERTIES(driver.cpp
                PROPERTIES COMPILE_FLAGS "-DDRV_GLX -DDRV_PULSEAUDIO -DDRV_XLIB -DDRV_XCB -DDRV_UDEV")
    endif()

    set_property(SOURCE driver.cpp APPEND_STRING PROPERTY INCLUDE_DIRECTORIES "${GTK_INCLUDE_DIRS};")
endif()

if(FREETYPE2_FOUND)
    set_property(SOURCE driver.cpp APPEND_STRING PROPERTY INCLUDE_DIRECTORIES "${FREETYPE2_INCLUDE_DIRS};")
    set_property(SOURCE driver.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " -DDRV_FREETYPE ")
    target_link_libraries(driver PRIVATE ${FREETYPE2_LIBRARIES})
    target_link_directories(driver PRIVATE ${FREETYPE2_LIBRARY_DIRS})
endif()

if(OPENAL_FOUND)
    set_property(SOURCE driver.cpp APPEND_STRING PROPERTY INCLUDE_DIRECTORIES "${OPENAL_INCLUDE_DIRS};")
    set_property(SOURCE driver.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " -DDRV_OPENAL ")
    target_link_libraries(driver PRIVATE ${OPENAL_LIBRARY})
    target_link_directories(driver PRIVATE ${OPENAL_LIBRARY_DIRS})
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(driver PRIVATE opengl32)

elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(driver PRIVATE "-framework CoreVideo -framework IOKit -framework Carbon -framework Metal -framework MetalKit -framework OpenGL -framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework AVFoundation")

else()
    target_link_directories(driver PUBLIC ${X11_LIBRARY_DIRS})
    target_link_directories(driver PUBLIC ${XCB_LIBRARY_DIRS})
    target_link_libraries(driver PUBLIC ${X11_LIBRARIES})
    target_link_libraries(driver PUBLIC ${XCB_LIBRARIES})

    # use -Wl,--verbose for detailed linker problems
    if (CMAKE_SYSTEM_NAME MATCHES "BSD")
        target_link_libraries(driver PUBLIC GL pulse usbhid)

    else() # linux
        target_link_libraries(driver PUBLIC GL pulse udev)
    endif()
endif()

target_sources(driver PRIVATE ${srcs})

target_link_libraries(driver PRIVATE glslang spirv)