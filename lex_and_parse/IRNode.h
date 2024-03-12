#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <iomanip>
#include <iostream>
#include "token.h"

#define SIMERROR(message) std::cout << message
#define ALIGN_VAL 8

/* todo:
1. var initial val
2. var len, offset
*/

// collect memory space allocated in heap
class MemoryCollector {
public:
    void Collect(void *heap_place) {
        heap_spaces_.push_back(heap_place);
    }

    void Clear() {
        for (auto i: heap_spaces_) {
            std::free(i);
        }
    }

private:
    std::vector<void *> heap_spaces_ = {};
};

extern MemoryCollector memory_collect;

/*
In C++, it is possible for a base class and a derived class to have static functions
with the same name, but they do not constitute an overloaded relationship; rather, it's
a hiding relationship.
*/
// operator new must be static, because it is before get the obj's address
#define OPERATOR_NEW    \
void* operator new(std::size_t count) {\
   void* place = std::malloc(count);\
   memory_collect.Collect(place);\
   return place;\
}

class IRType;

class IRVar;

class IRScope;

class IRStmt;

class IRFunc;

class IRStmt {
public:
    virtual void Display(std::ostream &os, unsigned lv = 0) {
        os << "empty stmt" << std::endl;
    }
};

/* expr-------------------------------------------------------------
*/

class IRType {
    // todo: support class/struct/enum type
public:
    OPERATOR_NEW

    enum type_enum {
        TVoid = 0, TBool = 1, TChar = 2, TShort = 3, TUInt = 4, TInt = 5, TULong = 6, TLong = 7
    };

    unsigned GetSize() {
        if (is_ptr_) {
            return 8;
        }
        unsigned size = 0;
        switch (base_type_) {
            case TVoid:
                size = 1;
                break;
            case TBool:
                size = 1;
                break;
            case TChar:
                size = 1;
                break;
            case TShort:
                size = 2;
                break;
            case TUInt:
                size = 4;
                break;
            case TInt:
                size = 4;
                break;
            case TULong:
                size = 8;
                break;
            case TLong:
                size = 8;
                break;
            default:
                assert(0);
        }
        for (auto i: dims_) {
            size *= i;
        }
        return size;
    }

    virtual void Display(std::ostream &os, unsigned lv = 0);

public:
    bool is_const_ = false;
    bool is_ptr_ = false;
    type_enum base_type_;
    std::vector<unsigned> dims_;
};

class IRExpr {
public:
    virtual bool IsConst() { return false; }

    virtual bool IsVar() { return false; }

    virtual void Display(std::ostream &os, unsigned lv = 0) {
        os << "empty expr" << std::endl;
    }
};

class IRConst : public IRExpr {
public:
    OPERATOR_NEW

    bool IsConst() override {return true;}
    virtual void Display(std::ostream &os, unsigned lv = 0) = 0;
};

class IRNum : public IRConst {
public:
    OPERATOR_NEW

    virtual void Display(std::ostream &os, unsigned lv = 0) {
        os << val_;
    }

    IRType::type_enum type_ = IRType::TLong;
    long val_ = 0;
};

class IRChar : public IRConst {
public:
    OPERATOR_NEW

    virtual void Display(std::ostream& os, unsigned lv = 0) {
        os << "\"" << ch_ << "\"";
    }
    char ch_ = 0;
};

class IRStr : public IRConst {
public:
    ~IRStr() {
        if (str_) free(str_);
    }
    OPERATOR_NEW

    virtual void Display(std::ostream& os, unsigned lv = 0) {}
    char* str_ = nullptr;
};

class IRVar : public IRExpr {
public:
    IRVar(const std::string &name) : name_(name) {}

    OPERATOR_NEW

    bool IsVar() override { return true; }

    virtual void Display(std::ostream &os, unsigned lv = 0) {
        // type_->Display(os, 0);
        os << name_;
    }

    bool is_externed_ = false;
    bool is_left_ = true;
    int offset_ = 0;
    IRType *type_ = nullptr;
    IRExpr *initial_val_ = nullptr;
    IRScope *scope_ = nullptr;  // need?
    std::string name_;
};

