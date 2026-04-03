
add_library(guikit STATIC)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(guikit PUBLIC GUIKIT_WINAPI)

    set(headers
        winapi/widgets/button.cpp
        winapi/widgets/checkbox.cpp
        winapi/widgets/checkbutton.cpp
        winapi/widgets/combobutton.cpp
        winapi/widgets/frame.cpp
        winapi/widgets/hyperlink.cpp
        winapi/widgets/imageView.cpp
        winapi/widgets/label.cpp
        winapi/widgets/lineedit.cpp
        winapi/widgets/listview.cpp
        winapi/widgets/multilineedit.cpp
        winapi/widgets/progressbar.cpp
        winapi/widgets/radiobox.cpp
        winapi/widgets/slider.cpp
        winapi/widgets/squareCanvas.cpp
        winapi/widgets/stepbutton.cpp
        winapi/widgets/tabframe.cpp
        winapi/widgets/treeview.cpp
        winapi/widgets/viewport.cpp
        winapi/widgets/logicViewer.cpp
        winapi/widgets/widget.cpp
        winapi/browserWindow.cpp
        winapi/clipboard.cpp
        winapi/dirent.h
        winapi/display.cpp
        winapi/dragndrop.cpp
        winapi/interProcess.cpp
        winapi/main.cpp
        winapi/main.h
        winapi/menu.cpp
        winapi/messageWindow.cpp
        winapi/processref.h
        winapi/statusbar.cpp
        winapi/tools.cpp
        winapi/dark.cpp
    )
else()
    target_compile_definitions(guikit PUBLIC GUIKIT_GTK)

    SET_SOURCE_FILES_PROPERTIES(api.cpp PROPERTIES INCLUDE_DIRECTORIES "${GTK_INCLUDE_DIRS}")

    set(headers
        gtk/widgets/button.cpp
        gtk/widgets/checkbox.cpp
        gtk/widgets/checkbutton.cpp
        gtk/widgets/combobutton.cpp
        gtk/widgets/frame.cpp
        gtk/widgets/hyperlink.cpp
        gtk/widgets/imageView.cpp
        gtk/widgets/label.cpp
        gtk/widgets/lineedit.cpp
        gtk/widgets/listview.cpp
        gtk/widgets/multilineedit.cpp
        gtk/widgets/progressbar.cpp
        gtk/widgets/radiobox.cpp
        gtk/widgets/slider.cpp
        gtk/widgets/squareCanvas.cpp
        gtk/widgets/stepbutton.cpp
        gtk/widgets/tabframe.cpp
        gtk/widgets/treeview.cpp
        gtk/widgets/viewport.cpp
        gtk/widgets/widget.cpp
        gtk/browserWindow.cpp
        gtk/display.cpp
        gtk/interProcess.cpp
        gtk/main.cpp
        gtk/main.h
        gtk/menu.cpp
        gtk/messageWindow.cpp
        gtk/statusbar.cpp
        gtk/tools.cpp
    )
endif()

set(headers ${headers}
    api.h
    tools/crc32.h
    tools/image.cpp
    tools/layout.cpp
    tools/setting.cpp
    tools/string.cpp
    tools/translate.cpp
    tools/ttf.cpp
    tools/utf8.cpp
    tools/decode/gzip.h
    tools/decode/images.h
    tools/decode/inflate.h
    tools/decode/lha.h
    tools/decode/png.h
    tools/decode/zip.h
    tools/encode/png.h
    tools/file/chunk.h
    tools/file/directory.h
    tools/file/file.cpp
    tools/file/tar.h
)

SET_SOURCE_FILES_PROPERTIES(tools/decode/lha.cpp PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/lha")

SET_SOURCE_FILES_PROPERTIES(tools/decode/images.cpp PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/stb")
SET_SOURCE_FILES_PROPERTIES(tools/decode/images.cpp PROPERTIES COMPILE_FLAGS "-Wno-stringop-overflow")

SET_SOURCE_FILES_PROPERTIES(tools/encode/images.cpp PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/stb;${CMAKE_SOURCE_DIR}/deps/cgif")

target_sources(guikit PRIVATE
    tools/decode/lha.cpp
    tools/decode/images.cpp
    tools/encode/images.cpp
    api.cpp
    ${headers}
)

set_source_files_properties(guikit ${headers} PROPERTIES HEADER_FILE_ONLY TRUE)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(guikit PRIVATE
        ws2_32 gdi32 comdlg32 ole32 comctl32 shlwapi msimg32 winmm dwmapi uxtheme gdiplus)

    set(guikit.resource data/resource_mingw.rc PARENT_SCOPE)
else()
    target_link_libraries(guikit PRIVATE fontconfig Xrandr)

    target_link_directories(guikit PUBLIC ${GTK_LIBRARY_DIRS})
    target_link_libraries(guikit PRIVATE ${GTK_LIBRARIES})

    set(guikit.resource)
endif()

# [Windows only] add guikit.resource to executable: add_executable( APP ${srcs} ${guikit.resource} )
# link guikit as library: target_link_libraries( APP guikit )
