#include "IRNode.h"

MemoryCollector memory_collect;

const char *KeyWordToStr(KeyWord key_word) {
    switch (key_word) {
        case KW_NOT:
            return "!";
            break;
        case KW_NEGATE:
            return "-";
            break;
        case KW_LEA:
            return "&";
            break;
        case KW_DEREF:
            return "*";
            break;
        case KW_INC:
            return "++";
            break;
        case KW_DEC:
            return "--";
            break;
        case KW_ADD:
            return "+";
            break;
        case KW_SUB:
            return "-";
            break;
        case KW_MUL:
            return "*";
            break;
        case KW_DIV:
            return "/";
            break;
        case KW_MOD:
            return "%";
            break;
        case KW_RDEC:
            return "--";
            break;
        case KW_RINC:
            return "++";
            break;
        case KW_AND:
            return "+";
            break;
        case KW_OR:
            return "||";
            break;
        case KW_ASSIGN:
            return "=";
            break;
        case KW_GT:
            return ">";
            break;
        case KW_GE:
            return ">=";
            break;
        case KW_LT:
            return "<";
            break;
        case KW_LE:
            return "<=";
            break;
        case KW_EQUAL:
            return "==";
            break;
        case KW_NEQUAL:
            return "!=";
            break;
        default:
            assert(0);
    }
}

// type ------------------------------------------------
void IRType::Display(std::ostream &os, unsigned lv) {
    std::string spaces(lv, ' ');
    os << spaces;
    if (is_const_) {
        os << "const ";
    }
    switch (base_type_) {
        case TVoid:
            os << "void ";
            break;
        case TBool:
            os << "bool ";
            break;
        case TChar:
            os << "char ";
            break;
        case TShort:
            os << "short ";
            break;
        case TInt:
            os << "int ";
            break;
        case TLong:
            os << "long ";
            break;
        default:
            assert(0);
    }
    if (is_ptr_) {
        os << "* ";
    }
    for (auto i: dims_) {
        os << "[" << i << "]";
    }
}

void IRFuncCall::Display(std::ostream &os, unsigned lv) {
    assert(func_);
    std::string spaces(lv, ' ');
    os << spaces << func_->name_ << "(";
    for (auto i: args_) {
        i->Display(os, 0);
    }
    os << ") ";
}

void IRVarDecl::Display(std::ostream &os, unsigned lv) {
    assert(!vars_.empty());
    vars_.front()->type_->Display(os, lv);
    unsigned cnt = vars_.size();
    for (unsigned i = 0; i < cnt; i++) {
        auto var = vars_[i];
        assert(var);
        os << " " << var->name_ << " = ";
        if (var->initial_val_) {
            var->initial_val_->Display(os, 0);
        }
        if (i + 1 != cnt) {
            os << ", ";
        }
    }
    os << ";\n";
}

void IRFunc::Display(std::ostream &os, unsigned lv) {
    std::string spaces(lv, ' ');
    os << spaces;
    ret_type_->Display(os, 0);  // type has ' ' at the back
    os << name_ << "(";
    unsigned cnt = params_.size();
    for (unsigned i = 0; i < cnt; i++) {
        params_[i]->type_->Display(os, 0);
        os << " ";
        params_[i]->Display(os, 0);
        if (i + 1 != cnt) {
            os << ", ";
        }
    }
    os << ") {\n";
    // not use IRCodeBlock::Display(os, lv); because of display format

    for (auto i: stmts_) {
        i->Display(os, lv + 4);
    }
    os << "\n" << spaces << "}" << std::endl;
}

IRCodeBlock *ParseIR::ParseGlobalBlock() {
    sym_table_->Enter("Global_Block");  // maybe generate unique block name

    auto cur_scope = sym_table_->GetCurScope();
    assert(cur_scope->IsCodeBlock());
    ((IRCodeBlock *) cur_scope)->is_global_ = true;

    TinyToken *token = nullptr;
    while ((token = peek())) {
        auto ir_stmt = ParseStmt();
        ((IRCodeBlock *) cur_scope)->stmts_.push_back(ir_stmt);
    }

    sym_table_->Leave();
    assert(peek() == nullptr);

    return (IRCodeBlock *) cur_scope;
}