class IRUnary : public IRExpr {
public:
    IRUnary(IRExpr *rval, KeyWord op) : op_(op), rval_(rval) {}

    OPERATOR_NEW

    virtual void Display(std::ostream &os, unsigned lv = 0) {
        os << KeyWordToStr(op_);
        rval_->Display(os, 0);
    }

    KeyWord op_ = KW_NONE;
    IRExpr *rval_ = nullptr;
};

class IRBinary : public IRExpr {
public:
    IRBinary(IRExpr *lval, IRExpr *rval, KeyWord op) : op_(op), lval_(lval), rval_(rval) {}

    OPERATOR_NEW

    virtual void Display(std::ostream &os, unsigned lv = 0) {
        lval_->Display(os, lv);
        os << " " << KeyWordToStr(op_) << " ";
        rval_->Display(os, lv);
    }

    KeyWord op_ = KW_NONE;
    IRExpr *lval_ = nullptr;
    IRExpr *rval_ = nullptr;
};

class IRFuncCall : public IRExpr {
public:
    OPERATOR_NEW

    virtual void Display(std::ostream &os, unsigned lv = 0);

    IRFunc *func_ = nullptr;
    std::vector<IRExpr *> args_;
};

/* scope-------------------------------------------------------------
*/

class IRScope {
public:
    IRScope(const std::string &name) : name_(name) {}

    IRScope(std::string &&name) : name_(std::move(name)) {}

    IRVar *CreateNewVar(const std::string &name, IRType *type) {
        IRVar *new_node = nullptr;
        if (name.empty()) {
            auto unique_name = GetUniqueName();
            new_node = new IRVar(unique_name);
            name_to_vars_[unique_name] = new_node;
        } else {
            assert(!name_to_vars_.count(name));
            new_node = new IRVar(name);
            name_to_vars_[name] = new_node;  // todo: should allow same name in different layer's scope
        }
        vars_.push_back(new_node);

        assert(type);
        new_node->type_ = type;
        unsigned len = type->GetSize();
        unsigned alignment = std::min(len, (unsigned) ALIGN_VAL);
        unsigned padding = alignment - size_ % alignment;
        new_node->offset_ = size_ + padding;
        size_ = size_ + padding + len;

        largest_len_ = std::max(largest_len_, len);

        return new_node;
    }

    IRVar *SearchVar(const std::string &name) {
        auto it = name_to_vars_.find(name);
        if (it != name_to_vars_.end()) {
            return it->second;
        } else {
            return nullptr;
        }
    }

    void Align() {
        if (size_ == 0) {
            return;
        }
        unsigned alignment = std::min((unsigned) ALIGN_VAL, largest_len_);
        if (size_ % alignment) {
            size_ += alignment - size_ % alignment;
        }
    }

    virtual bool IsCodeBlock() { return false; }

    virtual bool IsFunc() { return false; }

    virtual void Display(std::ostream &os, unsigned lv) {
        os << "empty scope" << std::endl;
    }

protected:
    std::string GetUniqueName() { return "unique_var_" + ++unique_var_idx_; }

    friend class SymTable;

public:
    std::string name_;
    std::vector<IRScope *> scopes_;
    std::vector<IRVar *> vars_;
    std::unordered_map<std::string, IRVar *> name_to_vars_;
    // std::vector<InterInst*> inter_insts_;
    IRScope *upper_ = nullptr;
    unsigned unique_var_idx_ = 0;
    unsigned size_ = 0;   // actual size
    unsigned largest_len_ = 0;
};

/* class function
class A; A a; a.Getname()
need to add class scope name because of virtual table, cache var member in cls
*/
class IRClass : public IRScope {
    OPERATOR_NEW
};

class IRCodeBlock : public IRScope, public IRStmt {
    // global scope is named as "Global_Scope"
public:
    IRCodeBlock(const std::string &name) : IRScope(name) {}

    IRCodeBlock(std::string &&name) : IRScope(std::move(name)) {}

    OPERATOR_NEW

    bool IsCodeBlock() override { return true; }

