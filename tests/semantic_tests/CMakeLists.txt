add_library(semantic_tests_lib OBJECT
        test_semantic.cpp
)

target_link_libraries(semantic_tests_lib PUBLIC
        GTest::gtest
        lang_lib
)

target_compile_definitions(semantic_tests_lib PRIVATE
        PROG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/programs"
)