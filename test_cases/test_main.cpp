#include <gtest/gtest.h>
#include "IRNode.h"

class MyTest : public testing::Test {
protected:
    // 在每个测试用例执行前设置测试环境
    void SetUp() override {}

    // 在每个测试用例执行后清理测试环境
    void TearDown() override {}
};

void ParseAndTestFile(const char *file_name) {
    ParseIR ir_parser;
    char path[255];
    getcwd(path, 255);
    std::string absolute_file_name = path;
    absolute_file_name.erase(absolute_file_name.size() - 3);   // remove bin
    absolute_file_name += "test_cases\\";  // .../bin/../test_cases
    absolute_file_name += file_name;

    FILE *c_file = fopen(absolute_file_name.c_str(), "r");
    if (c_file) {
        std::cout << "open file successfully" << std::endl;
        ir_parser.DoLexemeInterprete(c_file, absolute_file_name);
        auto gloabl_scope = ir_parser.SyntaxInpterprete();
        gloabl_scope->Display(std::cout, 0);
        fclose(c_file);
    } else {
        std::cout << "can't open file:" << std::endl;
        std::cout << absolute_file_name << std::endl;
    }
}

// 测试用例
TEST_F(MyTest, Test1) {
    ParseAndTestFile("simple_test0.txt");
}

TEST_F(MyTest, RomanToInt) {
    ParseAndTestFile("RomanToInt.txt");
}

TEST_F(MyTest, ReverseInt) {
    ParseAndTestFile("ReverseInt.txt");
}

TEST_F(MyTest, BitwiseCalculation) {
    ParseAndTestFile("BitwiseCalculation.txt");
}

TEST_F(MyTest, Test5) {
    // ParseAndTestFile("ReverseList.txt");
    // todo: need to support class
}


int main(int argc, char** argv) {
    // 初始化 Google Test 框架
    testing::InitGoogleTest(&argc, argv);
    // 运行所有测试用例
    return RUN_ALL_TESTS();
}