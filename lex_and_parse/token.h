#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <list>
#include "buffer_read.h"

using std::string;

enum KeyWord {
    KW_NONE = 0, KW_ERR, KW_END, KW_ID, KW_NUM,
    /*used in type or decl*/
    KW_STR, KW_LONG, KW_INT, KW_CHAR, KW_BOOL, KW_VOID,
    KW_EXTERN, KW_CONST,
    /*used in stmt*/
    KW_IF, KW_ELSE, KW_SWITCH, KW_CASE, KW_WHILE, KW_DO, KW_FOR, KW_BREAK, KW_CONTINUE, KW_RETURN,
    /*used in unary expr*/
    KW_NOT, KW_NEGATE, KW_LEA, KW_DEREF, KW_INC, KW_DEC, KW_RDEC, KW_RINC,
    /*used in binary expr*/
    KW_ADD, KW_SUB, KW_MUL, KW_DIV, KW_MOD,
    KW_AND, KW_OR, KW_ASSIGN, KW_GT,
    KW_GE, KW_LT, KW_LE, KW_EQUAL, KW_NEQUAL,
    /*generic tokens*/
    KW_COMMA/*,*/, KW_COLON/*:*/, KW_SEMICOLON/*;*/,
    KW_LPAREN, KW_RPAREN/*()*/,
    KW_LBRAC, KW_RBRAC/*{}*/, KW_LSBRAC, KW_RSBRAC/*[]*/,

    KW_ASTERISK, KW_DEFAULT,
};

extern const char *KeyWordToStr(KeyWord key_word);

enum LexEorror {
};

class TinyToken {
public:
    TinyToken() {}

    virtual ~TinyToken() {}

    virtual string ToString() = 0;

    // no need to provide this in abstruct class!
    static KeyWord GetKeyWord(string str) {
        auto it = token_to_keyword_.find(str);
        if (it == token_to_keyword_.end()) {
            return KW_NONE;
        } else {
            return it->second;
        }
    }

    static bool IsSingleKeyWord(char ch) {
        return single_char_keyword_.count(ch);
    }

    virtual bool IsId() { return false; }

    virtual string GetId() { return ""; }

    virtual bool IsKeyWord() { return false; }

    virtual KeyWord GetKeyWord() { return KW_NONE; }

    virtual bool IsChar() { return false; }

    virtual bool IsNum() { return false; }

    virtual long GetNum() { return 0; }

    virtual unsigned GetPriority() { return 0; }

    virtual bool IsUnary() { return false; }

    virtual bool IsBinary() { return false; }

private:
    static std::unordered_map<string, KeyWord> token_to_keyword_;
    static std::unordered_set<char> single_char_keyword_;
};


class TinyId : public TinyToken {
public:
    TinyId(const string &str) : str_(str) {}

    TinyId(string &&str) : str_(std::move(str)) {}  // also can be {} here
    // ~TinyId() {}
    string ToString() { return str_; }

    bool IsId() { return true; }

    string GetId() { return str_; }

private:
    string str_;
};

class TinyKeyWord : public TinyToken {
public:
    TinyKeyWord(KeyWord key_word) : key_word_(key_word) {}

    // ~TinyId() {}
    string ToString() { return ""; }

    bool IsKeyWord() { return true; }

    KeyWord GetKeyWord() { return key_word_; }

    unsigned GetPriority() override {
        switch (key_word_) {
            // (
            case KW_LPAREN:
                return 11;
                // =
            case KW_ASSIGN:
                return 10;
                // ||
            case KW_OR:
                return 9;
                // &&
            case KW_AND:
                return 8;
                // > < >= <= == !=
            case KW_GT:
            case KW_LT:
            case KW_GE:
            case KW_LE:
            case KW_EQUAL:
            case KW_NEQUAL:
                return 7;
                // + -
            case KW_ADD:
            case KW_SUB:
                return 6;
                // * / %
            case KW_MUL:
            case KW_DIV:
            case KW_MOD:
                return 5;
                // ! - & * ++ --
            case KW_NOT:
            case KW_NEGATE:
            case KW_LEA:
            case KW_DEREF:
            case KW_INC:
            case KW_DEC:
                return 4;
                // ++ --
            case KW_RINC:
            case KW_RDEC:
                return 3;
                // ()
            case KW_RPAREN:
                return 2;
                // []
            case KW_LBRAC:
            case KW_RBRAC:
                return 1;
            default:
                return 0;
        }
    }

    // left unary: 4, right unary: 3
    bool IsUnary() override {  // unary operator
        unsigned priority = GetPriority();
        return priority == 4 || priority == 3;
    }

    bool IsBinary() override {  // binary operator
        unsigned priority = GetPriority();
        return priority && priority != 4 && priority != 3;
    }

private:
    KeyWord key_word_;
};

using TinyStr = TinyId;

class TinyChar : public TinyToken {
public:
    TinyChar(char ch) : ch_(ch) {}

    // ~TinyChar() {}
    string ToString() { return string(1, ch_); }

    bool IsChar() { return true; }

private:
    char ch_;
};

