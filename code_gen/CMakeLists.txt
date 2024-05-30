file(GLOB code_gen_src *.cpp)  #aux_source_directory(. code_gen_src)
add_library(code_gen ${code_gen_src})
target_include_directories(code_gen PUBLIC ../lex_and_parse)