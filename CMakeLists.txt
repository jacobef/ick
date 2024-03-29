cmake_minimum_required(VERSION 3.22)

set(SOURCE_FILES
        driver/file_utils.c driver/file_utils.h driver/diagnostics.c driver/diagnostics.h
        data_structures/trie.c data_structures/trie.h
        debug/reminder.c debug/reminder.h debug/malloc.c debug/malloc.h
        data_structures/vector.h  data_structures/map.c data_structures/map.h  data_structures/result.c data_structures/result.h
        preprocessor/parser.h preprocessor/trigraphs.c preprocessor/trigraphs.h preprocessor/lines.h preprocessor/diagnostics.c preprocessor/diagnostics.h preprocessor/escaped_newlines.c preprocessor/escaped_newlines.h preprocessor/pp_token.c preprocessor/pp_token.h preprocessor/detector.h)

project(ick C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
string(APPEND CMAKE_EXE_LINKER_FLAGS "-fsanitize=address,undefined")
add_compile_options(-Weverything -Wno-padded -Wno-declaration-after-statement -Wno-missing-noreturn -Wno-documentation-unknown-command -Wno-unsafe-buffer-usage -Wno-used-but-marked-unused -O0 -g)
#add_compile_definitions(DEBUG)
add_executable(ick ${SOURCE_FILES} main.c)
include_directories(.)
