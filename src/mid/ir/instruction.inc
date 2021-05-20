// reference: LLVM 2.0

// Provide definitions of macros so that users of this file do not have to
// define everything to use it...
//
#ifndef FIRST_TERM_INST
#define FIRST_TERM_INST(num)
#endif
#ifndef HANDLE_TERM_INST
#ifndef HANDLE_INST
#define HANDLE_TERM_INST(num, opcode, Class)
#else
#define HANDLE_TERM_INST(num, opcode, Class) HANDLE_INST(num, opcode, Class)
#endif
#endif
#ifndef LAST_TERM_INST
#define LAST_TERM_INST(num)
#endif

#ifndef FIRST_BINARY_INST
#define FIRST_BINARY_INST(num)
#endif
#ifndef HANDLE_BINARY_INST
#ifndef HANDLE_INST
#define HANDLE_BINARY_INST(num, opcode, instclass)
#else
#define HANDLE_BINARY_INST(num, opcode, Class) HANDLE_INST(num, opcode, Class)
#endif
#endif
#ifndef LAST_BINARY_INST
#define LAST_BINARY_INST(num)
#endif

#ifndef FIRST_MEMORY_INST
#define FIRST_MEMORY_INST(num)
#endif
#ifndef HANDLE_MEMORY_INST
#ifndef HANDLE_INST
#define HANDLE_MEMORY_INST(num, opcode, Class)
#else
#define HANDLE_MEMORY_INST(num, opcode, Class) HANDLE_INST(num, opcode, Class)
#endif
#endif
#ifndef LAST_MEMORY_INST
#define LAST_MEMORY_INST(num)
#endif

#ifndef FIRST_CAST_INST
#define FIRST_CAST_INST(num)
#endif
#ifndef HANDLE_CAST_INST
#ifndef HANDLE_INST
#define HANDLE_CAST_INST(num, opcode, Class)
#else
#define HANDLE_CAST_INST(num, opcode, Class) HANDLE_INST(num, opcode, Class)
#endif
#endif
#ifndef LAST_CAST_INST
#define LAST_CAST_INST(num)
#endif

#ifndef FIRST_OTHER_INST
#define FIRST_OTHER_INST(num)
#endif
#ifndef HANDLE_OTHER_INST
#ifndef HANDLE_INST
#define HANDLE_OTHER_INST(num, opcode, Class)
#else
#define HANDLE_OTHER_INST(num, opcode, Class) HANDLE_INST(num, opcode, Class)
#endif
#endif
#ifndef LAST_OTHER_INST
#define LAST_OTHER_INST(num)
#endif

#ifndef FIRST_ASSIGN_INST
#define FIRST_ASSIGN_INST(num)
#endif
#ifndef HANDLE_ASSIGN_INST
#ifndef HANDLE_INST
#define HANDLE_ASSIGN_INST(num, opcode, Class)
#else
#define HANDLE_ASSIGN_INST(num, opcode, Class) HANDLE_INST(num, opcode, Class)
#endif
#endif
#ifndef LAST_ASSIGN_INST
#define LAST_ASSIGN_INST(num)
#endif

// Terminator Instructions - These instructions are used to terminate a basic
// block of the program.   Every basic block must end with one of these
// instructions for it to be a well formed basic block.
//


  FIRST_TERM_INST ( 1)
HANDLE_TERM_INST  ( 1, Ret        , ReturnInst)
HANDLE_TERM_INST  ( 2, Br         , BranchInst)
HANDLE_TERM_INST  ( 3, Jmp        , JumpInst)
  LAST_TERM_INST  ( 3)

// Standard binary operators...
 FIRST_BINARY_INST( 4)
HANDLE_BINARY_INST( 4, Add  , BinaryOperator)
HANDLE_BINARY_INST( 5, Sub  , BinaryOperator)
HANDLE_BINARY_INST( 6, Mul  , BinaryOperator)
HANDLE_BINARY_INST( 7, SDiv , BinaryOperator)
HANDLE_BINARY_INST( 8, UDiv , BinaryOperator)
HANDLE_BINARY_INST( 9, SRem , BinaryOperator)
HANDLE_BINARY_INST(10, URem , BinaryOperator)

// Logical operators (integer operands)
HANDLE_BINARY_INST(11, And  , BinaryOperator)
HANDLE_BINARY_INST(12, Or   , BinaryOperator)
HANDLE_BINARY_INST(13, Xor  , BinaryOperator)
HANDLE_BINARY_INST(14, Shl  , BinaryOperator) // Shift left  (logical)
HANDLE_BINARY_INST(15, AShr , BinaryOperator) // shift right (arithmetic)
HANDLE_BINARY_INST(16, LShr , BinaryOperator) // Shift right (logical)
  LAST_BINARY_INST(16)

// Memory operators...
 FIRST_MEMORY_INST(17)
