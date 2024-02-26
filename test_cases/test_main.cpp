#include <gtest/gtest.h>
#include "IRNode.h"

class MyTest : public testing::Test {
protected:
    // 在每个测试用例执行前设置测试环境
    void SetUp() override {
        // 在这里执行一些准备工作，比如初始化资源
        // 这里我们简单地输出一条消息表示设置完成
        std::cout << "Setting up test environment..." << std::endl;
    }

    // 在每个测试用例执行后清理测试环境
    void TearDown() override {
        // 在这里执行一些清理工作，比如释放资源
        // 这里我们简单地输出一条消息表示清理完成
        std::cout << "Tearing down test environment..." << std::endl;
    }
};

// 测试用例
TEST_F(MyTest, Test1) {
    std::cout << "Test1 passed!" << std::endl;

    ParseIR ir_parser;
    std::string filename = "D:\\cpp\\cpp_code\\tiny_compiler\\test_cases\\test.txt";
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
}

TEST_F(MyTest, Test2) {
// 在这里编写另一个测试代码
// 这里我们简单地输出一条消息表示测试通过
std::cout << "Test2 passed!" << std::endl;
}

int main(int argc, char** argv) {
    // 初始化 Google Test 框架
    testing::InitGoogleTest(&argc, argv);
    // 运行所有测试用例
    return RUN_ALL_TESTS();
}