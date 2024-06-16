//
// Created by tom_cat233 on 25/5/2024.
//
#pragma once
#include "IRNode.h"

enum InstOperator {
    OP_NOP,
    OP_LABEL,
    OP_DEC,  //declaration
    OP_ENTRY, OP_EXIT,  // function's entry and exit
    OP_ASSIGN,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    // ! - & *
    OP_NOT, OP_NEG, OP_LEA, OP_DREF,

    OP_GT, OP_GE, OP_LT, OP_LE, OP_EQU, OP_NE,

    OP_AND, OP_OR,

    OP_LDREF, OP_RDREF,  //*x = y, x = *y
    OP_CMP,
    OP_TEST,  // bitwise AND operator
    OP_SETE, OP_SETNE,  // == 0, ！= 0
    OP_JMP,  // unconditional jump
    OP_JE, OP_JNE, OP_JG, OP_JGE, OP_JL, OP_JLE,  // conditional jump
    OP_ARG,
    OP_PROC,  // func();
    OP_CALL,  // x = func();
    OP_RET,  // return;
    OP_PUSH,
    OP_POP,
};

class QuadInst {
public:
    QuadInst(InstOperator op, IRTemp* result, IRAsmOperand* left, IRAsmOperand* right) : op_(op), result_(result), left_(left), right_(right) {
        // if test, left and right must not be immediate
    }
    // comparison operators, result is in flag registers
    QuadInst(InstOperator op, IRFunc* func) : op_(op), func_(func) {}
    QuadInst(InstOperator op, QuadInst* jmp_target) : op_(op), jmp_target_(jmp_target) {}
    QuadInst(InstOperator op, unsigned label) : op_(op), label_(label) {}
    void * operator new (std::size_t count) {
        if (count == 0)
            ++count; // avoid std::malloc(0) which may return nullptr on success

        if (void *ptr = std::malloc(count)) {
            quad_insts_.push_back((QuadInst*)ptr);
            return ptr;
        } else {
            return nullptr;
        }
    }

    InstOperator op_;
    // todo: test $0, $0 is prohibited
    union {
        struct {
            IRTemp *result_;
            IRAsmOperand *left_;
            IRAsmOperand *right_;
        };
        IRFunc *func_;
        QuadInst *jmp_target_;
        unsigned label_;
    };
    static std::vector<QuadInst*> quad_insts_;
};

class QuadCodeEmitter {
public:
    QuadCodeEmitter(IRCodeBlock* block) : global_block_(block) {}
    void EmitStmt(IRStmt* stmt);
    void EmitFunc(IRFunc* func);
    IRAsmOperand *EmitExpr(IRExpr *expr, IRTemp *result_storer = nullptr, std::vector<QuadInst *> *jmp_true = nullptr,
                           std::vector<QuadInst *> *jmp_false = nullptr);
    void EmitGlobalVars();  // static var or global vars
    // local vars cache important temp result, such as frequently-used results
    // first, vars chose by users are viewed as local vars, but these may be used once and are most likely used only in registers
    // second, frequently-used results are viewed as local vars
    void EmitLocalVars();
private:
    bool is_cur_block_global_ = false;
    bool is_logic_operators_ = false;  // Is this expr level a logic operator (&&, || or !)? IF so, don't need to convert it to boolean.
    unsigned label_count_ = 0;
    // IRTemp* last_expr_ = nullptr; maybe not used
    IRCodeBlock* global_block_ = nullptr;
    IRScope* cur_scope_ = nullptr;
    QuadInst* loop_begin_ = nullptr;
    std::vector<QuadInst*> quad_insts_{};
    std::vector<QuadInst*> breaks_{};  // at the end of loop, backfill jmp point of breaks
    std::vector<QuadInst*> returns_{};
    std::vector<IRAsmOperand*> temps_;
};