HANDLE_MEMORY_INST(17, Alloca, AllocaInst)  // Stack management
HANDLE_MEMORY_INST(18, Load  , LoadInst  )  // Memory manipulation instrs
HANDLE_MEMORY_INST(19, Store , StoreInst )
HANDLE_MEMORY_INST(20, Access, AccessInst)  // GetElementPtr in llvm
HANDLE_MEMORY_INST(21, Malloc, MallocInst)  // TODO: Heap management instructions
HANDLE_MEMORY_INST(22, Free  , FreeInst  )  // TODO: Heap management instructions
  LAST_MEMORY_INST(22)

// Cast operators ...
// NOTE: The order matters here because CastInst::isEliminableCastPair
// NOTE: (see Instructions.cpp) encodes a table based on this ordering.
 FIRST_CAST_INST(23)
HANDLE_CAST_INST(23, Trunc   , TruncInst   )  // Truncate integers
HANDLE_CAST_INST(24, ZExt    , ZExtInst    )  // Zero extend integers
HANDLE_CAST_INST(25, SExt    , SExtInst    )  // Sign extend integers
HANDLE_CAST_INST(26, PtrToInt, PtrToIntInst)  // Pointer -> Integer
HANDLE_CAST_INST(27, IntToPtr, IntToPtrInst)  // Integer -> Pointer
HANDLE_CAST_INST(28, BitCast , BitCastInst )  // Type cast
  LAST_CAST_INST(28)

// Other operators...
 FIRST_OTHER_INST(29)
HANDLE_OTHER_INST(29, ICmp   , ICmpInst   )  // Integer comparison instruction
HANDLE_OTHER_INST(30, PHI    , PHINode    )  // PHI node instruction
HANDLE_OTHER_INST(31, Call   , CallInst   )  // Call a function
HANDLE_OTHER_INST(32, Select , SelectInst )  // select instruction
HANDLE_OTHER_INST(33, UserOp1, Instruction)  // May be used internally in a pass
HANDLE_OTHER_INST(34, UserOp2, Instruction)  // Internal to passes only
HANDLE_OTHER_INST(35, VAArg  , VAArgInst  )  // vaarg instruction
HANDLE_OTHER_INST(36, ExtractElement, ExtractElementInst)// extract from vector.
HANDLE_OTHER_INST(37, InsertElement, InsertElementInst)  // insert into vector
HANDLE_OTHER_INST(38, ShuffleVector, ShuffleVectorInst)  // shuffle two vectors.
HANDLE_OTHER_INST(39, Undef,   Undef)   // Assign with other operator
  LAST_OTHER_INST(39)

 FIRST_ASSIGN_INST(40)
HANDLE_ASSIGN_INST(40, Assign,  BinaryOperator)   // load and store
HANDLE_ASSIGN_INST(41, AssAdd,  BinaryOperator)   // Assign with add operator
HANDLE_ASSIGN_INST(42, AssSub,  BinaryOperator)   // Assign with sub operator
HANDLE_ASSIGN_INST(43, AssMul,  BinaryOperator)   // Assign with mul operator
HANDLE_ASSIGN_INST(44, AssSDiv, BinaryOperator)   // Assign with sdiv operator
HANDLE_ASSIGN_INST(45, AssUDiv, BinaryOperator)   // Assign with udiv operator
HANDLE_ASSIGN_INST(46, AssSRem, BinaryOperator)   // Assign with srem operator
HANDLE_ASSIGN_INST(47, AssURem, BinaryOperator)   // Assign with urem operator
HANDLE_ASSIGN_INST(48, AssAnd,  BinaryOperator)   // Assign with and operator
HANDLE_ASSIGN_INST(49, AssOr,   BinaryOperator)   // Assign with or operator
HANDLE_ASSIGN_INST(50, AssXor,  BinaryOperator)   // Assign with xor operator
HANDLE_ASSIGN_INST(51, AssShl,  BinaryOperator)   // Assign with shl operator
HANDLE_ASSIGN_INST(52, AssAShr, BinaryOperator)   // Assign with ashr operator
HANDLE_ASSIGN_INST(53, AssLShr, BinaryOperator)   // Assign with lshr operator
  LAST_ASSIGN_INST(53)


#undef  FIRST_TERM_INST
#undef HANDLE_TERM_INST
#undef   LAST_TERM_INST

#undef  FIRST_BINARY_INST
#undef HANDLE_BINARY_INST
#undef   LAST_BINARY_INST

#undef  FIRST_MEMORY_INST
#undef HANDLE_MEMORY_INST
#undef   LAST_MEMORY_INST

#undef  FIRST_CAST_INST
#undef HANDLE_CAST_INST
#undef   LAST_CAST_INST

#undef  FIRST_OTHER_INST
#undef HANDLE_OTHER_INST
#undef   LAST_OTHER_INST

#undef  FIRST_ASSIGN_INST
#undef HANDLE_ASSIGN_INST
#undef   LAST_ASSIGN_INST

#ifdef HANDLE_INST
#undef HANDLE_INST
#endif