/*block statement {}*/
IRCodeBlock *ParseIR::ParseBlock() {
    assert(peek()->IsKeyWord() && step()->GetKeyWord() == KW_LBRAC);
    sym_table_->Enter("");  // maybe generate unique block name

    auto cur_scope = sym_table_->GetCurScope();
    assert(cur_scope->IsCodeBlock());

    while (!(peek()->IsKeyWord() && peek()->GetKeyWord() == KW_RBRAC)) {
        auto ir_stmt = ParseStmt();
        ((IRCodeBlock *) cur_scope)->stmts_.push_back(ir_stmt);
    }

    sym_table_->Leave();
    assert(step()->GetKeyWord() == KW_RBRAC);

    return (IRCodeBlock *) cur_scope;
}

// todo: is_extern is not used
IRStmt *ParseIR::ParseDecl(bool is_extern) {
    // todo: handle is_extern
    IRType *type = ParseType();
    return ParseDeclTail(type, false);
}

// if is_func_port, will return nullptr cause it isn't a stmt
IRStmt *ParseIR::ParseDeclTail(IRType *type, bool is_func_port, IRVarDecl *var_decl) {
    auto cur_scope = sym_table_->GetCurScope();
    assert(peek()->IsId());
    std::string id_name = step()->GetId();

    // if type already has dimensions, need to add copy function!
    IRType *raw_type = new IRType;
    *raw_type = *type;
    raw_type->dims_.clear();
    type = raw_type;

    bool is_dimension = false;
    // match []
    while (peek()->IsKeyWord() && peek()->GetKeyWord() == KW_LSBRAC) {
        is_dimension = true;
        step();
        assert(peek()->IsNum());
        type->dims_.push_back(step()->GetNum());
        assert(step()->GetKeyWord() == KW_RSBRAC);
    }

    // match (...), if function
    if (peek()->GetKeyWord() != KW_LPAREN) {
        // var decls

        IRVar *new_node = cur_scope->CreateNewVar(id_name, type);
        // cur_scope->inter_insts_.push_back(new InterInst(OP_DEC, new_node, nullptr));

        if (is_func_port) {
            assert(cur_scope->IsFunc());
            ((IRFunc *) cur_scope)->params_.push_back(new_node);
            // var_decl is nullptr here
        } else {
            // create var_decl stmt if not created
            if (var_decl == nullptr) {
                var_decl = new IRVarDecl();
            }
            var_decl->vars_.push_back(new_node);
        }

        auto key_word = peek()->GetKeyWord();
        if (key_word == KW_ASSIGN) {
            step();
            new_node->initial_val_ = ParseExpr();
        }

        key_word = peek()->GetKeyWord();

        // 1. int a, b;  2. int calculate(int a, int b);
        // three situations after id: 1. , and id    2. ;     3. , and type id (only in func params)
        assert(key_word == KW_COMMA || key_word == KW_SEMICOLON || key_word == KW_RPAREN);
        if (key_word == KW_COMMA) { // ,
            step();
            if (!is_func_port) {
                ParseDeclTail(type);
            }
        } else if (key_word == KW_SEMICOLON) { // ;
            assert(!is_func_port);
            step();
        }
        return var_decl;
    } else {
        // function decls
        assert(!is_dimension && !is_func_port);
        step(); // skip (

        sym_table_->Enter(std::move(id_name), true);

        IRType *var_type = nullptr;
        while (peek()->GetKeyWord() != KW_RPAREN) {
            var_type = ParseType();
            ParseDeclTail(var_type, true);
        }
        assert(step()->GetKeyWord() == KW_RPAREN);

        auto cur_scope = sym_table_->GetCurScope();
        assert(cur_scope->IsFunc());

        // same code as in ParseBlock, but different scope
        assert(step()->GetKeyWord() == KW_LBRAC);
        while (!(peek()->IsKeyWord() && peek()->GetKeyWord() == KW_RBRAC)) {
            auto ir_stmt = ParseStmt();
            ((IRFunc *) cur_scope)->stmts_.push_back(ir_stmt);
        }

        sym_table_->Leave();
        assert(step()->GetKeyWord() == KW_RBRAC);

        auto func_decl = new IRFuncDecl();
        func_decl->func_ = (IRFunc *) cur_scope;
        func_decl->func_->ret_type_ = type;
        return func_decl;
    }
}

/*variable decl statement*/

/*  expression statements
    1. x = a++ + (b - -20) % 10;
    2. x = a || a + b * c
*/
/*  assign operators:
    1. =
    2. +=  -=  *=  /=  %=  &=  |=  ^=  <<=  >>=, todo
    3. static_cast<T>=, todo
*/
/*  unary operators:
    -  *  &  !  ++ -- (~, not supported)

    binary operators:


    key points:
    1. priority of unary operators is 4, need to determine who are unary ops
    2. () is unique
*/

