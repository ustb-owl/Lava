#ifndef CLASS_ID
#define CLASS_ID(id, opcode, name)
#endif


#define HANDLE_TERM_INST(N, OPC, CLASS) CLASS_ID(N, OPC, CLASS)
#include "mid/ir/instruction.inc"

#define FIRST_BINARY_INST(N) CLASS_ID(N, , BinaryOperator)
#include "mid/ir/instruction.inc"

#define HANDLE_MEMORY_INST(N, OPC, CLASS) CLASS_ID(N, OPC, CLASS)
#include "mid/ir/instruction.inc"

#define FIRST_CAST_INST(N) CLASS_ID(N, , CastInst)
#include "mid/ir/instruction.inc"

#define HANDLE_OTHER_INST(N, OPC, CLASS) CLASS_ID(N, OPC, CLASS)
#include "mid/ir/instruction.inc"

#define HANDLE_ASSIGN_INST(N, OPC, CLASS) CLASS_ID(N, , OPC)
#include "mid/ir/instruction.inc"

CLASS_ID(66, , Function)
CLASS_ID(67, , BasicBlock)
CLASS_ID(68, , ArgRefSSA)
CLASS_ID(69, , GlobalVariable)
CLASS_ID(70, , ConstantInt)
CLASS_ID(71, , ConstantArray)
CLASS_ID(72, , ConstantString)

// class for LLIR
CLASS_ID(80, , LLFunction)
CLASS_ID(81, , LLBasicBlock)
CLASS_ID(82, , LLOperand)
CLASS_ID(83, , LLInst)
CLASS_ID(84, , LLBinaryInst)
CLASS_ID(85, , LLMove)
CLASS_ID(86, , LLJump)
CLASS_ID(87, , LLReturn)
CLASS_ID(88, , LLLoad)
CLASS_ID(89, , LLStore)
CLASS_ID(90, , LLCompare)
CLASS_ID(91, , LLCall)
CLASS_ID(92, , LLComment)
CLASS_ID(93, , LLFMA)
CLASS_ID(94, , LLMLA)
CLASS_ID(95, , LLMLS)
CLASS_ID(96, , LLBranch)



#undef CLASS_ID
