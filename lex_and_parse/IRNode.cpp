#include "IRNode.h"

MemoryCollector memory_collect;

const char *KeyWordToStr(KeyWord key_word) {
    switch (key_word) {
        case KW_NOT:
            return "!";
        case KW_NEGATE:
            return "-";
        case KW_POSITIVE:
            return "+";
        case KW_LEA:
            return "&";
        case KW_DEREF:
            return "*";
        case KW_LINC:
            return "++";
        case KW_LDEC:
            return "--";
        case KW_RDEC:
            return "--";
        case KW_RINC:
            return "++";
        case KW_ADD:
            return "+";
        case KW_SUB:
            return "-";
        case KW_MUL:
            return "*";
        case KW_DIV:
            return "/";
        case KW_MOD:
            return "%";
        case KW_AND:
            return "&&";
        case KW_OR:
            return "||";
        case KW_BIT_AND:
            return "&";
        case KW_ASSIGN:
            return "=";
        case KW_GT:
            return ">";
        case KW_GE:
            return ">=";
        case KW_LT:
            return "<";
        case KW_LE:
            return "<=";
        case KW_EQUAL:
            return "==";
        case KW_NEQUAL:
            return "!=";
        case KW_SUBTRACT_EQ:
            return "-=";
        case KW_ADD_EQ:
            return "+=";
        case KW_DIV_EQ:
            return "/=";
        case KW_COMMA:
            return ",";
        case KW_COLON:
            return ":";
        case KW_SEMICOLON:
            return ";";
        case KW_LPAREN:
            return "(";
        case KW_RPAREN:
            return ")";
        case KW_LBRAC:
            return "{";
        case KW_RBRAC:
            return "}";
        case KW_LSBRAC:
            return "[";
        case KW_RSBRAC:
            return "]";
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
        case TUInt:
            os << "unsigned ";
            break;
        case TInt:
            os << "int ";
            break;
        case TULong:
            os << "unsigned long ";
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
    unsigned size = args_.size();
    for (unsigned i = 0; i < size; i++) {
        args_[i]->Display(os, 0);
        if (i != size - 1) {
            os << ", ";
        }
    }
    os << ") ";
}

void IRSysFuncCall::Display(std::ostream &os, unsigned int lv) {
    std::string spaces(lv, ' ');
    os << spaces;
    if (key_word_ == KW_PRINTF) {
        os << "printf(";
    } else {
        assert(0);
    }
    unsigned size = args_.size();
    for (unsigned i = 0; i < size; i++) {
        args_[i]->Display(os, 0);
        if (i != size - 1) {
            os << ", ";
        }
    }
    os << ") ";
}

void IRVarDecl::Display(std::ostream &os, unsigned lv, bool new_line) {
    assert(!vars_.empty());
    if (new_line) {
        os << "\n";
    }
    vars_.front()->type_->Display(os, lv);
    unsigned cnt = vars_.size();
    for (unsigned i = 0; i < cnt; i++) {
        auto var = vars_[i];
        assert(var);
        os << " " << var->name_;
        if (var->initial_val_) {
            os  << " = ";
            var->initial_val_->Display(os, 0);
        }
        if (i + 1 != cnt) {
            os << ", ";
        }
    }
    os << ";";
}

void IRFunc::Display(std::ostream &os, unsigned lv) {
    std::string spaces(lv, ' ');
    os << "\n" << spaces;
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
    os << ") {";
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

        if (type->base_type_ != IRType::TVoid) {
            auto last_stmt = ((IRFunc *) cur_scope)->stmts_.back();
            assert(last_stmt->ClassId() == IR_Return);
            ((IRReturn*)last_stmt)->is_last_return_ = true;
        }
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

// ++, --, *, &, +, - are ambiguous according to the last token is operand or operator
void ChangeAmbiguity(TinyKeyWord* token) {
    KeyWord key_word = token->GetKeyWord();
    auto it = TinyToken::ambiguous_keyword_.find(key_word);
    if (it != TinyToken::ambiguous_keyword_.end()) {
        ((TinyKeyWord*)token)->SetKeyWord(it->second);
    }
}

IRExpr *ParseIR::ParseExpr() {
    std::stack<TinyKeyWord *> opers;
    std::stack<IRExpr *> exprs;
    TinyToken *token = nullptr;
    bool is_operand = false;  // while (*end != '\0') ++end;
    while (true) {
        token = peek();
        if (token->IsKeyWord()) {
            if (!is_operand) {
                ChangeAmbiguity((TinyKeyWord*)token);
            }
            if (opers.empty()) {
                if (token->GetKeyWord() == KW_RPAREN || token->GetPriority() == 0) {
                    // , ; or funccall(x), follow set: ) or priority == 0
                    if (exprs.empty()) {  // directly parse , ; or )
                        return nullptr;
                    } else {
                        assert(exprs.size() == 1);
                        return exprs.top();
                    }
                } else {
                    step();
                    opers.push((TinyKeyWord *)token);
                }
            } else if (token->GetKeyWord() == KW_RPAREN || token->GetKeyWord() == KW_RSBRAC ||
                       token->GetPriority() == 0 || opers.top()->IsLeftPrior((TinyKeyWord *) token)) {
                TinyKeyWord* op = opers.top();

                if (op->GetKeyWord() == KW_LPAREN) {
                    if (token->GetKeyWord() == KW_RPAREN) {  // remove ()
                        step();
                        opers.pop();
                        is_operand = true;
                    } else if (token->GetPriority() == 0) {
                        assert(0);
                    } else {  // (*a++), although ( is < *, still push * in stack
                        step();
                        opers.push((TinyKeyWord*)token);
                        is_operand = false;
                    }
                } else if (op->GetKeyWord() == KW_LSBRAC) {
                    if (token->GetKeyWord() == KW_RSBRAC) {  // remove [index]
                        assert(exprs.size() >= 2);
                        step();
                        opers.pop();

                        IRExpr *lval = nullptr, *rval = nullptr;

                        rval = exprs.top();
                        exprs.pop();
                        lval = exprs.top();
                        exprs.pop();
                        exprs.push(ParseBinary(lval, rval, KW_LSBRAC));
                        is_operand = true;
                    } else if (token->GetPriority() == 0) {
                        assert(0);
                    } else {  // [*a++], although [ is < *, still push * in stack
                        step();
                        opers.push((TinyKeyWord*)token);
                        is_operand = false;
                    }
                } else if (op->IsUnary()) {
                    assert(!exprs.empty());
                    IRExpr* rval = exprs.top();
                    exprs.pop();
                    opers.pop();
                    exprs.push(ParseUnary(rval, op->GetKeyWord()));
                    is_operand = true;
                } else if (op->IsBinary()) {
                    assert(exprs.size() > 1);  // because !opers.empty()
                    IRExpr *lval = nullptr, *rval = nullptr;

                    rval = exprs.top();
                    exprs.pop();
                    lval = exprs.top();
                    exprs.pop();
                    opers.pop();
                    exprs.push(ParseBinary(lval, rval, op->GetKeyWord()));
                    is_operand = true;
                } else {
                    assert(0);
                }
            } else {
                step();
                opers.push((TinyKeyWord *) token);
                is_operand = false;
            }
        } else if (token->IsId()) {
            step();
            auto id_name = token->GetId();
            assert(!id_name.empty());

            // todo: lack detection of ' ', like: func (arg0, arg1...)
            token = peek();
            auto sys_func_kw = IRSysFuncCall::GetSysFuncKW(id_name);
            if (token->GetKeyWord() == KW_LPAREN && (sys_func_kw == IRSysFuncCall::KW_NONE)) {
                // todo: now don't support default value as func/task call's arg cause lack of copy method
                step();
                IRFunc *func = sym_table_->SearchFunc(id_name);
                assert(func);
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
            } else if (sys_func_kw != IRSysFuncCall::KW_NONE) {
                // todo: now don't support default value as func/task call's arg cause lack of copy method
                step();
                assert(token->GetKeyWord() == KW_LPAREN);
                IRSysFuncCall* sys_func_call = new IRSysFuncCall();
                sys_func_call->key_word_ = sys_func_kw;

                IRExpr* arg = nullptr;
                while ((arg = ParseExpr())) {
                    sys_func_call->args_.push_back(arg);

                    token = peek();
                    if (token->GetKeyWord() == KW_COMMA) {
                        token = step();
                    }
                }

                token = step();
                assert(token->GetKeyWord() == KW_RPAREN);
                exprs.push(sys_func_call);
            } else {
                IRVar *var = sym_table_->SearchVar(id_name);
                assert(var);
                exprs.push(var);
            }
            is_operand = true;
        } else if (token->IsNum()) {
            step();
            IRNum *number = new IRNum();
            number->val_ = token->GetNum();
            exprs.push(number);
            is_operand = true;
        } else if (token->IsChar()) {
            step();
            IRChar* ir_char = new IRChar();
            ir_char->ch_ = token->GetChar();
            exprs.push(ir_char);
            is_operand = true;
        } else if (token->IsStr()) {
            step();
            IRStr* ir_str = new IRStr();
            ir_str->str_ = token->GetStr();
            exprs.push(ir_str);
            is_operand = true;
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

    TinyToken* token = peek();
    if (token->GetKeyWord() == KW_CONST) {
        new_type->is_const_ = true;
        step();
    }

    token = step();
    auto type_key_word = token->GetKeyWord();
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
        case KW_UINT:
            new_type->base_type_ = IRType::TUInt;
            break;
        case KW_INT:
            new_type->base_type_ = IRType::TInt;
            break;
        case KW_ULONG:
            new_type->base_type_ = IRType::TULong;
            break;
        case KW_LONG:
            new_type->base_type_ = IRType::TLong;
            break;
        default:
            assert(0);
    }

    token = peek();
    if (token->GetKeyWord() == KW_DEREF || token->GetKeyWord() == KW_MUL) {
        // const * or int * etc. are parsed as deref
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
        {KW_LONG, KW_ULONG, KW_INT, KW_UINT, KW_CHAR, KW_BOOL, KW_VOID};

IRStmt *ParseIR::ParseStmt() {
    TinyToken *token = nullptr;
    token = peek();
    // need to consider: keywords used in expr all have priority, although those in type don't
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
            token = peek();
            if (token->GetKeyWord() == KW_ELSE) {
                token = step();
                if_stmt->else_stmt_ = ParseBlock();
            }

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
        } else if (key_word == KW_WHILE) {
            step();
            auto while_stmt = new IRWhile();
            token = step();
            assert(token->GetKeyWord() == KW_LPAREN);
            while_stmt->condition_ = ParseExpr();
            token = step();
            assert(token->GetKeyWord() == KW_RPAREN);
            while_stmt->while_body_ = ParseStmt();
            return while_stmt;
        } else {
            /* todo: when support class, user-defined type need to consider
            */
            return ParseDecl();
        }
    } else {
        auto expr_stmt = new IRExprStmt();
        expr_stmt->expr_ = ParseExpr();
        token = step();
        assert(token->GetKeyWord() == KW_SEMICOLON);
        return expr_stmt;
    }
}