IRExpr *ParseIR::ParseUnary(IRExpr *expr, KeyWord op) {
    // need to consider const propagation
    auto unary = new IRUnary(expr, op);
    return unary;
}

IRExpr *ParseIR::ParseBinary(IRExpr *left, IRExpr *right, KeyWord op) {
    // need to consider const propagation
    auto binary = new IRBinary(left, right, op);
    return binary;
}

IRExpr *ParseIR::ParseExpr() {
    std::stack<TinyKeyWord *> opers;
    std::stack<IRExpr *> exprs;
    TinyToken *token = nullptr;
    while (true) {
        token = peek();
        if (token->IsKeyWord()) {
            bool is_unary = token->IsUnary();
            // unary operator or lower priority
            if (opers.empty()) {
                if (token->GetKeyWord() == KW_RPAREN || token->GetPriority() == 0) {
                    // , ; or funccall(x)
                    assert(exprs.size() == 1);
                    return exprs.top();
                } else {
                    step();
                    opers.push((TinyKeyWord *) token);
                }
            } else if (is_unary) {
                // todo: not support a++ ++ or ++a++
                IRExpr *rval = nullptr;
                step();
                TinyToken *next_token = peek();
                KeyWord op = token->GetKeyWord();
                if (next_token && next_token->IsKeyWord()) {
                    // not consider a++ ++ or ++a++, so it is right unary operator
                    rval = exprs.top();
                    exprs.pop();
                } else {
                    step();
                    assert(next_token->IsId());
                    auto id_name = next_token->GetId();
                    rval = sym_table_->SearchVar(id_name);
                    assert(rval);
                }
                exprs.push(ParseUnary(rval, op));
            } else if (token->GetKeyWord() == KW_RPAREN ||
                       (token->GetPriority() >= opers.top()->GetPriority()) || token->GetPriority() == 0) {
                KeyWord op = opers.top()->GetKeyWord();

                if (op == KW_LPAREN) {  // remove ()
                    step();
                    opers.pop();
                } else {
                    assert(exprs.size() > 1);  // because !opers.empty()
                    IRExpr *lval = nullptr, *rval = nullptr;

                    rval = exprs.top();
                    exprs.pop();
                    lval = exprs.top();
                    exprs.pop();
                    opers.pop();
                    exprs.push(ParseBinary(lval, rval, op));
                }
            } else {
                step();
                opers.push((TinyKeyWord *) token);
            }
        } else if (token->IsId()) {
            step();
            auto id_name = token->GetId();
            assert(!id_name.empty());

            // todo: lack detection of ' ', like: func (arg0, arg1...)
            token = peek();
            if (token->GetKeyWord() == KW_LPAREN) {
                // todo: now don't support default value as func/task call's arg cause lack of copy method
                step();
                IRFunc *func = sym_table_->SearchFunc(id_name);
                IRFuncCall *func_call = new IRFuncCall();
                func_call->func_ = func;

                unsigned size = func->params_.size();
                for (unsigned i = 0; i < size; i++) {
                    func_call->args_.push_back(ParseExpr());
                    if (i + 1 != size) {
                        token = step();
                        assert(token->GetKeyWord() == KW_COMMA);
                    }
                }
                token = step();
                assert(token->GetKeyWord() == KW_RPAREN);
                exprs.push(func_call);
            } else {
                IRVar *var = sym_table_->SearchVar(id_name);
                assert(var);
                exprs.push(var);
            }
        } else if (token->IsNum()) {
            step();
            IRConst *number = new IRConst();
            number->val_ = token->GetNum();
            exprs.push(number);
        } else {
            assert(0);
            break;
        }
    }
    assert(exprs.size() == 1 && opers.empty());
    return exprs.top();
}

IRType *ParseIR::ParseType() {
    IRType *new_type = new IRType();

    if (peek()->GetKeyWord() == KW_CONST) {
        new_type->is_const_ = true;
        step();
    }

    auto type_key_word = step()->GetKeyWord();
    switch (type_key_word) {
        case KW_VOID:
            new_type->base_type_ = IRType::TVoid;
            assert(!new_type->is_ptr_);
            break;
        case KW_BOOL:
            new_type->base_type_ = IRType::TBool;
            break;
        case KW_CHAR:
            new_type->base_type_ = IRType::TChar;
            break;
        case KW_INT:
            new_type->base_type_ = IRType::TInt;
            break;
        case KW_LONG:
            new_type->base_type_ = IRType::TLong;
            break;
        default:
            assert(0);
    }

    if (peek()->GetKeyWord() == KW_ASTERISK) {
        new_type->is_ptr_ = true;
        step();
    }

    return new_type;
}

