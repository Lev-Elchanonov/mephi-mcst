add_library(parlex_tests_lib OBJECT
        test_parser.cpp
)

target_link_libraries(parlex_tests_lib PUBLIC
        GTest::gtest
        lang_lib
)

target_compile_definitions(parlex_tests_lib PRIVATE
        LEXERR_DIR="${CMAKE_CURRENT_SOURCE_DIR}/programs/lexerr_programs"
        SYNERR_DIR="${CMAKE_CURRENT_SOURCE_DIR}/programs/synerr_programs"
        VALID_DIR="${CMAKE_CURRENT_SOURCE_DIR}/programs/valid_programs"
)