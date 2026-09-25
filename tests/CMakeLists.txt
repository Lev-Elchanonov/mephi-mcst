find_package(GTest REQUIRED)


add_subdirectory(graph_tests)
add_subdirectory(parlex_tests)


add_executable(tests
        test_main.cpp     # единственный main()
)

target_link_libraries(tests PRIVATE
        GTest::gtest
        lang_lib
        $<TARGET_OBJECTS:graph_tests_lib>
        $<TARGET_OBJECTS:parlex_tests_lib>
)

target_link_libraries(tests PRIVATE GTest::gtest)

add_test(NAME tests COMMAND tests)