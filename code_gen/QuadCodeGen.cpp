//
// Created by tom_cat233 on 25/5/2024.
//
#include "QuadCodeGen.h"
#include <array>

std::vector<QuadInst *> QuadInst::quad_insts_{};

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

            IRScope* cur_scope = cur_scope_;
            cur_scope_ = (IRCodeBlock*)stmt;
            for (auto i: ((IRCodeBlock *) stmt)->stmts_) {
                EmitStmt(i);
            }
            cur_scope_ = cur_scope;

            is_cur_block_global_ = is_cur_block_global;
            break;
        }
        case IR_Func: {
            inst = new QuadInst(OP_LABEL, label_count_++);
            inst = new QuadInst(OP_PUSH, IRReg::GetReg(IRReg::rbp), nullptr, nullptr);
            inst = new QuadInst(OP_ASSIGN, IRReg::GetReg(IRReg::rbp), IRReg::GetReg(IRReg::rsp), nullptr);

            IRScope* cur_scope = cur_scope_;
            cur_scope_ = (IRFunc*)stmt;
            for (auto i: ((IRCodeBlock *) stmt)->stmts_) {
                EmitStmt(i);
            }
            cur_scope_ = cur_scope;

            break;
        }
        case IR_VarDecl: {
            auto vars = ((IRVarDecl *) stmt)->vars_;
            if (is_cur_block_global_) {
                for (auto i : vars) {
                    // allocate memory space for global vars
                    inst = new QuadInst(OP_DEC, i, nullptr, nullptr);
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

            std::vector<QuadInst *> jmp_true;
            std::vector<QuadInst *> jmp_false;
            IRAsmOperand* last_expr = EmitExpr(condition, nullptr, &jmp_true, &jmp_false);
            if (is_logic_operators_) {
                EmitStmt(then_stmt);
                inst = new QuadInst(OP_LABEL, label_count_++);
                for (auto i : jmp_false) {
                    i->jmp_target_ = inst;
                }
                EmitStmt(else_stmt);
            } else {
                inst = new QuadInst(OP_TEST, nullptr, last_expr, last_expr);
                auto jmp_inst = new QuadInst(OP_JE, (QuadInst *) nullptr);
                EmitStmt(then_stmt);
                inst = new QuadInst(OP_LABEL, label_count_++);
                jmp_inst->jmp_target_ = inst;
                EmitStmt(else_stmt);
            }

            break;
        }
        case IR_For: {
            IRFor* ir_for = (IRFor*)stmt;
            breaks_.emplace(std::vector<QuadInst*>{});
            continues_.emplace(std::vector<QuadInst*>{});
            EmitStmt(ir_for->init_);
            std::vector<QuadInst*> jmp_true{}, jmp_false{};
            QuadInst* loop_begin = new QuadInst(OP_LABEL, label_count_++);

            auto condition = EmitExpr(ir_for->condition_, nullptr, &jmp_true, &jmp_false);
            if (!is_logic_operators_) {
                inst = new QuadInst(OP_TEST, nullptr, condition, condition);
                inst = new QuadInst(OP_JE, (QuadInst*) nullptr);
                jmp_false.push_back(inst);
            }
            EmitStmt(ir_for->for_body_);
            inst = new QuadInst(OP_LABEL, label_count_++);
            auto& continues = continues_.top();
            for (auto i : continues) {
                i->jmp_target_ = inst;
            }
            EmitStmt(ir_for->iteration_);

            inst = new QuadInst(OP_JMP, loop_begin);

            inst = new QuadInst(OP_LABEL, label_count_++);
            auto& breaks = breaks_.top();
            for (auto i : breaks) {
                i->jmp_target_ = inst;
            }
            for (auto i : jmp_false) {
                i->jmp_target_ = inst;
            }
            breaks_.pop();
            continues_.pop();

            break;
        }
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
            std::vector<QuadInst*> jmp_true{}, jmp_false{};
            breaks_.emplace(std::vector<QuadInst*>{});
            continues_.emplace(std::vector<QuadInst*>{});

            auto condition = EmitExpr(((IRWhile *) stmt)->condition_, nullptr, &jmp_true, &jmp_false);
            if (!is_logic_operators_) {
                inst = new QuadInst(OP_TEST, nullptr, condition, condition);
                QuadInst *jmp_inst = new QuadInst(OP_JE, (QuadInst *) nullptr);
                jmp_false.push_back(jmp_inst);
            }

            EmitStmt(((IRWhile *) stmt)->while_body_);
            inst = new QuadInst(OP_JMP, label_inst);
            inst = new QuadInst(OP_LABEL, label_count_++);

            auto& continues = continues_.top();
            for (auto i : continues) {
                i->jmp_target_ = label_inst;
            }
            auto& breaks = breaks_.top();
            for (auto i : breaks) {
                i->jmp_target_ = inst;
            }
            for (auto i : jmp_false) {
                i->jmp_target_ = inst;
            }
            breaks_.pop();
            continues_.pop();

            break;
        }
        case IR_Break:
            inst = new QuadInst(OP_JMP, (QuadInst*)nullptr);
            assert(!breaks_.empty());
            breaks_.top().push_back(inst);
            break;
        case IR_Continue:
            inst = new QuadInst(OP_JMP, (QuadInst*) nullptr);
            assert(!continues_.empty());
            continues_.top().push_back(inst);
            break;
        case IR_Return:
            if (((IRReturn*)stmt)->is_last_return_) {
                EmitExpr(((IRReturn*)stmt)->ret_val_, IRReg::GetReg(IRReg::rax));
                inst = new QuadInst(OP_LABEL, label_count_++);

                for (auto i: returns_) {
                    i->jmp_target_ = inst;
                }
                returns_.clear();

                inst = new QuadInst(OP_POP, IRReg::GetReg(IRReg::rbp), nullptr, nullptr);
                inst = new QuadInst(OP_RET, (unsigned)0);
            } else {
                EmitExpr(((IRReturn*)stmt)->ret_val_, IRReg::GetReg(IRReg::rax));
                inst = new QuadInst(OP_JMP, (QuadInst*)nullptr);
                returns_.push_back(inst);
            }
            break;
        case IR_Null:
            inst = new QuadInst(OP_NOP, (unsigned )0);
            return;
        default:
            break;
    }
}