    bool IsGlobal() { return is_global_; }

    virtual void Display(std::ostream &os, unsigned lv = 0) {
        std::string spaces(lv, ' ');
        if (!is_global_) {
            os << spaces << "{" << std::endl;
        }
        for (auto i: stmts_) {
            i->Display(os, is_global_ ? 0 : lv + 4);
        }
        if (!is_global_) {
            os << spaces << "}" << std::endl;
        }
    }

    bool is_global_ = false;  // if this is global namespace?
    std::vector<IRStmt *> stmts_;
};

class IRFunc : public IRCodeBlock {
public:
    IRFunc(const std::string &name) : IRCodeBlock(name) {}

    IRFunc(std::string &&name) : IRCodeBlock(std::move(name)) {}

    OPERATOR_NEW

    bool IsFunc() override { return true; }

    virtual void Display(std::ostream &os, unsigned lv = 0);

    IRType *ret_type_ = nullptr;
    std::vector<IRVar *> params_;
};


class SymTable {
public:
    void Enter(std::string &&scope_name, bool is_func = false) {
        IRScope *new_scope = nullptr;
        if (is_func) {
            new_scope = new IRFunc(scope_name);

            // todo: need to support function overloading
            assert(!name_to_func_.count(scope_name));
            name_to_func_[scope_name] = (IRFunc *) new_scope;
        } else {
            new_scope = new IRCodeBlock(std::move(scope_name));
        }
        if (cur_scope_) {
            cur_scope_->scopes_.push_back(new_scope);
            new_scope->upper_ = cur_scope_;
        }
        cur_scope_ = new_scope;
    }

    void Leave() {
        cur_scope_->Align();
        cur_scope_ = cur_scope_->upper_ ? cur_scope_->upper_ : 0;
    }

    IRScope *GetCurScope() { return cur_scope_; }

    IRVar *SearchVar(const std::string &var_name) {
        IRScope *scope = cur_scope_;
        IRVar *var = nullptr;
        while (scope) {
            var = scope->SearchVar(var_name);
            if (var) return var;
            scope = scope->upper_;
        }
        return nullptr;
    }

    IRFunc *SearchFunc(const std::string &func_name) {
        auto it = name_to_func_.find(func_name);
        if (it != name_to_func_.end()) {
            return it->second;
        } else {
            return nullptr;
        }
    }

private:
    IRScope *gloabl_scope_ = nullptr;
    IRScope *cur_scope_ = nullptr;
    std::unordered_map<std::string, IRFunc *> name_to_func_;
};

// stmt --------------------------------------------------

class IRVarDecl : public IRStmt {
public:
    OPERATOR_NEW

    virtual void Display(std::ostream &os, unsigned lv = 0);

    std::vector<IRVar *> vars_;
};

class IRFuncDecl : public IRStmt {
public:
    OPERATOR_NEW

    virtual void Display(std::ostream &os, unsigned lv = 0) {
        func_->Display(os, lv);
    }

    IRFunc *func_ = nullptr;
};

/*
represented by binary expression stmt
class IRAssign : public IRStmt {
public:
   IRExpr* left_;
   IRExpr* right_;
};*/

class IRExprStmt : public IRStmt {
public:
    OPERATOR_NEW

    virtual void Display(std::ostream &os, unsigned lv = 0) {
        std::string spaces(lv, ' ');
        os << spaces;
        expr_->Display(os, 0);
        os << ";";
    }

    IRExpr *expr_ = nullptr;
};

class IRIfElse : public IRStmt {
public:
    OPERATOR_NEW

    virtual void Display(std::ostream &os, unsigned lv = 0) {
        std::string spaces(lv, ' ');
        os << spaces << "if (";
        condition_->Display(os, 0);
        os << ") ";
        then_stmt_->Display(os, lv + 4);
        os << "then ";
        else_stmt_->Display(os, lv + 4);
    }

    IRExpr *condition_ = nullptr;
    IRStmt *then_stmt_ = nullptr;
    IRStmt *else_stmt_ = nullptr;
};

// todo: other loop stmts
class IRFor : public IRStmt {
public:
    OPERATOR_NEW

