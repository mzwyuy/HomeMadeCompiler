#include "IRNode.h"

enum InstructionOperator {
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

class InterInst {
public:
   InterInst(InstructionOperator op, IRVar* result, IRVar* left, IRVar* right = nullptr) : op_(op), result_(result), left_(left), right_(right) {}
   string label_;
   InstructionOperator op_;
   IRVar* result_;
   IRVar* left_;
   IRVar* right_;
   IRFunc* func_;
   InterInst* jmp_target_;
};