class TinyNum : public TinyToken {
public:
    TinyNum(long num) : num_(num) {}

    string ToString() { return std::to_string(num_); }

    bool IsNum() { return true; }

    long GetNum() { return num_; }

private:
    long num_;
};

class LexemeInterpreter {
public:
    LexemeInterpreter(FILE *file_in, std::string filename_in) : buffer_read_(file_in, filename_in) {}

    void Interprete() {
        char ch;
        std::string str;
        while ((ch = buffer_read_.GetChar())) {
            str.clear();
            if (IsAlpha(ch) || ch == '_') {
                while (IsAlpha(ch) || ch == '_' || IsNum(ch)) {
                    // todo: match comment /* and //
                    str.push_back(ch);
                    ch = buffer_read_.AdvanceAndGetChar();
                }
                // int calculate(int a, int b);

                auto key_word = TinyToken::GetKeyWord(str);
                if (key_word) {
                    if ((key_word == KW_SUB || key_word == KW_MUL) && tokens_.back()->GetKeyWord()) {
                        // unary operator
                        key_word = key_word == KW_SUB ? KW_NEGATE : KW_DEREF;
                    }
                    tokens_.push_back(new TinyKeyWord(key_word));
                } else {
                    tokens_.push_back(new TinyId(std::move(str)));
                }
            } else if (IsEmpty(ch)) {
                while (IsEmpty(ch)) {
                    ch = buffer_read_.AdvanceAndGetChar();
                }
            } else if (IsNum(ch)) {
                // todo: 0x, 0b, 0o,   RaiseError("Invalid num.");
                long num = 0;
                while (IsNum(ch)) {
                    num = num * 10 + ch - '0';
                    ch = buffer_read_.AdvanceAndGetChar();
                }
                tokens_.push_back(new TinyNum(num));
            } else if (ch == '\"') {
                ch = buffer_read_.AdvanceAndGetChar();
                while (ch != '"') {
                    if (ch == '\\') {
                        ch = buffer_read_.AdvanceAndGetChar();
                        if (ch == '\\') {
                            str.push_back('\\');
                        } else if (ch == 'n') {
                            str.push_back('\n');
                        } else if (ch == 't') {
                            str.push_back('\t');
                        } else if (ch == '"') {
                            str.push_back('"');
                        } else if (ch == '\'') {
                            str.push_back('\'');
                        } else if (ch == '\n') {
                            RaiseError("Not support \n.");
                        } else if (ch == -1) {
                            RaiseError("Invalid string: -1.");
                        } else {
                            str.push_back('\\' + ch);
                        }
                    } else if (ch == '\n' || ch == -1) {
                        RaiseError("Invalid string: file end or without right \".");
                    } else {
                        str.push_back(ch);
                    }
                    ch = buffer_read_.AdvanceAndGetChar();
                }
                tokens_.push_back(new TinyStr(str));
            } else if (ch == -1) {
                // reach the end of file
                return;
            } else if (TinyToken::IsSingleKeyWord(ch)) {
                // some keywords don't need space to separate
                char next_ch = buffer_read_.AdvanceAndGetChar();
                str.push_back(ch);
                if ((ch == '+' && next_ch == '+') || (ch == '-' && next_ch == '-') || (ch == '>' && next_ch == '=') ||
                    (ch == '<' && next_ch == '=') || (ch == '&' && next_ch == '&') || (ch == '|' && next_ch == '|') ||
                    (ch == '=' && next_ch == '=') || (ch == '!' && next_ch == '=')) {
                    // ++, -- && || >= <= ==
                    str.push_back(next_ch);
                    ch = buffer_read_.AdvanceAndGetChar();
                } else if (ch == '/' && next_ch == '*') {
                    // todo: need to add IRComment node
                    ch = buffer_read_.AdvanceAndGetChar();
                    next_ch = buffer_read_.AdvanceAndGetChar();
                    while (!(ch == '*' && next_ch == '/')) {
                        ch = next_ch;
                        next_ch = buffer_read_.AdvanceAndGetChar();
                    }
                    ch = buffer_read_.AdvanceAndGetChar();
                    continue;
                } else {
                    // ( ) { } [ ] ...
                    ch = next_ch;
                }
                auto key_word = TinyToken::GetKeyWord(str);
                assert(key_word != 0);
                tokens_.push_back(new TinyKeyWord(key_word));
            } else {
                RaiseError("Unknown char.\n");
            }
        }
    }

    bool IsAlpha(char ch) {
        return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
    }

    bool IsNum(char ch) {
        return ch >= '0' && ch <= '9';
    }

    bool IsEmpty(char ch) {
        return ch == ' ' || ch == '\t' || ch == '\n';
    }

    bool IsEmptyOrEnd(char ch) {
        return IsEmpty(ch) || ch == -1;
    }

    void RaiseError(const char *cstr) {
        // todo: need improve! need to add OriginLoc
        std::cout << cstr << std::endl;
        assert(0);
    }

    std::list<TinyToken *> &&GetTokens() { return std::move(tokens_); }

private:
    BufferRead buffer_read_;
    std::list<TinyToken *> tokens_;
};