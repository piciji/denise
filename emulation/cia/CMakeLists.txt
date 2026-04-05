
add_library(cia OBJECT)

set(headers
        ../cia/base.h
        ../cia/m6526.h
        ../cia/m8520.h
        ../cia/register.cpp
)

set_source_files_properties(cia ${headers}
        PROPERTIES HEADER_FILE_ONLY TRUE)

target_sources(cia PRIVATE
        base.cpp
        m6526.cpp
        ${headers}
)

add_library(ciaNew OBJECT)

set(headers
        new/cia.h
        new/defines.h
        new/register.cpp
        new/tod.cpp
)

set_source_files_properties(ciaNew ${headers}
        PROPERTIES HEADER_FILE_ONLY TRUE)

target_sources(ciaNew PRIVATE
        new/cia.cpp
        ${headers}
)
