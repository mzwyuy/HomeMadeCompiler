file(GLOB lex_and_parse_src *.cpp) #aux_source_directory(. lex_and_parse_src)
add_library (lex_and_parse ${lex_and_parse_src})