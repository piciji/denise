cmake_minimum_required(VERSION 3.14)

project(denise LANGUAGES CXX)

set(AppName "Denise")
set(translationFolder "translation")
set(dataFolder "data")
set(fontFolder "fonts")
set(presetFolder "presets")
set(imagesFolder "images")
set(soundFolder "sounds")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_NO_SYSTEM_FROM_IMPORTED YES)

set(ltoSupported 0)

if(DEFINED ENABLE_LTO AND ENABLE_LTO AND NOT CMAKE_SIZEOF_VOID_P EQUAL 4) # no 32 bit support
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ltoSupported)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES ${CMAKE_HOST_SYSTEM_PROCESSOR})
    endif()

    if (${CMAKE_OSX_ARCHITECTURES} STREQUAL "arm64")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11")
        set(macArchFolder "macArm")
    else()
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13")
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

set(APP_INCLUDE "${CMAKE_SOURCE_DIR}/data/libs/include")

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(CMAKE_GENERATOR STREQUAL "MSYS Makefiles")
        find_package(Freetype)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(FREETYPE2 freetype2)
        pkg_check_modules(LIBUSB libusb-1.0)
        pkg_check_modules(LIBLAME lame)

        if(FREETYPE2_FOUND)
            set(FREETYPE2_LIBRARIES ${FREETYPE2_STATIC_LIBRARIES})
            set(FREETYPE2_LIBRARY_DIRS)
        endif()

        if(LIBUSB_FOUND)
            set(LIBUSB_LIBRARIES ${LIBUSB_STATIC_LIBRARIES})
            set(LIBUSB_LIBRARY_DIRS)
        endif()

        if(LIBLAME_FOUND)
            set(LIBLAME_LIBRARIES ${LIBLAME_STATIC_LIBRARIES})
            set(LIBLAME_LIBRARY_DIRS)
        endif()
    else() # MinGW
        set(FREETYPE2_FOUND true)
        set(FREETYPE2_INCLUDE_DIRS "${APP_INCLUDE}")
        set(FREETYPE2_LIBRARIES)
        set(FREETYPE2_LIBRARY_DIRS)
        set(_libs libfreetype.a libpng16.a libz.a libbz2.a libbrotlidec.a libbrotlicommon.a)
        foreach(_lib IN LISTS _libs)
            set(FREETYPE2_LIBRARIES ${FREETYPE2_LIBRARIES} ${CMAKE_SOURCE_DIR}/data/libs/static/${architecture}/${_lib})
        endforeach()

        set(LIBUSB_FOUND true)
        set(LIBUSB_INCLUDE_DIRS "${APP_INCLUDE}")
        set(LIBUSB_LIBRARIES "${CMAKE_SOURCE_DIR}/data/libs/static/${architecture}/libusb-1.0.a")
        set(LIBUSB_LIBRARY_DIRS)

        set(LIBLAME_FOUND true)
        set(LIBLAME_INCLUDE_DIRS "${APP_INCLUDE}")
        set(LIBLAME_LIBRARIES "${CMAKE_SOURCE_DIR}/data/libs/static/${architecture}/libmp3lame.a")
        set(LIBLAME_LIBRARY_DIRS)
    endif()

elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    find_package(OpenAL)

    set(FREETYPE2_FOUND true)
    set(FREETYPE2_INCLUDE_DIRS "${APP_INCLUDE}")
    set(FREETYPE2_LIBRARIES)
    set(FREETYPE2_LIBRARY_DIRS)
    set(_libs libbrotlicommon.1.dylib libbrotlidec.1.dylib libbz2.1.0.dylib libpng16.16.dylib libz.1.dylib libfreetype.6.dylib)
    foreach(_lib IN LISTS _libs)
        set(FREETYPE2_LIBRARIES ${FREETYPE2_LIBRARIES} ${CMAKE_SOURCE_DIR}/data/libs/shared/${macArchFolder}/${_lib})
    endforeach()

    set(LIBUSB_FOUND true)
    set(LIBUSB_INCLUDE_DIRS "${APP_INCLUDE}")
    set(LIBUSB_LIBRARIES "${CMAKE_SOURCE_DIR}/data/libs/shared/${macArchFolder}/libusb-1.0.0.dylib")
    set(LIBUSB_LIBRARY_DIRS)

    set(LIBLAME_FOUND true)
    set(LIBLAME_INCLUDE_DIRS "${APP_INCLUDE}")
    set(LIBLAME_LIBRARIES "${CMAKE_SOURCE_DIR}/data/libs/shared/${macArchFolder}/libmp3lame.0.dylib")
    set(LIBLAME_LIBRARY_DIRS)
else()
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

    if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD") # libusb_dev_mem_ stuff undeclared
        set(LIBUSB_FOUND false)
    endif()
endif()

add_subdirectory(data)
add_subdirectory(deps)
add_subdirectory(guikit)
add_subdirectory(driver)
add_subdirectory(emulation/cia)
add_subdirectory(emulation/wdc/65C02)
add_subdirectory(emulation/wdc/65816)
add_subdirectory(emulation/libc64)
add_subdirectory(emulation/libami)
add_subdirectory(program)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(DIRECTORY data/${translationFolder} DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(DIRECTORY data/${dataFolder} DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(DIRECTORY data/${fontFolder} DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.ttf")
    install(DIRECTORY data/${soundFolder} DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(DIRECTORY data/${presetFolder} DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(DIRECTORY data/${imagesFolder} DESTINATION ${CMAKE_INSTALL_PREFIX})

    install(FILES readme.md DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(FILES licence.md DESTINATION ${CMAKE_INSTALL_PREFIX})

elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    install(DIRECTORY data/${translationFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)
    install(DIRECTORY data/${dataFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)
    install(DIRECTORY data/${fontFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources FILES_MATCHING PATTERN "*.ttf")
    install(DIRECTORY data/${presetFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)
    install(DIRECTORY data/${imagesFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)
    install(DIRECTORY data/${soundFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)

    install(FILES data/Info.plist DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents)
    install(FILES licence.md DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)
    install(FILES readme.md DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)
    install(FILES data/img/${PROJECT_NAME}.icns DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources)

    install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/MacOS)

    if(LIBUSB_FOUND)
        install(FILES ${LIBUSB_LIBRARIES} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Frameworks)
    endif()

    if(LIBLAME_FOUND)
        install(FILES ${LIBLAME_LIBRARIES} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Frameworks)
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
    install(DIRECTORY data/${presetFolder} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
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