    virtual void Display(std::ostream &os, unsigned lv = 0) {
        std::string spaces(lv, ' ');
        os << spaces << "for (";
        init_->Display(os, 0);
        os << "; ";
        condition_->Display(os, 0);
        os << "; ";
        iteration_->Display(os, 0);
        os << ") ";
        for_body_->Display(os, lv);
    }

    IRStmt *init_ = nullptr;
    IRExpr *condition_ = nullptr;
    IRStmt *iteration_ = nullptr;
    IRStmt *for_body_ = nullptr;
};

class IRWhile : public IRStmt {
public:
    OPERATOR_NEW

    virtual void Display(std::ostream &os, unsigned lv = 0) {
        std::string spaces(lv, ' ');
        os << spaces << "while (";
        condition_->Display(os, 0);
        os << ")";
        while_body_->Display(os, lv);
    }
    IRExpr *condition_ = nullptr;
    IRStmt* while_body_ = nullptr;
};

// jump stmts
class IRBreak : public IRStmt {
public:
    virtual void Display(std::ostream &os, unsigned lv = 0) {
        std::string spaces(lv, ' ');
        os << spaces << "break;";
    }

    OPERATOR_NEW
};

class IRContinue : public IRStmt {
public:
    OPERATOR_NEW

    virtual void Display(std::ostream &os, unsigned lv = 0) {
        std::string spaces(lv, ' ');
        os << spaces << "continue;";
    }
};

class IRReturn : public IRStmt {
public:
    OPERATOR_NEW

    virtual void Display(std::ostream &os, unsigned lv = 0) {
        std::string spaces(lv, ' ');
        os << spaces << "return ";
        if (ret_val_) {
            ret_val_->Display(os, 0);
        }
        os << ";";
    }

    IRExpr *ret_val_ = nullptr;
};

class IRNull : public IRStmt {
public:
    OPERATOR_NEW

    virtual void Display(std::ostream &os, unsigned lv = 0) {
        std::string spaces(lv, ' ');
        os << spaces << ";";
    }
};

class ParseIR {
public:
    ParseIR() { sym_table_ = new SymTable(); }

    void DoLexemeInterprete(FILE *file_in, std::string filename) {
        LexemeInterpreter interpreter(file_in, filename);
        interpreter.Interprete();
        tokens_ = interpreter.GetTokens();
        cur_pos_ = 0;
        // iter = tokens_.begin();
    }

    IRCodeBlock *SyntaxInpterprete() {
        global_block_ = ParseGlobalBlock();
        return global_block_;
    }

    ~ParseIR() {
        for (auto i: tokens_) delete i;
        delete sym_table_;
    }

    // todo: tail is nullptr!
    TinyToken *step() {
        if (cur_pos_ >= tokens_.size()) {
            return nullptr;
        } else {
            return tokens_[cur_pos_++];
            // return *(iter++);
        }
    }

    TinyToken *peek() {
        if (cur_pos_ >= tokens_.size()) {
            return nullptr;
        } else {
            return tokens_[cur_pos_];
            // return *(iter);
        }
    }

    // statements
    IRStmt *ParseStmt();

    IRStmt *ParseDecl(bool is_extern = false);

    IRStmt *ParseDeclTail(IRType *type, bool is_func_port = false, IRVarDecl *var_decl = nullptr);

    IRCodeBlock *ParseBlock();

    IRCodeBlock *ParseGlobalBlock();  // outermost entry

    // exprs
    IRExpr *ParseExpr();

    IRExpr *ParseUnary(IRExpr *expr, KeyWord op);

    IRExpr *ParseBinary(IRExpr *left, IRExpr *right, KeyWord op);

    IRType *ParseType();

    IRVar *ParsePtr(IRVar *var);

    IRVar *ParseLea(IRVar *var);

private:
    SymTable *sym_table_ = nullptr;
    IRCodeBlock *global_block_ = nullptr;
    std::vector<TinyToken *> tokens_;
    unsigned cur_pos_ = 0;
    // std::vector<TinyToken *>::iterator iter;
};
