
add_library(cpu65816 STATIC)

set(headers
        w65816.h
        optable.h
        dasmHandler.h
        alu.cpp
        instructions.cpp
)

set_source_files_properties(cpu65816 ${headers}
        PROPERTIES HEADER_FILE_ONLY TRUE)

target_sources(cpu65816 PRIVATE
        w65816.cpp
        dasmHandler.cpp
        ${headers}
)
