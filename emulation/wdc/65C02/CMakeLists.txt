
add_library(cpu65c02 STATIC)

set(headers
        w65C02.h
        alu.cpp
        instructions.cpp
)

set_source_files_properties(cpu65c02 ${headers}
        PROPERTIES HEADER_FILE_ONLY TRUE)

target_sources(cpu65c02 PRIVATE
        w65C02.cpp
        ${headers}
)
