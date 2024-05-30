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
    OP_NOT, OP_NEGATE, OP_LEA, OP_DREF,

    OP_GT, OP_GE, OP_LT, OP_LE, OP_EQU, OP_NE,

    OP_AND, OP_OR,

    OP_LDREF, OP_RDREF,  //*x = y, x = *y
    OP_JMP,  // conditional jump
    OP_JEQU, OP_JNEQU, OP_JNE, // unconditional jump
    OP_ARG,
    OP_PROC,  // func();
    OP_CALL,  // x = func();
    OP_RET,  // return;
    OP_RETV  // return x;
};

class QuadInst {
public:
    QuadInst(InstOperator op, IRVar* result, IRVar* left, IRVar* right = nullptr) : op_(op), result_(result), left_(left), right_(right) {}
    QuadInst(InstOperator op, IRFunc* func) : op_(op), func_(func) {}
    QuadInst(InstOperator op, QuadInst* jmp_target) : op_(op), jmp_target_(jmp_target) {}
    QuadInst(InstOperator op, unsigned label) : op_(op), label_(label) {}

    InstOperator op_;
    union {
        struct {
            IRVar *result_;
            IRVar *left_;
            IRVar *right_;
        };
        IRFunc *func_;
        QuadInst *jmp_target_;
        unsigned label_;
    };
};

class QuadCodeEmitter {
public:
    QuadCodeEmitter(IRCodeBlock* block) : global_block_(block) {}
    void EmitStmt(IRStmt* stmt);
    void EmitFunc(IRFunc* func);
    void EmitExpr(IRExpr* expr);
    void EmitGlobalVars();  // static var or glabal vars
    // local vars cache important temp result, such as frequently-used results
    // first, vars chose by users are viewed as local vars, but these may be used once and are most likely used only in registers
    // second, frequently-used results are viewed as local vars
    void EmitLocalVars();
private:
    bool is_cur_block_global_ = false;
    unsigned label_count_ = 0;
    IRCodeBlock* global_block_ = nullptr;
    std::vector<QuadInst *> quad_insts_{};
    std::vector<IRTemp*> temps_;
};