IRAsmOperand *QuadCodeEmitter::EmitExpr(IRExpr *expr, IRTemp *result_storer, std::vector<QuadInst *> *jmp_true,
                                        std::vector<QuadInst *> *jmp_false) {
    // only jmp or appoint a result storer
    // considering this: we use jmp_true to skip creating a result_storer
    // If jmp_true and logic operator, don't return. Otherwise, return
    /**
     *                                                jmp_true                 not jmp_true
     *   logic this, logic next                no ret, no convert           need ret, no convert
     *   logic this, not logic next            no ret, need convert         need ret, need convert
     *   not logic this, logic next                    need ret                   need ret
     *   not logic this, not next                      need ret                   need ret
     */
    assert((result_storer && !jmp_true) || (!result_storer && jmp_true));
    assert((jmp_true && jmp_false) || (!jmp_true && !jmp_false));
    assert(expr);
    is_cur_block_global_ = false;
    is_logic_operators_ = false;
    QuadInst *inst = nullptr;
    switch (expr->ClassId()) {
        case IR_Num:
        case IR_Char:
        case IR_StrLiteral:
        case IR_Temp:
        case IR_Var:
        case IR_Reg:
            if (!result_storer || result_storer == expr) {
                // for const, expr can't be equal as result_storer
                return (IRAsmOperand*)expr;
            } else {
                inst = new QuadInst(OP_ASSIGN, result_storer, (IRAsmOperand*)expr, nullptr);
                return result_storer;  // result_storer shares the result of expr
            }
        case IR_Unary: {
            auto op = ((IRUnary*)expr)->op_;
            auto rval = ((IRUnary*)expr)->rval_;
            switch (op) {
                case KW_POSITIVE:
                    return EmitExpr(rval, result_storer);
                case KW_LINC:
                case KW_LDEC: {
                    // todo: index select is not var!
                    assert(rval->ClassId() == IR_Var);
                    auto var = (IRVar*)((IRUnary*)expr)->rval_;
                    IRNum* num = new IRNum(1);
                    inst = new QuadInst(op == KW_LINC ? OP_ADD : OP_SUB, var, var, num);
                    if (result_storer && result_storer != var) {
                        inst = new QuadInst(OP_ASSIGN, result_storer, var, nullptr);
                        return result_storer;
                    } else {
                        return var;
                    }
                }
                case KW_NOT: {
                    // logic operators: and, or and not, owing to short-circuit evaluation, && and || must use jump
                    bool saved_logic_operators;
                    if (jmp_true) {
                        auto rval_temp = EmitExpr(rval, nullptr, jmp_false, jmp_true);
                        saved_logic_operators = is_logic_operators_;
                        is_logic_operators_ = true;
                        if (saved_logic_operators) {
                            return nullptr;  // don't need return anything, just jmp
                        } else {
                            inst = new QuadInst(OP_TEST, nullptr, rval_temp, rval_temp);
                            // todo: can improve, need to know upper's operator is || or &&
                            inst  = new QuadInst(OP_JE, (QuadInst*) nullptr);
                            jmp_true->push_back(inst);

                            inst = new QuadInst(OP_JMP, (QuadInst*) nullptr);
                            jmp_false->push_back(inst);
                            return nullptr;
                        }
                    }
                    std::vector<QuadInst *> jmp_true_internal, jmp_false_internal;
                    auto rval_temp = EmitExpr(rval, nullptr, &jmp_true_internal, &jmp_false_internal);
                    saved_logic_operators = is_logic_operators_;
                    is_logic_operators_ = true;
                    if (saved_logic_operators) {
                        inst = new QuadInst(OP_LABEL, label_count_++);
                        for (auto i : jmp_false_internal) {
                            i->jmp_target_ = inst;
                        }
                        IRTemp* result = result_storer ? result_storer : cur_scope_->CreateTemp(nullptr);
                        inst = new QuadInst(OP_ASSIGN, result, new IRNum(1), nullptr);
                        auto end_jmp = new QuadInst(OP_JMP, (QuadInst*)nullptr);

                        inst = new QuadInst(OP_LABEL, label_count_++);
                        for (auto i : jmp_true_internal) {
                            i->jmp_target_ = inst;
                        }
                        inst = new QuadInst(OP_ASSIGN, result, new IRNum(0), nullptr);
                        inst = new QuadInst(OP_LABEL, label_count_++);
                        end_jmp->jmp_target_ = inst;
                        return result;
                    }

                    inst = new QuadInst(OP_TEST, nullptr, rval_temp, rval_temp);
                    if (result_storer) {
                        inst = new QuadInst(OP_SETE, result_storer, nullptr, nullptr);
                        result_storer->width_ = IRTemp::ONE_BYTE;  // todo: need?
                        return result_storer;
                    } else {
                        IRTemp* result = cur_scope_->CreateTemp(nullptr);
                        inst = new QuadInst(OP_SETE, result, nullptr,  nullptr);
                        result->width_ = IRTemp::ONE_BYTE;  // todo: need?
                        return result;
                    }
                }
                case KW_NEGATE: {
                    auto rval_temp = EmitExpr(rval);
                    if (!result_storer) {
                        result_storer = cur_scope_->CreateTemp(nullptr);
                    }
                    inst = new QuadInst(OP_NEG, result_storer, rval_temp, nullptr);
                    return result_storer;
                }
                case KW_LEA: {
                    // todo: index select is not var!
                    assert(rval->ClassId() == IR_Var);
                    auto var = (IRVar*)((IRUnary*)expr)->rval_;
                    if (result_storer) {
                        inst = new QuadInst(OP_LEA, result_storer, var, nullptr);
                        return result_storer;
                    } else {
                        IRTemp* result = cur_scope_->CreateTemp(nullptr);
                        inst = new QuadInst(OP_LEA, result, var, nullptr);
                        return result;
                    }
                }
                case KW_DEREF: {
                    auto rval_temp = EmitExpr(rval);
                    assert(!rval_temp->IsConst());  // cant deref a immediate?
                    if (result_storer) {
                        inst = new QuadInst(OP_DREF, result_storer, rval_temp, nullptr);
                        return result_storer;
                    } else {
                        IRTemp* result = cur_scope_->CreateTemp(nullptr);
                        inst = new QuadInst(OP_DREF, result, rval_temp, nullptr);
                        return result;
                    }
                }
                case KW_RDEC:
                case KW_RINC: {
                    // todo: index select is not var!
                    assert(rval->ClassId() == IR_Var);
                    auto var = (IRVar*)((IRUnary*)expr)->rval_;
                    IRNum* num = new IRNum(1);
                    IRTemp* result = nullptr;
                    if (result_storer != var) {
                        result = cur_scope_->CreateTemp(nullptr);
                        inst = new QuadInst(OP_ASSIGN, result, var, nullptr);
                    } else {
                        result = var;
                    }

                    inst = new QuadInst(op == KW_LINC ? OP_ADD : OP_SUB, var, var, num);

                    return result;
                }
                default:
                    break;
            }
        }
        case IR_Binary: {
            auto op = ((IRBinary*)expr)->op_;
            auto left = ((IRBinary*)expr)->lval_;
            auto right = ((IRBinary*)expr)->rval_;
            IRAsmOperand* left_temp = nullptr;
            IRAsmOperand* right_temp = nullptr;
            const static std::vector<InstOperator> binary_operators_ = {
                    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_AND, OP_OR};
            switch (op) {
                case KW_ADD:
                case KW_SUB:
                case KW_MUL:
                case KW_DIV:
                case KW_MOD:
                case KW_BIT_AND:
                case KW_BIT_OR: {
                    left_temp = EmitExpr(left, nullptr);
                    right_temp = EmitExpr(right, nullptr);

                    if (!result_storer) {
                        result_storer = cur_scope_->CreateTemp(nullptr);  // todo: consider width
                    }
                    inst = new QuadInst(binary_operators_[op - KW_AND], result_storer, left_temp, right_temp);
                    return result_storer;
                }
                case KW_ASSIGN: {
                    right_temp = EmitExpr(right, nullptr);

                    assert(left->IsVar());
                    inst = new QuadInst(OP_ASSIGN, (IRVar*)left, right_temp, nullptr);
                    return (IRVar*)left;
                }
                case KW_AND: {
                    std::vector<QuadInst *> jmp_true_internal{}, jmp_false_internal{};
                    auto left_temp = EmitExpr(left, nullptr, &jmp_true_internal,
                                              jmp_false ? jmp_false : &jmp_false_internal);
                    if (!is_logic_operators_) {
                        inst = new QuadInst(OP_TEST, nullptr, left_temp, left_temp);
                        // save the time consumption of a jump to left's true exit
                        inst = new QuadInst(OP_JE, (QuadInst *) nullptr);
                        if (jmp_false) {
                            jmp_false->push_back(inst);
                        } else {
                            jmp_false_internal.push_back(inst);
                        }
                    }
                    inst = new QuadInst(OP_LABEL, label_count_++);
                    for (auto i: jmp_true_internal) {
                        i->jmp_target_ = inst;
                    }
                    jmp_true_internal.clear();

                    auto right_temp = EmitExpr(right, nullptr, jmp_true ? jmp_true : &jmp_true_internal,
                                               jmp_false ? jmp_false : &jmp_false_internal);
                    if (!is_logic_operators_) {
                        inst = new QuadInst(OP_TEST, nullptr, right_temp, right_temp);
                        inst = new QuadInst(OP_JNE, (QuadInst *) nullptr);
                        if (jmp_true) {
                            jmp_true->push_back(inst);
                        } else {
                            jmp_true_internal.push_back(inst);
                        }
                        inst = new QuadInst(OP_JMP, (QuadInst *) nullptr);
                        if (jmp_false) {
                            jmp_false->push_back(inst);
                        } else {
                            jmp_false_internal.push_back(inst);
                        }
                    }

                    is_logic_operators_ = true;
                    if (jmp_true) {
                        return nullptr;
                    }
                    IRTemp* result = nullptr;
                    if (!result_storer) {
                        result = cur_scope_->CreateTemp(nullptr);
                    } else {
                        result = result_storer;
                    }
                    inst = new QuadInst(OP_LABEL, label_count_++);
                    for (auto i: jmp_true_internal) {
                        i->jmp_target_ = inst;
                    }
                    inst = new QuadInst(OP_ASSIGN, result, new IRNum(1), nullptr);
                    auto jmp_inst = new QuadInst(OP_JMP, (QuadInst *) nullptr);
                    inst = new QuadInst(OP_LABEL, label_count_++);
                    for (auto i: jmp_false_internal) {
                        i->jmp_target_ = inst;
                    }
                    inst = new QuadInst(OP_ASSIGN, result, new IRNum(0), nullptr);
                    inst = new QuadInst(OP_LABEL, label_count_++);
                    jmp_inst->jmp_target_ = inst;
                    return result;
                }
                case KW_OR: {
                    std::vector<QuadInst *> jmp_true_internal{}, jmp_false_internal{};
                    auto left_temp = EmitExpr(left, nullptr, jmp_true ? jmp_true : &jmp_true_internal,
                                              &jmp_false_internal);
                    if (!is_logic_operators_) {
                        inst = new QuadInst(OP_TEST, nullptr, left_temp, left_temp);
                        // save the time consumption of a jump to left's true exit
                        inst = new QuadInst(OP_JNE, (QuadInst *) nullptr);
                        if (jmp_true) {
                            jmp_true->push_back(inst);
                        } else {
                            jmp_true_internal.push_back(inst);
                        }
                    }
                    inst = new QuadInst(OP_LABEL, label_count_++);
                    for (auto i: jmp_false_internal) {
                        i->jmp_target_ = inst;
                    }
                    jmp_true_internal.clear();

                    auto right_temp = EmitExpr(right, nullptr, jmp_true ? jmp_true : &jmp_true_internal,
                                               jmp_false ? jmp_false : &jmp_false_internal);
                    if (!is_logic_operators_) {
                        inst = new QuadInst(OP_TEST, nullptr, right_temp, right_temp);
                        inst = new QuadInst(OP_JNE, (QuadInst *) nullptr);
                        if (jmp_true) {
                            jmp_true->push_back(inst);
                        } else {
                            jmp_true_internal.push_back(inst);
                        }
                        inst = new QuadInst(OP_JMP, (QuadInst *) nullptr);
                        if (jmp_false) {
                            jmp_false->push_back(inst);
                        } else {
                            jmp_false_internal.push_back(inst);
                        }
                    }

                    is_logic_operators_ = true;
                    if (jmp_true) {
                        return nullptr;
                    }
                    IRTemp* result = nullptr;
                    if (!result_storer) {
                        result = cur_scope_->CreateTemp(nullptr);
                    } else {
                        result = result_storer;
                    }
                    inst = new QuadInst(OP_LABEL, label_count_++);
                    for (auto i: jmp_true_internal) {
                        i->jmp_target_ = inst;
                    }
                    inst = new QuadInst(OP_ASSIGN, result, new IRNum(1), nullptr);
                    auto jmp_inst = new QuadInst(OP_JMP, (QuadInst *) nullptr);
                    inst = new QuadInst(OP_LABEL, label_count_++);
                    for (auto i: jmp_false_internal) {
                        i->jmp_target_ = inst;
                    }
                    inst = new QuadInst(OP_ASSIGN, result, new IRNum(0), nullptr);
                    inst = new QuadInst(OP_LABEL, label_count_++);
                    jmp_inst->jmp_target_ = inst;
                    return result;
                }
                case KW_GT:
                case KW_GE:
                case KW_LT:
                case KW_LE:
                case KW_EQUAL:
                case KW_NEQUAL: {
                    static const std::array<InstOperator, 6> jmp_operators = {OP_JG, OP_JGE, OP_JL, OP_JLE, OP_JE, OP_JNE};
                    static const std::array<InstOperator, 6> set_operators = {OP_SETG, OP_SETGE, OP_SETL, OP_SETLE, OP_SETE, OP_SETNE};

                    auto left_temp = EmitExpr(left, nullptr, nullptr, nullptr);
                    auto right_temp = EmitExpr(right, nullptr, nullptr, nullptr);

                    inst = new QuadInst(OP_CMP, nullptr, left_temp, right_temp);
                    if (jmp_true) {
                        inst = new QuadInst(jmp_operators[op - KW_GT], (QuadInst*) nullptr);
                        jmp_true->push_back(inst);
                        inst = new QuadInst(OP_JMP, (QuadInst*) nullptr);
                        jmp_false->push_back(inst);
                        is_logic_operators_ = true;
                        return nullptr;
                    } else {
                        IRTemp* result = nullptr;
                        if (result_storer) {
                            result = result_storer;
                        } else {
                            result = cur_scope_->CreateTemp(nullptr);
                        }
                        inst = new QuadInst(set_operators[op - KW_GT], result, nullptr, nullptr);
                        is_logic_operators_ = false;
                        return result;
                    }
                }
                case KW_SUBTRACT_EQ:
                case KW_ADD_EQ: {
                    assert(left->ClassId() == IR_Var);
                    auto right_temp = EmitExpr(right, nullptr, nullptr, nullptr);
                    inst = new QuadInst(op == KW_SUBTRACT_EQ ? OP_SUB : OP_ADD, (IRVar*)left, (IRVar*)left, right_temp);
                    return (IRVar*)left;
                }
                case KW_DIV_EQ: {
                    assert(left->ClassId() == IR_Var);
                    auto right_temp = EmitExpr(right, nullptr, nullptr, nullptr);
                    inst = new QuadInst(OP_DIV, (IRVar *) left, (IRVar *) left, right_temp);
                    return (IRVar *) left;
                }
                default:
                    assert(0);
                    return nullptr;
            }
        }
        case IR_FuncCall: {
            auto ir_func_call = (IRFuncCall*)expr;
            assert(ir_func_call->args_.size() <= 4);
            auto size = ir_func_call->args_.size();
            static const std::array<IRReg*, 4> regs = {IRReg::GetReg(IRReg::rcx), IRReg::GetReg(IRReg::rdx), IRReg::GetReg(IRReg::r8), IRReg::GetReg(IRReg::r9)};
            for (unsigned i = 0; i < size; i++) {
                EmitExpr(ir_func_call->args_[i], regs[i], nullptr, nullptr);
            }
            auto it = func_addr_map_.find(ir_func_call->func_);
            assert(it != func_addr_map_.end());
            QuadInst* call_inst = it->second;
            inst = new QuadInst(OP_CALL, call_inst);
            return IRReg::GetReg(IRReg::rax);
        }
        case IR_SysFuncCall:
            assert(0);
            return nullptr;
        default:
            return nullptr;
    }
}