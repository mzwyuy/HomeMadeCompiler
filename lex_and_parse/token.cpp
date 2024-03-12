#include "token.h"

std::unordered_map<string, KeyWord> TinyToken::token_to_keyword_= {
    /*{"none", KW_NONE}, {"err", KW_ERR}, {"end", KW_END}, {"id", KW_ID}, {"num", KW_NUM},
    {"str", KW_STR},*/

    {"long", KW_LONG}, {"unsigned long", KW_ULONG}, {"int", KW_INT},  {"unsigned", KW_UINT},
    {"char", KW_CHAR}, {"bool", KW_BOOL}, {"void", KW_VOID},

    {"extern", KW_EXTERN}, {"const", KW_CONST}, {"if", KW_IF}, {"else", KW_ELSE},
    {"switch", KW_SWITCH}, {"case", KW_CASE}, {"default", KW_DEFAULT}, {"while", KW_WHILE},
    {"do", KW_DO}, {"for", KW_FOR}, {"break", KW_BREAK},
    {"continue", KW_CONTINUE}, {"return", KW_RETURN},

    {"+", KW_ADD}, {"-", KW_SUB}, {"*", KW_MUL},
    {"/", KW_DIV}, {"%", KW_MOD}, {"++", KW_LINC}, {"--", KW_LDEC}, {"!", KW_NOT},
    {"&", KW_LEA}, {"&&", KW_AND}, {"||", KW_OR}, {"=", KW_ASSIGN}, {">", KW_GT},
    {">=", KW_GE}, {"<", KW_LT}, {"<=", KW_LE}, {"==",KW_EQUAL}, {"!=", KW_NEQUAL},
    {",", KW_COMMA}, {":", KW_COLON}, {";", KW_SEMICOLON}, {"(", KW_LPAREN}, {")", KW_RPAREN},
    {"{", KW_LBRAC}, {"}", KW_RBRAC}, {"[", KW_LSBRAC}, {"]", KW_RSBRAC}
    // KW_NEGATE, KW_DEREF, KW_RDEC, KW_RINC
    // todo: ~, sizeof()
};

std::unordered_set<char> TinyToken::single_char_keyword_ = {'!', '-', '&', '*', '+', '/', '%', '=', '>', '<', ',', ':',
                                                            ';', '(', ')', '{', '}', '[', ']', '|'};