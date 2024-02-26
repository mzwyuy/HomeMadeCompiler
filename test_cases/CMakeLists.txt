include_directories(../lex_and_parse)
aux_source_directory(. test_cases_src)
add_executable(test_tiny_compiler ${test_cases_src})
target_link_libraries(test_tiny_compiler PUBLIC gtest gtest_main lex_and_parse)

enable_testing()
add_test(NAME test_tiny_compiler COMMAND test_tiny_compiler)
add_dependencies(test_tiny_compiler gtest gtest_main lex_and_parse)