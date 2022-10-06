cmake_minimum_required(VERSION 3.22)

set(SOURCE_FILES data_structures/vector.h driver/diagnostics.c driver/diagnostics.h data_structures/map.c data_structures/map.h preprocessor/trigraphs.c preprocessor/trigraphs.h driver/file_utils.c driver/file_utils.h preprocessor/lines.h preprocessor/diagnostics.c preprocessor/diagnostics.h preprocessor/lines.h preprocessor/escaped_newlines.c preprocessor/escaped_newlines.h preprocessor/pp_token.c preprocessor/pp_token.h preprocessor/detector.h data_structures/trie.c data_structures/trie.h)

project(ick C)

set(CMAKE_C_STANDARD 99)
string(APPEND CMAKE_EXE_LINKER_FLAGS "-fsanitize=address,undefined")
add_compile_options(-Wall -Wextra -Wpedantic -Og -g)
add_executable(ick ${SOURCE_FILES} main.c)
include_directories(.)
