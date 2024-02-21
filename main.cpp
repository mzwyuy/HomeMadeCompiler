#include "IRNode.h"
#include <iostream>

int main() {
    ParseIR ir_parser;
    std::string filename = "D:\\cpp\\cpp_code\\tiny_compiler\\test.txt";
    FILE* c_file = fopen(filename.c_str(), "r");
    if (c_file) {
        std::cout << "open file successfully" << std::endl;
        ir_parser.DoLexemeInterprete(c_file, filename);
        auto gloabl_scope = ir_parser.SyntaxInpterprete();
        gloabl_scope->Display(std::cout, 0);
        fclose(c_file);
    } else {
        std::cout << "can't open file" << std::endl;
    }

    return 0;
}