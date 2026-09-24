find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)



flex_target(Lexer
        src/lang_lexer.l
        ${CMAKE_CURRENT_BINARY_DIR}/lang_lexer.cpp
)

bison_target(Parser
        src/lang_parser.y
        ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cpp
        DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.hpp
)


add_flex_bison_dependency(Lexer Parser)


add_executable(lang
        main.cpp
        ${FLEX_Lexer_OUTPUTS}
        ${BISON_Parser_OUTPUTS}
)


target_include_directories(lang PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}
)


# Генерированный flex/bison код часто имеет неиспользуемые функции — глушим warning
# target_compile_options(lang PRIVATE -Wno-unused-function)