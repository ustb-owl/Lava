#ifndef LAVA_INSTDEF_H
#define LAVA_INSTDEF_H

/* Low-Level Intermediate representation (LLIR) */

#include <set>
#include <array>
#include <utility>
#include "mid/ir/ssa.h"
#include "common/classid.h"

namespace lava {
class IdManager;
}

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
inline ArmCond opposite_cond(ArmCond c) {
  constexpr static ArmCond OPPOSITE[] = {ArmCond::Any, ArmCond::Ne, ArmCond::Eq, ArmCond::Lt,
                                         ArmCond::Le,  ArmCond::Gt, ArmCond::Ge};
  return OPPOSITE[(int)c];
}

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
using LLOperandList = std::vector<LLOperandPtr>;

class LLFunction {
private:
  mid::FuncPtr               _function;
  std::vector<LLBlockPtr>    _blocks;

  // number of virtual registers allocated
  std::size_t                _virtual_max{};

  // size of stack allocated for local alloca and spilled regisiters
  std::size_t                _stack_size{};

  std::set<ArmReg>           _callee_saved_regs;

  bool                       _has_call_inst;

public:
  explicit LLFunction(mid::FuncPtr func)
    : _function(std::move(func)), _has_call_inst(false) {}

  bool is_decl() const { return _function->is_decl(); }

  mid::FuncPtr function()       const { return _function;          }
  std::size_t virtual_max()     const { return _virtual_max;       }
  std::size_t stack_size()      const { return _stack_size;        }
  std::set<ArmReg> saved_regs() const { return _callee_saved_regs; }
  std::vector<LLBlockPtr> blocks()    { return _blocks;            }
  const LLBlockPtr &entry()           { return _blocks[0];         }
  bool has_call_inst()          const { return _has_call_inst;     }

  void SetVirtualMax(std::size_t virtual_max) { _virtual_max = virtual_max;     }
  void SetStackSize(std::size_t stack_size)   { _stack_size = stack_size;       }
  void AddBlock(const LLBlockPtr &block)      { _blocks.push_back(block);       }
  void AddSavedRegister(ArmReg reg)           { _callee_saved_regs.insert(reg); }
  void SetHasCallInst(bool value)             { _has_call_inst = value;         }


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
  LLInstList          &insts()      { return _insts;         }
  LLInstList::iterator inst_begin() { return _insts.begin(); }
  LLInstList::iterator inst_end()   { return _insts.end();   }
  const LLFunctionPtr &parent()     { return _parent;        }
  const std::string   &name() const { return _name;          }
  CLASSOF(LLBasicBlock)
};

class LLOperand {
public:
  enum class State {
    RealReg,
    Allocated,
    Virtual,
    Immediate,
  };

private:
  State        _state;       // state of operand
  ArmReg       _reg;         // which register (state is RealReg)
  std::size_t  _virtual_num; // virtual register num (state is Virtual)
  int          _imm_num;     // number of immediate num (state is Immediate)
  LLOperandPtr _allocated;   // allocation result (state is virtual)

public:
  LLOperand(State state)
    : _state(state), _virtual_num(-1), _imm_num(-1), _allocated(nullptr) {}

  LLOperand(State state, int n);

  bool IsVirtual()   const { return _state == State::Virtual;   }
  bool IsRealReg()   const { return _state == State::RealReg;   }
  bool IsImmediate() const { return _state == State::Immediate; }

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

  void set_allocated(const LLOperandPtr &allocated) {
    DBG_ASSERT(_state == State::Virtual, "this is not virtual reg");
    _allocated = allocated;
  }

  void ReplaceWith(const LLOperandPtr &V);

  State        state()       const { return _state;       }
  ArmReg       reg()         const { return _reg;         }
  int          imm_num()     const { return _imm_num;     }
  std::size_t  virtual_num() const { return _virtual_num; }
  LLOperandPtr allocated()   const { return _allocated;   }

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

    // stack
    PUSH, POP,

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

  virtual ~LLInst() {}

  Opcode opcode() const { return _opcode; }
  LLBlockPtr &parent()  { return _block;  }

  void SetParent(const LLBlockPtr &parent) { _block = parent; }

  virtual LLOperandList  operands() = 0;
  virtual LLOperandPtr       dest() { return nullptr; };
  virtual void set_dst(const LLOperandPtr &dst) { };
  virtual void set_operand(const LLOperandPtr &opr, std::size_t index) {
    ERROR("should not reach here");
  }

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
    : LLInst(opcode, ClassId::LLBinaryInstId),
      _dst(std::move(dst)), _lhs(std::move(lhs)), _rhs(std::move(rhs)) {}

