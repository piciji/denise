
add_library(icons STATIC)

set(headers icons.h)

set_source_files_properties(icons ${headers}
        PROPERTIES HEADER_FILE_ONLY TRUE)

target_sources(icons PRIVATE
        icons/icons.cpp
        ${headers}
)


add_library(logos STATIC)

set(headers logos.h)

set_source_files_properties(logos ${headers}
        PROPERTIES HEADER_FILE_ONLY TRUE)

target_sources(logos PRIVATE
        icons/logos.cpp
        ${headers}
)