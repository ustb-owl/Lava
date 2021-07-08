#ifndef LAVA_INSTDEF_H
#define LAVA_INSTDEF_H

/* Low-Level Intermediate representation (LLIR) */

#include <array>
#include <utility>
#include "mid/ir/ssa.h"
#include "common/classid.h"


#define CLASSOF(CLS) \
  static inline bool classof(CLS *) { return true; } \
  static inline bool classof(const CLS *) { return true; }

#define CLASSOF_INST(CLS) \
  static inline bool classof(LLInst *inst) { return inst->classId() == ClassId::CLS##Id; } \
  static inline bool classof(const LLInst *inst) { return inst->classId() == ClassId::CLS##Id; }

namespace lava::back {

// ref: https://en.wikipedia.org/wiki/Calling_convention#ARM_(A32)
enum class ArmReg {
  // args and return value (caller saved)
  r0,
  r1,
  r2,
  r3,
  // local variables (callee saved)
  r4,
  r5,
  r6,
  r7,
  r8,
  r9,
  r10,
  r11,
  // special purposes
  r12,
  r13,
  r14,
  r15,
  // some aliases
  fp = r11,  // frame pointer (omitted), allocatable
  ip = r12,  // ipc scratch register, used in some instructions (caller saved)
  sp = r13,  // stack pointer
  lr = r14,  // link register (caller saved)
  pc = r15,  // program counter
};

enum class ArmCond { Any, Eq, Ne, Ge, Gt, Le, Lt };

class ArmShift {
public:
  enum class ShiftType {
    // no shifting
    None,
    // arithmetic right
    Asr,
    // logic left
    Lsl,
    // logic right
    Lsr,
    // rotate right
    Ror,
    // rotate right one bit with extend
    Rrx
  };

private:
  int       _shift;
  ShiftType _type;

public:
  ArmShift() : _shift(0), _type(ShiftType::None) {}
  ArmShift(int shift, ShiftType type) : _shift(shift), _type(type) {}

  bool is_none() const { return _type == ShiftType::None; }

  // getter/setter
  int shift()      const { return _shift; }
  ShiftType type() const { return _type;  }

  void setShift(int shift)     { _shift = shift; }
  void setType(ShiftType type) { _type = type;   }

  explicit operator std::string() const;
};

std::ostream &operator<<(std::ostream &os, const ArmShift &shift);

std::ostream &operator<<(std::ostream &os, const ArmCond &cond);

class LLFunction;
class LLBasicBlock;
class LLInst;
class LLOperand;

using LLBlockPtr    = std::shared_ptr<LLBasicBlock>;
using LLFunctionPtr = std::shared_ptr<LLFunction>;
using LLInstPtr     = std::shared_ptr<LLInst>;
using LLOperandPtr  = std::shared_ptr<LLOperand>;
using LLInstList    = std::list<LLInstPtr>;

class LLFunction {
private:
  mid::FuncPtr               _function;
  std::vector<LLBlockPtr>    _blocks;

  // number of virtual registers allocated
  std::size_t                _virtual_max{};

  // size of stack allocated for local alloca and spilled regisiters
  std::size_t                _stack_size{};

  std::unordered_set<ArmReg> _callee_saved_regs;

public:
  explicit LLFunction(mid::FuncPtr func) : _function(std::move(func)) {}

  bool is_decl() const { return _function->is_decl(); }

  mid::FuncPtr function()   const  { return _function;    }
  std::size_t virtual_max() const  { return _virtual_max; }
  std::size_t stack_size()  const  { return _stack_size;  }
  std::vector<LLBlockPtr> blocks() { return _blocks;      }

  void SetVirtualMax(std::size_t virtual_max) { _virtual_max = virtual_max; }
  void SetStackSize(std::size_t stack_size)   { _stack_size = stack_size;   }

  // classof used for dyn_cast
  CLASSOF(LLFunction)
};


class LLBasicBlock {
private:
  std::string   _name;
  mid::BlockPtr _block;
  LLFunctionPtr _parent;

  // predecessors and successors
  std::vector<LLBlockPtr>   _predecessors;
  std::array<LLBlockPtr, 2> _successors;

  // instructions
  LLInstList                _insts;

  // TODO: liveness analysis
  std::unordered_set<LLOperandPtr> _live_use;
  std::unordered_set<LLOperandPtr> _definition;
  std::unordered_set<LLOperandPtr> _live_in;
  std::unordered_set<LLOperandPtr> _live_out;

public:
  explicit LLBasicBlock(std::string name, mid::BlockPtr block, LLFunctionPtr parent)
    : _name(std::move(name)), _block(std::move(block)), _parent(std::move(parent)) {}

  // getter/setter
   LLInstList    &insts()           { return _insts;         }
  LLInstList::iterator inst_begin() { return _insts.begin(); }
  LLInstList::iterator inst_end()   { return _insts.end();   }
  const LLFunctionPtr &parent()     {return _parent;         }

  CLASSOF(LLBasicBlock)
};

class LLOperand {
public:
  enum class State {
    RealReg,
    Allocated,
    Virtual,
    Immediate
  };

private:
  State       _state;       // state of operand
  ArmReg      _reg;         // which register (state is RealReg)
  std::size_t _virtual_num; // virtual register num (state is Virtual)
  std::size_t _imm_num;     // number of immediate num (state is immediate)

public:
  LLOperand(State state)
    : _state(state), _virtual_num(-1), _imm_num(-1) {}

  LLOperand(State state, int n) : _state(state) {
    switch (state) {
      case State::Immediate: _imm_num = n;                  break;
      case State::Virtual:   _virtual_num = n;              break;
      case State::RealReg:   _reg = static_cast<ArmReg>(n); break;
      default: break;
    }
  }

  inline static LLOperandPtr Register(ArmReg reg) {
    auto n = (int)reg;
    DBG_ASSERT(n >= int(ArmReg::r0) && n <= int(ArmReg::pc), "reg is out of range (r0 to r15)");
    return std::make_shared<LLOperand>(State::RealReg, n);
  }

  inline static LLOperandPtr Virtual(std::size_t &n) {
    return std::make_shared<LLOperand>(State::Virtual, n++);
  }

  inline static LLOperandPtr Immediate(int n) {
    return std::make_shared<LLOperand>(State::Immediate, n);
  }

  State state() const { return _state; }

  CLASSOF(LLOperand)
};

// Low level instruction
class LLInst {
public:
  enum class Opcode {
    // binary
    Add, Sub, Mul, SDiv, SRem, And, Or, Xor, Shl, AShr, LShr,

    // control
    Branch, Jump, Return,

    // memory
    Load, Store, Move,

    Compare, Call, Global,

    // FMA
    MLA, MLS,

    // comment
    Comment
  };

private:
  LLBlockPtr   _block;
  Opcode       _opcode;
  ClassId      _class_id;

public:
  explicit LLInst(Opcode opcode, ClassId id = ClassId::LLInstId)
    : _opcode(opcode), _class_id(id) {}

  Opcode opcode() const { return _opcode; }
  LLBlockPtr &parent()  { return _block;  }

  void SetParent(const LLBlockPtr &parent) { _block = parent; }

  ClassId classId() const { return _class_id; }
  CLASSOF(LLInst)
};

class LLBinaryInst : public LLInst {
private:
  LLOperandPtr _dst;
  LLOperandPtr _lhs;
  LLOperandPtr _rhs;
  ArmShift     _shift;

public:

  LLBinaryInst(Opcode opcode, LLOperandPtr dst, LLOperandPtr lhs, LLOperandPtr rhs)
    : LLInst(opcode, ClassId::BinaryOperatorId),
      _dst(std::move(dst)), _lhs(std::move(lhs)), _rhs(std::move(rhs)) {}

  // getter/setter
  const LLOperandPtr &dst() { return _dst;   }
  const LLOperandPtr &lhs() { return _lhs;   }
  const LLOperandPtr &rhs() { return _rhs;   }
  ArmShift shift()    const { return _shift; }

  void setShift(ArmShift shift) { _shift = shift; }
  void setShiftNum(int num) { _shift.setShift(num); }
  void setShiftType(ArmShift::ShiftType type) { _shift.setType(type); }

  CLASSOF(LLBinaryInst)
  CLASSOF_INST(LLBinaryInst)
};

class LLMove : public LLInst {
private:
  LLOperandPtr _dst;
  LLOperandPtr _src;
  ArmShift     _shift;

public:
  LLMove(LLOperandPtr dst, LLOperandPtr src)
    : LLInst(Opcode::Move, ClassId::LLMoveId),
      _dst(std::move(dst)), _src(std::move(src)) {}

  // getter/setter
  const LLOperandPtr &dst() { return _dst;   }
  const LLOperandPtr &src() { return _src;   }
  ArmShift shift()    const { return _shift; }

  void setShift(ArmShift shift)               { _shift = shift;       }
  void setShiftNum(int num)                   { _shift.setShift(num); }
  void setShiftType(ArmShift::ShiftType type) { _shift.setType(type); }

  CLASSOF(LLMove)
  CLASSOF_INST(LLMove)
};

// beq <true_block>
// b  <false_block>
class LLBranch : public LLInst {
private:
  ArmCond      _arm_cond;
  LLOperandPtr _cond;
  LLBlockPtr   _true_block;
  LLBlockPtr   _false_block;

public:
  LLBranch(ArmCond arm_cond, LLOperandPtr cond, LLBlockPtr true_block, LLBlockPtr false_block)
    : LLInst(Opcode::Branch, ClassId::LLBranchId),
      _arm_cond(arm_cond), _cond(std::move(cond)), _true_block(std::move(true_block)), _false_block(std::move(false_block)) {}

  ArmCond      arm_cond() const { return _arm_cond;    }
  LLOperandPtr cond()           { return _cond;        }
  LLBlockPtr   true_block()     { return _true_block;  }
  LLBlockPtr   false_block()    { return _false_block; }

  CLASSOF(LLBranch)
  CLASSOF_INST(LLBranch)
};

// b label
class LLJump : public LLInst {
private:
  LLBlockPtr _target;

public:
  LLJump(LLBlockPtr target)
    : LLInst(Opcode::Jump, ClassId::LLJumpId), _target(std::move(target)) {}

  LLBlockPtr target() { return _target; }

  CLASSOF(LLJump)
  CLASSOF_INST(LLJump)
};

// bx lr
class LLReturn : public LLInst {
public:
  LLReturn()
    : LLInst(Opcode::Return, ClassId::LLReturnId) {}

  CLASSOF(LLReturn)
  CLASSOF_INST(LLReturn)
};

// ldr rd, [rn {, offset}]
class LLLoad : public LLInst {
private:
  LLOperandPtr _dst;
  LLOperandPtr _addr;
  LLOperandPtr _offset;

public:
  LLLoad(LLOperandPtr dst, LLOperandPtr addr, LLOperandPtr offset)
    : LLInst(Opcode::Load, ClassId::LLLoadId),
      _dst(std::move(dst)), _addr(std::move(addr)), _offset(std::move(offset)) {}

  LLOperandPtr dst()    { return _dst;    }
  LLOperandPtr addr()   { return _addr;   }
  LLOperandPtr offset() { return _offset; }

  void SetDst(const LLOperandPtr &dst)       { _dst    = dst;    }
  void SetAddr(const LLOperandPtr &addr)     { _addr   = addr;   }
  void SetOffset(const LLOperandPtr &offset) { _offset = offset; }

  CLASSOF(LLLoad)
  CLASSOF_INST(LLLoad)
};

// str rd, [rn {, offset}]
class LLStore : public LLInst {
private:
  LLOperandPtr _data;
  LLOperandPtr _addr;
  LLOperandPtr _offset;

public:
  LLStore(LLOperandPtr data, LLOperandPtr addr, LLOperandPtr offset)
      : LLInst(Opcode::Store, ClassId::LLStoreId),
        _data(std::move(data)), _addr(std::move(addr)), _offset(std::move(offset)) {}

  LLOperandPtr data()   { return _data;   }
  LLOperandPtr addr()   { return _addr;   }
  LLOperandPtr offset() { return _offset; }

  CLASSOF(LLStore)
  CLASSOF_INST(LLStore)
};

// cmp rn, <operand2>
class LLCompare : public LLInst {
private:
  LLOperandPtr _lhs;
  LLOperandPtr _rhs;

public:
  LLCompare(LLOperandPtr lhs, LLOperandPtr rhs)
    : LLInst(Opcode::Compare, ClassId::LLCompareId),
      _lhs(std::move(lhs)), _rhs(std::move(rhs)) {}

  LLOperandPtr lhs() { return _lhs; }
  LLOperandPtr rhs() { return _rhs; }

  CLASSOF(LLCompare)
  CLASSOF_INST(LLCompare)
};

// bl <label>
class LLCall : public LLInst {
private:
  mid::FuncPtr _function;

public:
  LLCall(mid::FuncPtr function)
    : LLInst(Opcode::Call, ClassId::LLCallId), _function(std::move(function)) {}
};

// comment
class LLComment : public LLInst {
private:
  std::string _comment;

public:
  LLComment(const std::string &str) : LLInst(Opcode::Comment, ClassId::LLCommentId) {}

  const std::string &comment() { return _comment; }

  CLASSOF(LLComment)
  CLASSOF_INST(LLComment)
};

class LLFMA : public LLInst {
private:
  LLOperandPtr _dst;
  LLOperandPtr _lhs;
  LLOperandPtr _rhs;
  LLOperandPtr _acc;

public:
  LLFMA(Opcode opcode, const LLOperandPtr &dst, const LLOperandPtr &lhs, const LLOperandPtr &rhs, const LLOperandPtr &acc, ClassId classId)
    : LLInst(opcode, classId), _dst(dst), _lhs(lhs), _rhs(rhs), _acc(acc) {}

  LLOperandPtr dst() { return _dst; }
  LLOperandPtr lhs() { return _lhs; }
  LLOperandPtr rhs() { return _rhs; }
  LLOperandPtr acc() { return _acc; }

  CLASSOF(LLFMA)
  CLASSOF_INST(LLFMA)
};

// mla rd, rm, rs, rn <--> rd := (rn + (rm * rs))
class LLMLA : public LLFMA {
public:
  LLMLA(const LLOperandPtr &dst, const LLOperandPtr &lhs, const LLOperandPtr &rhs, const LLOperandPtr &acc)
    : LLFMA(Opcode::MLA, dst, lhs, rhs, acc, ClassId::LLMLAId) {}

  CLASSOF(LLMLA)
  CLASSOF_INST(LLMLA)
};

// mls rd, rm, rs, rn <--> rd := (rn - (rm * rs))
class LLMLS : public LLFMA {
public:
  LLMLS(const LLOperandPtr &dst, const LLOperandPtr &lhs, const LLOperandPtr &rhs, const LLOperandPtr &acc)
      : LLFMA(Opcode::MLA, dst, lhs, rhs, acc, ClassId::LLMLAId) {}

  CLASSOF(LLMLS)
  CLASSOF_INST(LLMLS)
};
}

#endif //LAVA_INSTDEF_H
