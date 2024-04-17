cmake_minimum_required(VERSION 3.22)

set(SOURCE_FILES
        driver/file_utils.c driver/file_utils.h driver/diagnostics.c driver/diagnostics.h
        data_structures/trie.c data_structures/trie.h
        debug/reminder.c debug/reminder.h debug/malloc.c debug/malloc.h
        data_structures/vector.h data_structures/map.h  data_structures/result.c data_structures/result.h
        preprocessor/parser.h preprocessor/trigraphs.c preprocessor/trigraphs.h preprocessor/sized_str.h preprocessor/diagnostics.c preprocessor/diagnostics.h preprocessor/escaped_newlines.c preprocessor/escaped_newlines.h preprocessor/pp_token.c preprocessor/pp_token.h preprocessor/detector.h
        preprocessor/parser.c
        debug/color_print.c
        debug/color_print.h
        preprocessor/macro_expansion.c
        preprocessor/macro_expansion.h
        preprocessor/token_rule_definitions.c
        preprocessor/sized_str.c
        preprocessor/conditional_inclusion.c
        preprocessor/conditional_inclusion.h
        preprocessor/char_parser.c
        preprocessor/char_parser.h
        preprocessor/char_rule_definitions.c
        mappings/typedefs.h
)

project(ick C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
string(APPEND CMAKE_EXE_LINKER_FLAGS "-fsanitize=address,undefined")
add_compile_options(-Weverything -Wno-padded -Wno-declaration-after-statement -Wno-missing-noreturn -Wno-documentation-unknown-command -Wno-unsafe-buffer-usage -Wno-used-but-marked-unused -Wno-switch-default -O0 -g)
#add_compile_definitions(DEBUG)
add_executable(ick ${SOURCE_FILES} main.c)
include_directories(.)