IRVar *ParseIR::ParsePtr(IRVar *var) {
    auto type = var->type_;
    if (!type->is_ptr_) {
        SIMERROR("Error: Dereferencing of non-pointer variable.");
        return var;
    }
    // todo: if var is lea
    auto scope = sym_table_->GetCurScope();

    IRType *new_type = new IRType;
    *new_type = *type;
    new_type->is_ptr_ = false;

    IRVar *new_node = scope->CreateNewVar("", new_type);

    // scope->inter_insts_.push_back(new InterInst(OP_DEC, new_node, nullptr));
    // scope->inter_insts_.push_back(new InterInst(OP_RDREF, new_node, var));
    return new_node;
}

IRVar *ParseIR::ParseLea(IRVar *var) {
    if (!var->is_left_) {
        SIMERROR("Error: Taking the address of a non-lvalue variable.");
        return var;
    }
    // todo: if var is deref
    auto scope = sym_table_->GetCurScope();

    IRType *new_type = new IRType;
    *new_type = *var->type_;
    new_type->is_ptr_ = true;

    IRVar *new_node = scope->CreateNewVar("", new_type);

    // scope->inter_insts_.push_back(new InterInst(OP_DEC, new_node, nullptr));
    // scope->inter_insts_.push_back(new InterInst(OP_LEA, new_node, var));
    return new_node;
}

// built-in type tokens
const std::unordered_set<KeyWord> type_tokens =
        {KW_STR, KW_LONG, KW_INT, KW_CHAR, KW_BOOL, KW_VOID};

IRStmt *ParseIR::ParseStmt() {
    TinyToken *token = nullptr;
    token = peek();
    // need to consider: key words used in expr all have priority, although those in type don't
    if (token->IsKeyWord() && !token->GetPriority()) {
        // var/func decl stmt
        KeyWord key_word = token->GetKeyWord();
        if (key_word == KW_CONST || type_tokens.count(key_word)) {
            return ParseDecl();
        } else if (key_word == KW_IF) {
            step();
            auto if_stmt = new IRIfElse();
            token = step();
            assert(token->GetKeyWord() == KW_LPAREN);
            if_stmt->condition_ = ParseExpr();

            token = step();
            assert(token->GetKeyWord() == KW_RPAREN);

            if_stmt->then_stmt_ = ParseBlock();
            token = step();
            assert(token->GetKeyWord() == KW_ELSE);
            if_stmt->else_stmt_ = ParseBlock();

            return if_stmt;
        } else if (key_word == KW_FOR) {
            step();
            // todo: enter block
            auto for_stmt = new IRFor();
            token = step();
            assert(token->GetKeyWord() == KW_LPAREN);
            for_stmt->init_ = ParseStmt();

            auto condition = ParseExpr();  // todo: must can be cast to boolean
            token = step();
            assert(token->GetKeyWord() == KW_SEMICOLON);
            for_stmt->condition_ = condition;

            auto iteration = new IRExprStmt();
            iteration->expr_ = ParseExpr();
            token = step();
            assert(token->GetKeyWord() == KW_RPAREN);
            for_stmt->iteration_ = iteration;

            for_stmt->for_body_ = ParseBlock();
            return for_stmt;
        } else if (key_word == KW_BREAK) {
            step();
            token = step();
            assert(token->GetKeyWord() == KW_SEMICOLON);
            return new IRBreak();
        } else if (key_word == KW_CONTINUE) {
            step();
            token = step();
            assert(token->GetKeyWord() == KW_SEMICOLON);
            return new IRContinue();
        } else if (key_word == KW_RETURN) {
            step();
            auto ir_return = new IRReturn();
            ir_return->ret_val_ = ParseExpr();

            token = step();
            assert(token->GetKeyWord() == KW_SEMICOLON);
            return ir_return;
        } else if (key_word == KW_SEMICOLON) {
            step();
            return new IRNull();
        } else if (key_word == KW_LBRAC) {
            return ParseBlock();
        } else {
            /* todo: when support class, user-defined type need to consider
            */
            return ParseDecl();
        }
    } else {
        auto expr_stmt = new IRExprStmt();
        expr_stmt->expr_ = ParseExpr();
        return expr_stmt;
    }
}