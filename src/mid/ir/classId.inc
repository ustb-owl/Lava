#ifndef CLASS_ID
#define CLASS_ID(id, opcode, name)
#endif


#define HANDLE_TERM_INST(N, OPC, CLASS) CLASS_ID(N, OPC, CLASS)
#include "instruction.inc"

#define FIRST_BINARY_INST(N) CLASS_ID(N, , BinaryOperator)
#include "instruction.inc"

#define HANDLE_MEMORY_INST(N, OPC, CLASS) CLASS_ID(N, OPC, CLASS)
#include "instruction.inc"

#define FIRST_CAST_INST(N) CLASS_ID(N, , CastInst)
#include "instruction.inc"

#define HANDLE_OTHER_INST(N, OPC, CLASS) CLASS_ID(N, OPC, CLASS)
#include "instruction.inc"

#define HANDLE_ASSIGN_INST(N, OPC, CLASS) CLASS_ID(N, , OPC)
#include "instruction.inc"

CLASS_ID(66, , Function)
CLASS_ID(67, , BasicBlock)
CLASS_ID(68, , ArgRefSSA)
CLASS_ID(69, , GlobalVariable)
CLASS_ID(70, , ConstantInt)
CLASS_ID(71, , ConstantArray)
CLASS_ID(72, , ConstantString)


#undef CLASS_ID