  // getter/setter
  const LLOperandPtr &dst() { return _dst;   }
  const LLOperandPtr &lhs() { return _lhs;   }
  const LLOperandPtr &rhs() { return _rhs;   }
  ArmShift shift()    const { return _shift; }

  LLOperandList operands() final;
  LLOperandPtr dest() final { return dst(); }
  void set_dst(const LLOperandPtr &dst) final {
    _dst = dst;
  }

  void set_operand(const LLOperandPtr &opr, std::size_t index) override;

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
  bool         _is_arg;
  ArmCond      _cond;

public:
  LLMove(LLOperandPtr dst, LLOperandPtr src, ArmCond cond = ArmCond::Any)
    : LLInst(Opcode::Move, ClassId::LLMoveId),
      _dst(std::move(dst)), _src(std::move(src)),  _is_arg(false), _cond(cond) {}

  // getter/setter
  bool             is_arg() { return _is_arg; }
  const LLOperandPtr &dst() { return _dst;    }
  const LLOperandPtr &src() { return _src;    }
  ArmShift shift()    const { return _shift;  }
  ArmCond cond()      const { return _cond;   }

  void setShift(ArmShift shift)               { _shift = shift;       }
  void setShiftNum(int num)                   { _shift.setShift(num); }
  void setShiftType(ArmShift::ShiftType type) { _shift.setType(type); }
  void SetIsArg(bool value)                   { _is_arg = value;      }

  LLOperandList operands() final;
  LLOperandPtr dest() final { return dst(); }
  void set_operand(const LLOperandPtr &opr, std::size_t index) override;
  void set_dst(const LLOperandPtr &dst) final {
    _dst = dst;
  }

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

  LLOperandList operands() final { return LLOperandList(); }
  void set_operand(const LLOperandPtr &opr, std::size_t index) override;

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

  LLOperandList operands() final { return LLOperandList(); }

  CLASSOF(LLJump)
  CLASSOF_INST(LLJump)
};

// bx lr
class LLReturn : public LLInst {
public:
  LLReturn()
    : LLInst(Opcode::Return, ClassId::LLReturnId) {}

  LLOperandList operands() final { return LLOperandList(); }

  CLASSOF(LLReturn)
  CLASSOF_INST(LLReturn)
};

// ldr rd, [rn {, offset}]
class LLLoad : public LLInst {
private:
  LLOperandPtr _dst;
  LLOperandPtr _addr;
  LLOperandPtr _offset;

  bool         _is_arg;

public:
  LLLoad(LLOperandPtr dst, LLOperandPtr addr, LLOperandPtr offset)
    : LLInst(Opcode::Load, ClassId::LLLoadId),
      _dst(std::move(dst)), _addr(std::move(addr)), _offset(std::move(offset)) {}

  bool         is_arg() { return _is_arg; }
  LLOperandPtr dst()    { return _dst;    }
  LLOperandPtr addr()   { return _addr;   }
  LLOperandPtr offset() { return _offset; }

  LLOperandPtr  dest()     final { return dst(); }
  LLOperandList operands() final;
  void set_operand(const LLOperandPtr &opr, std::size_t index) override;
  void set_dst(const LLOperandPtr &dst) final {
    _dst = dst;
  }

  void SetIsArg(bool value)                  { _is_arg = value;  }
  void SetDst(const LLOperandPtr &dst)       { _dst    = dst;    }
  void SetAddr(const LLOperandPtr &addr)     { _addr   = addr;   }
  void SetOffset(const LLOperandPtr &offset) { _offset = offset; }

  CLASSOF(LLLoad)
  CLASSOF_INST(LLLoad)
};

// TODO: dirty hack
class LLLoadPseudo : public LLInst {
private:
  LLOperandPtr _dst;
  int          _imm;

public:
  LLLoadPseudo(LLOperandPtr dst, int number)
    : LLInst(Opcode::Load, ClassId::LLLoadPseudoId), _dst(std::move(dst)), _imm(number){}

  LLOperandPtr dst() { return _dst; }
  int          imm() { return _imm; }

  LLOperandList operands() final { return LLOperandList(); }

  CLASSOF(LLLoadPseudo)
  CLASSOF_INST(LLLoadPseudo)
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

  LLOperandList operands() final;
  void set_operand(const LLOperandPtr &opr, std::size_t index) override;

  CLASSOF(LLStore)
  CLASSOF_INST(LLStore)
};

// cmp rn, <operand2>
class LLCompare : public LLInst {
private:
  ArmCond      _cond;
  LLOperandPtr _lhs;
  LLOperandPtr _rhs;
public:
  LLCompare(ArmCond cond, LLOperandPtr lhs, LLOperandPtr rhs)
    : LLInst(Opcode::Compare, ClassId::LLCompareId),
      _cond(cond), _lhs(std::move(lhs)), _rhs(std::move(rhs)) {}

