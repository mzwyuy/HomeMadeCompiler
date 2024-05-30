//
// Created by tom_cat233 on 25/5/2024.
//
#include "QuadCodeGen.h"

void QuadCodeEmitter::EmitStmt(IRStmt* stmt) {
    QuadInst* inst = nullptr;
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
            quad_insts_.push_back(inst);  // todo: need new?
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
        case IR_ExprStmt:
            EmitExpr(((IRExprStmt *) stmt)->expr_);
        case IR_IfElse:

        case IR_For:
        case IR_While:
        case IR_Break:
        case IR_Continue:
        case IR_Return:
        case IR_Null:
            return;
        default:
            break;
    }
}
void QuadCodeEmitter::EmitExpr(IRExpr* expr) {

}