
add_executable(${PROJECT_NAME})

set(headers
        program.h
        audio/dsp/base.h
        audio/dsp/bass.h
        audio/dsp/data.h
        audio/dsp/echo.h
        audio/dsp/panning.h
        audio/dsp/reverb.h
        audio/mixer/drive.h
        audio/record/handler.h
        audio/record/wavWriter.h
        audio/resampler/cosine.h
        audio/resampler/cosinePrecise.h
        audio/resampler/cosineSSE.h
        audio/resampler/data.h
        audio/resampler/sinc.h
        audio/manager.h
        cmd/cmd.h
        config/layouts/drivers.h
        config/layouts/drivers.cpp
        config/layouts/settings.h
        config/layouts/settings.cpp
        config/archiveViewer.h
        config/config.h
        config/slider.h
        config/sliderAlt.h
        emuconfig/config.h
        emuconfig/layouts/model.h
        emuconfig/layouts/presentation.h
        firmware/manager.h
        input/capture.cpp
        input/global.cpp
        input/hotkeys.cpp
        input/manager.h
        input/mapping.cpp
        media/autoloader.h
        media/fileloader.h
        media/layout.cpp
        media/media.h
        media/swapper.cpp
        media/recentFiles.h
        states/states.h
        thread/emuThread.h
        tools/chronos.h
        tools/circularBuffer.h
        tools/dataStorage.h
        tools/filepool.h
        tools/filesetting.h
        tools/gaussianBlur.h
        tools/logger.h
        tools/mathVector.h
        tools/shortcuts.h
        tools/sincFirFilter.h
        tools/httpClient.h
        video/luts.cpp
        video/manager.h
        video/palette.h
        video/props.cpp
        video/shaderParser.h
        video/sync.cpp
        view/imageViewer.h
        view/message.h
        view/placeholder.cpp
        view/status.h
        view/view.h
        audio.cpp
        files.cpp
        input.cpp
        video.cpp
)

set_source_files_properties(${PROJECT_NAME} ${headers}
        PROPERTIES HEADER_FILE_ONLY TRUE)

set(srcs
        program.cpp
        thread/emuThread.cpp
        input/manager.cpp
        view/view.cpp
        debugger/debugger.cpp
        debugger/cpuDebugger.cpp
        debugger/memDebugger.cpp
        debugger/ciaDebugger.cpp
        debugger/videoDebugger.cpp
        debugger/audioDebugger.cpp
        debugger/dmaDebugger.cpp
        debugger/copperDebugger.cpp
        debugger/conditionViewDebugger.cpp
        debugger/watcherHelper.cpp
        view/status.cpp
        config/config.cpp
        emuconfig/config.cpp
        emuconfig/layouts/model.cpp
        emuconfig/layouts/presentation.cpp
        emuconfig/layouts/audio.cpp
        emuconfig/layouts/configurations.cpp
        emuconfig/layouts/firmware.cpp
        emuconfig/layouts/geometry.cpp
        emuconfig/layouts/input.cpp
        emuconfig/layouts/misc.cpp
        emuconfig/layouts/palette.cpp
        emuconfig/layouts/system.cpp
        media/media.cpp
        media/autoloader.cpp
        media/fileloader.cpp
        media/recentFiles.cpp
        config/archiveViewer.cpp
        states/states.cpp
        audio/manager.cpp
        audio/dsp/bass.cpp
        audio/dsp/reverb.cpp
        audio/dsp/echo.cpp
        audio/dsp/panning.cpp
        audio/resampler/cosine.cpp
        audio/resampler/cosineSSE.cpp
        audio/resampler/sinc.cpp
        audio/record/handler.cpp
        audio/record/wavWriter.cpp
        audio/mixer/drive.cpp
        firmware/manager.cpp
        cmd/cmd.cpp
        video/palette.cpp
        video/manager.cpp
        video/shaderParser.cpp
        view/imageViewer.cpp
        tools/httpClient.cpp
        tools/diskGuesser.cpp
        
        ${headers}
)

if(LIBLAME_FOUND)
    set(srcs ${srcs} audio/record/mp3Writer.cpp)

    SET_SOURCE_FILES_PROPERTIES(audio/record/mp3Writer.cpp
            PROPERTIES INCLUDE_DIRECTORIES "${LIBLAME_INCLUDE_DIRS};")
    SET_SOURCE_FILES_PROPERTIES(audio/record/handler.cpp
            PROPERTIES INCLUDE_DIRECTORIES "${LIBLAME_INCLUDE_DIRS};")

    set_property(SOURCE audio/record/handler.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " -DLIBLAME ")

    target_link_directories(${PROJECT_NAME} PUBLIC ${LIBLAME_LIBRARY_DIRS})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBLAME_LIBRARIES})
endif()

SET_SOURCE_FILES_PROPERTIES(program.cpp
        PROPERTIES COMPILE_FLAGS "-DAPP_NAME=\"\\\"${AppName}\\\"\" -DTRANSLATION_FOLDER=\"\\\"${translationFolder}/\\\"\" -DDATA_FOLDER=\"\\\"${dataFolder}/\\\"\" -DSHADER_FOLDER=\"\\\"${shaderFolder}/\\\"\" -DIMAGES_FOLDER=\"\\\"${imagesFolder}/\\\"\" -DSOUND_FOLDER=\"\\\"${soundFolder}/\\\"\" ")

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(srcs ${srcs} "${CMAKE_SOURCE_DIR}/${guikit.resource}" )

    target_link_libraries(${PROJECT_NAME} PUBLIC "-static")

    if(CMAKE_BUILD_TYPE STREQUAL Release)
        target_link_libraries(${PROJECT_NAME} PUBLIC "-mwindows")
    else()
        target_link_libraries(${PROJECT_NAME} PUBLIC "-mconsole")
    endif()

elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(${PROJECT_NAME} PROPERTIES OSX_ARCHITECTURES ${CMAKE_OSX_ARCHITECTURES})
    set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_RPATH "@executable_path/../Frameworks")
endif()

#target_compile_options(${PROJECT_NAME} PUBLIC -Wno-shift-count-overflow)

target_sources(${PROJECT_NAME} PUBLIC ${srcs} )

target_link_libraries(${PROJECT_NAME} PUBLIC guikit driver coreC64 coreAmiga logos icons)