  ArmCond      cond() { return _cond; }
  LLOperandPtr rhs()  { return _rhs;  }
  LLOperandPtr lhs()  { return _lhs;  }

  LLOperandList operands() final;
  void set_operand(const LLOperandPtr &opr, std::size_t index) override;

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

  const mid::FuncPtr &function() { return _function;       }
  LLOperandList operands() final { return LLOperandList(); }

  CLASSOF(LLCall)
  CLASSOF_INST(LLCall)
};

// comment
class LLComment : public LLInst {
private:
  std::string _comment;

public:
  LLComment(const std::string &str)
    : LLInst(Opcode::Comment, ClassId::LLCommentId), _comment(str) {}

  const std::string &comment()   { return _comment; }
  LLOperandList operands() final { return LLOperandList(); }

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

  LLOperandList operands() override;
  LLOperandPtr dest() override { return dst(); }
  void set_operand(const LLOperandPtr &opr, std::size_t index) override;
  void set_dst(const LLOperandPtr &dst) override {
    _dst = dst;
  }

  CLASSOF(LLFMA)
  CLASSOF_INST(LLFMA)
};

// mla rd, rm, rs, rn <--> rd := (rn + (rm * rs))
class LLMLA : public LLFMA {
public:
  LLMLA(const LLOperandPtr &dst, const LLOperandPtr &lhs, const LLOperandPtr &rhs, const LLOperandPtr &acc)
    : LLFMA(Opcode::MLA, dst, lhs, rhs, acc, ClassId::LLMLAId) {}

  LLOperandPtr dest() final { return LLFMA::dst(); }
  LLOperandList operands() override { return LLFMA::operands(); }

  void set_operand(const LLOperandPtr &opr, std::size_t index) override {
    LLFMA::set_operand(opr, index);
  }

  void set_dst(const LLOperandPtr &dst) final {
    LLFMA::set_dst(dst);
  }

  CLASSOF(LLMLA)
  CLASSOF_INST(LLMLA)
};

// mls rd, rm, rs, rn <--> rd := (rn - (rm * rs))
class LLMLS : public LLFMA {
public:
  LLMLS(const LLOperandPtr &dst, const LLOperandPtr &lhs, const LLOperandPtr &rhs, const LLOperandPtr &acc)
      : LLFMA(Opcode::MLA, dst, lhs, rhs, acc, ClassId::LLMLSId) {}

  LLOperandPtr dest() final { return LLFMA::dst(); }
  LLOperandList operands() override { return LLFMA::operands(); }

  void set_operand(const LLOperandPtr &opr, std::size_t index) override {
    LLFMA::set_operand(opr, index);
  }

  void set_dst(const LLOperandPtr &dst) final {
    LLFMA::set_dst(dst);
  }

  CLASSOF(LLMLS)
  CLASSOF_INST(LLMLS)
};

// push {reg-list}
class LLPush : public LLInst {
private:
  std::vector<ArmReg> _reg_list;
public:
  LLPush(std::vector<ArmReg> reglist)
    : LLInst(Opcode::PUSH, ClassId::LLPushId), _reg_list(std::move(reglist)) {}

  LLOperandList operands() final { return LLOperandList(); }
  std::vector<ArmReg> &reg_list() { return _reg_list; }

  CLASSOF(LLPush)
  CLASSOF_INST(LLPush)
};

// pop {reg-list}
class LLPop : public LLInst {
private:
  std::vector<ArmReg> _reg_list;
public:
  LLPop(std::vector<ArmReg> reglist)
      : LLInst(Opcode::PUSH, ClassId::LLPopId), _reg_list(std::move(reglist)) {}

  LLOperandList operands() final { return LLOperandList(); }
  std::vector<ArmReg> &reg_list() { return _reg_list; }

  CLASSOF(LLPop)
  CLASSOF_INST(LLPop)
};


class LLGlobal : public LLInst {
private:
  LLOperandPtr         _dst;
  mid::GlobalVariable *_glob_var;

public:
  LLGlobal(const LLOperandPtr &dst, mid::GlobalVariable *glob)
    : LLInst(Opcode::Global, ClassId::LLGlobalId), _dst(dst), _glob_var(glob) {}

  mid::GlobalVariable *global_variable() { return _glob_var; }
  LLOperandPtr dst() { return _dst; }

  LLOperandPtr dest() final { return _dst; }
  LLOperandList operands() final { return LLOperandList(); }

  void set_dst(const LLOperandPtr &dst) { _dst = dst; }

  CLASSOF(LLGlobal)
  CLASSOF_INST(LLGlobal)
};


}

#endif //LAVA_INSTDEF_H
