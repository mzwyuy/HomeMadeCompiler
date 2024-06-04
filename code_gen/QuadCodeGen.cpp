//
// Created by tom_cat233 on 25/5/2024.
//
#include "QuadCodeGen.h"

void QuadCodeEmitter::EmitStmt(IRStmt* stmt) {
    QuadInst* inst = nullptr;
    if (stmt == nullptr) {
        return;
    }
    switch (stmt->ClassId()) {
        /*    IR_Stmt, IR_CodeBlock, IR_Func, IR_VarDecl, IR_FuncDecl, IR_ExprStmt, IR_IfElse, IR_For,
    IR_While, IR_Break, IR_Continue, IR_Return, IR_Null,*/
        case IR_Stmt:
            assert(0);
            break;
        case IR_CodeBlock: {
            bool is_cur_block_global = is_cur_block_global_;
            is_cur_block_global_ = ((IRCodeBlock *) stmt)->is_global_;
            for (auto i: ((IRCodeBlock *) stmt)->stmts_) {
                EmitStmt(i);
            }
            is_cur_block_global_ = is_cur_block_global;
            break;
        }
        case IR_Func:
            inst = new QuadInst(OP_LABEL, label_count_++);
            quad_insts_.push_back(inst);
            inst = new QuadInst(OP_PUSH, IRReg::GetReg(IRReg::rbp));
            quad_insts_.push_back(inst);
            inst = new QuadInst(OP_ASSIGN, IRReg::GetReg(IRReg::rbp), IRReg::GetReg(IRReg::rsp));
            quad_insts_.push_back(inst);
            for (auto i : ((IRCodeBlock*)stmt)->stmts_) {
                EmitStmt(i);
            }
            break;
        case IR_VarDecl: {
            auto vars = ((IRVarDecl *) stmt)->vars_;
            if (is_cur_block_global_) {
                for (auto i : vars) {
                    // allocate memory space for global vars
                    inst = new QuadInst(OP_DEC, i, nullptr, nullptr);
                    quad_insts_.push_back(inst);
                }
            }
            break;
        }
        case IR_FuncDecl:
            // todo: not emit quadruple code, maybe remove IRFuncDecl?
            break;
        case IR_ExprStmt:
            EmitExpr(((IRExprStmt *) stmt)->expr_);
            break;
        case IR_IfElse: {
            auto condition = ((IRIfElse *) stmt)->condition_;
            assert(condition);

            auto then_stmt = ((IRIfElse *) stmt)->then_stmt_;
            assert(then_stmt);
            auto else_stmt = ((IRIfElse *) stmt)->else_stmt_;
            unsigned inst_cnt = 0;

            if (condition->ClassId() == IR_Binary) {
                auto op = ((IRBinary*)condition)->op_;
                if (op == KW_NOT || op == KW_AND || op == KW_OR) {  // logical operators
                    std::vector<QuadInst **> jmp_true;
                    std::vector<QuadInst **> jmp_false;
                    EmitBoolean(condition, jmp_true, jmp_false);

                    auto then_stmt = ((IRIfElse *) stmt)->then_stmt_;
                    assert(then_stmt);
                    inst_cnt = quad_insts_.size();
                    EmitStmt(then_stmt);
                    inst = quad_insts_[inst_cnt];
                    for (auto i : jmp_true) {
                        *i = inst;
                    }

                    inst_cnt = quad_insts_.size();
                    if (else_stmt) {
                        EmitStmt(else_stmt);
                        inst = quad_insts_[inst_cnt];
                    } else {
                        inst = new QuadInst(OP_LABEL, label_count_++);
                        quad_insts_.push_back(inst);
                    }
                    for (auto i: jmp_false) {
                        *i = inst;
                    }
                    break;
                }
            }
            // todo: change comparison binary operators condition to je, jne, jg, jge, jl or jle to improve perf
            EmitExpr(condition);
            inst = new QuadInst(OP_CMP, last_expr_, last_expr_);
            quad_insts_.push_back(inst);
            inst = new QuadInst(OP_JE, (QuadInst*)nullptr);
            quad_insts_.push_back(inst);
            EmitStmt(then_stmt);

            inst_cnt = quad_insts_.size();
            if (else_stmt) {
                EmitStmt(else_stmt);
            }
            inst->jmp_target_ = quad_insts_[inst_cnt];

            break;
        }
        case IR_For:
            // todo: not support now
            assert(0);
            break;
        case IR_While: {
            /* if (temp) {        label0
             *     stmt0          calc temp
             * }                  test temp, temp
             * stmt1              jz(je) label1
             * =>                 stmt0
             *                    jmp label0
             *                    lable1
             *                    stmt1
             * */
            QuadInst *label_inst = new QuadInst(OP_LABEL, label_count_++);
            quad_insts_.push_back(label_inst);
            EmitExpr(((IRWhile *) stmt)->condition_);
            assert(last_expr_);
            inst = new QuadInst(OP_TEST, last_expr_, last_expr_);
            quad_insts_.push_back(inst);
            QuadInst *jmp_inst = new QuadInst(OP_JE, (QuadInst *) nullptr);

            EmitStmt(((IRWhile *) stmt)->while_body_);
            inst = new QuadInst(OP_JMP, label_inst);
            quad_insts_.push_back(inst);
            inst = new QuadInst(OP_LABEL, label_count_++);
            quad_insts_.push_back(inst);
            jmp_inst->jmp_target_ = inst;

            for (auto i : breaks_) {
                i->jmp_target_ = inst;
            }
            break;
        }
        case IR_Break:
            inst = new QuadInst(OP_JMP, (QuadInst*)nullptr);
            quad_insts_.push_back(inst);
            breaks_.push_back(inst);
            break;
        case IR_Continue:
            inst = new QuadInst(OP_JMP, loop_begin_);
            quad_insts_.push_back(inst);
            break;
        case IR_Return:
            if (((IRReturn*)stmt)->is_last_return_) {
                EmitExpr(((IRReturn*)stmt)->ret_val_, IRReg::GetReg(IRReg::rax));
                inst = new QuadInst(OP_LABEL, label_count_++);

                for (auto i: returns_) {
                    i->jmp_target_ = inst;
                }
                returns_.clear();

                quad_insts_.push_back(inst);
                inst = new QuadInst(OP_POP, IRReg::GetReg(IRReg::rbp));
                quad_insts_.push_back(inst);
                inst = new QuadInst(OP_RET, (unsigned)0);
                quad_insts_.push_back(inst);
            } else {
                EmitExpr(((IRReturn*)stmt)->ret_val_, IRReg::GetReg(IRReg::rax));
                inst = new QuadInst(OP_JMP, (QuadInst*)nullptr);
                quad_insts_.push_back(inst);
                returns_.push_back(inst);
            }
            break;
        case IR_Null:
            inst = new QuadInst(OP_NOP, (unsigned )0);
            quad_insts_.push_back(inst);
            return;
        default:
            break;
    }
}
void QuadCodeEmitter::EmitExpr(IRExpr* expr, IRTemp* result_storer) {

}
void QuadCodeEmitter::EmitBoolean(IRExpr *expr, std::vector<QuadInst **> &jmp_true,
                                  std::vector<QuadInst **> &jmp_false) {

}