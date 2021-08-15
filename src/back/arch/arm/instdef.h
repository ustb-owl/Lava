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

enum class ArmPSR { N, C, F, S, X };

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

std::ostream &operator<<(std::ostream &os, const ArmPSR &psr);

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
  bool                       _need_hash;

public:
  explicit LLFunction(mid::FuncPtr func)
    : _function(std::move(func)), _has_call_inst(false), _need_hash(false) {}

  bool is_decl()           const { return _function->is_decl();           }
  bool is_tail_recursion() const { return _function->is_tail_recursion(); }

  mid::FuncPtr function()       const { return _function;          }
  std::size_t virtual_max()     const { return _virtual_max;       }
  std::size_t stack_size()      const { return _stack_size;        }
  std::set<ArmReg> saved_regs() const { return _callee_saved_regs; }
  std::vector<LLBlockPtr> &blocks()   { return _blocks;            }
  const LLBlockPtr &entry()           { return _blocks[0];         }
  bool has_call_inst()          const { return _has_call_inst;     }
  bool isDecl()                 const { return _need_hash;         }

  std::vector<LLBlockPtr>::iterator block_begin() { return _blocks.begin(); }
  std::vector<LLBlockPtr>::iterator block_end()   { return _blocks.end();   }


  void SetVirtualMax(std::size_t virtual_max)    { _virtual_max = virtual_max;     }
  void SetStackSize(std::size_t stack_size)      { _stack_size = stack_size;       }
  void AddBlock(const LLBlockPtr &block)         { _blocks.push_back(block);       }
  void AddSavedRegister(ArmReg reg)              { _callee_saved_regs.insert(reg); }
  void SetHasCallInst(bool value)                { _has_call_inst = value;         }
  void SetBlocks(std::vector<LLBlockPtr> blocks) { _blocks = std::move(blocks);    }
  void SetNeedHash()                             { _need_hash = true;              }


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
  void SetBlockName(const std::string &name) { _name = name; }
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
  State        _state;        // state of operand
  ArmReg       _reg;          // which register (state is RealReg)
  std::size_t  _virtual_num;  // virtual register num (state is Virtual)
  int          _imm_num;      // number of immediate num (state is Immediate)
  LLOperandPtr _allocated;    // allocation result (state is virtual)
  bool         _allow_to_tmp; // if this operand is allowed to allocated to r0-r3

public:
  LLOperand(State state)
    : _state(state), _virtual_num(-1), _imm_num(-1), _allocated(nullptr), _allow_to_tmp(true) {}

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

  void set_not_allowed_to_tmp(bool value) {
    _allow_to_tmp = value;
  }

  void ReplaceWith(const LLOperandPtr &V);

  State        state()        const { return _state;        }
  ArmReg       reg()          const { return _reg;          }
  int          imm_num()      const { return _imm_num;      }
  std::size_t  virtual_num()  const { return _virtual_num;  }
  LLOperandPtr allocated()    const { return _allocated;    }
  bool         allow_to_tmp() const { return _allow_to_tmp; }

  CLASSOF(LLOperand)
};

bool operator==(const LLOperandPtr &lhs, const LLOperandPtr &rhs);

// Low level instruction
class LLInst {
public:
  enum class Opcode {
    // binary
    Add, Sub, Mul, SDiv, SRem, And, Or, Xor, Shl, AShr, LShr, Bic,

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
  ArmPSR       _psr;

public:

  LLBinaryInst(Opcode opcode, LLOperandPtr dst, LLOperandPtr lhs, LLOperandPtr rhs)
    : LLInst(opcode, ClassId::LLBinaryInstId),
      _dst(std::move(dst)), _lhs(std::move(lhs)), _rhs(std::move(rhs)), _psr(ArmPSR::N) {}

  // getter/setter
  const LLOperandPtr &dst() { return _dst;   }
  const LLOperandPtr &lhs() { return _lhs;   }
  const LLOperandPtr &rhs() { return _rhs;   }
  ArmShift shift()    const { return _shift; }
  ArmPSR psr()        const { return _psr;   }

  LLOperandList operands() final;
  LLOperandPtr dest() final { return dst(); }
  void set_dst(const LLOperandPtr &dst) final {
    _dst = dst;
  }

  void set_operand(const LLOperandPtr &opr, std::size_t index) override;

  void setShift(ArmShift shift) { _shift = shift; }
  void setShiftNum(int num) { _shift.setShift(num); }
  void setShiftType(ArmShift::ShiftType type) { _shift.setType(type); }
  void setPSR(ArmPSR psr) { _psr = psr; }

  CLASSOF(LLBinaryInst)
  CLASSOF_INST(LLBinaryInst)
};

class LLMove : public LLInst {
private:
  LLOperandPtr _dst;
  LLOperandPtr _src;
  ArmShift     _shift;
  ArmCond      _cond;
  bool         _is_arg;

public:
  LLMove(LLOperandPtr dst, LLOperandPtr src, ArmCond cond = ArmCond::Any)
    : LLInst(Opcode::Move, ClassId::LLMoveId),
      _dst(std::move(dst)), _src(std::move(src)),
      _shift(ArmShift()), _cond(cond), _is_arg(false){}

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

  bool IsSimple() { return (_cond == ArmCond::Any && _shift.is_none()); }

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
  bool         _need_out_false;

public:
  LLBranch(ArmCond arm_cond, LLOperandPtr cond, LLBlockPtr true_block, LLBlockPtr false_block)
    : LLInst(Opcode::Branch, ClassId::LLBranchId),
      _arm_cond(arm_cond), _cond(std::move(cond)), _true_block(std::move(true_block)), _false_block(std::move(false_block)),
      _need_out_false(true) {}

  ArmCond      arm_cond() const { return _arm_cond;       }
  LLOperandPtr cond()           { return _cond;           }
  LLBlockPtr   true_block()     { return _true_block;     }
  LLBlockPtr   false_block()    { return _false_block;    }
  bool         need_out_false() { return _need_out_false; }
  void set_out_false(bool val)  { _need_out_false = val;  }

  LLOperandList operands() final { return LLOperandList(); }
  void set_operand(const LLOperandPtr &opr, std::size_t index) override;

  CLASSOF(LLBranch)
  CLASSOF_INST(LLBranch)
};

// b{cond} label
class LLJump : public LLInst {
private:
  LLBlockPtr _target;
  ArmCond    _cond;
  bool       _is_pl;

public:
  LLJump(LLBlockPtr target, ArmCond cond = ArmCond::Any)
    : LLInst(Opcode::Jump, ClassId::LLJumpId),
      _target(std::move(target)), _cond(cond), _is_pl(false) {}

  LLBlockPtr target()  { return _target; }
  ArmCond cond() const { return _cond;   }

  bool IsPl() const    { return _is_pl; }
  void SetPL(bool val) { _is_pl = val;  }

  LLOperandList operands() final { return LLOperandList(); }

  CLASSOF(LLJump)
  CLASSOF_INST(LLJump)
};

// bx{cond} lr
class LLReturn : public LLInst {
private:
  ArmCond _cond;
public:
  LLReturn(ArmCond cond = ArmCond::Any)
    : LLInst(Opcode::Return, ClassId::LLReturnId), _cond(cond) {}

  LLOperandList operands() final { return LLOperandList(); }

  ArmCond cond() const { return _cond; }

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
  ArmCond      _cond;

public:
  LLLoad(LLOperandPtr dst, LLOperandPtr addr, LLOperandPtr offset)
    : LLInst(Opcode::Load, ClassId::LLLoadId),
      _dst(std::move(dst)), _addr(std::move(addr)), _offset(std::move(offset)),
      _is_arg(false), _cond(ArmCond::Any) {}

  bool         is_arg() { return _is_arg; }
  ArmCond      cond()   { return _cond;   }
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
  void SetCond(ArmCond cond)                 { _cond = cond;     }
  void SetDst(const LLOperandPtr &dst)       { _dst    = dst;    }
  void SetAddr(const LLOperandPtr &addr)     { _addr   = addr;   }
  void SetOffset(const LLOperandPtr &offset) { _offset = offset; }

  CLASSOF(LLLoad)
  CLASSOF_INST(LLLoad)
};


// ldrpl rd, [rn {, offset}]
class LLLoadPL : public LLInst {
private:
  LLOperandPtr _dst;
  LLOperandPtr _addr;
  LLOperandPtr _offset;

public:
  LLLoadPL(LLOperandPtr dst, LLOperandPtr addr, LLOperandPtr offset)
      : LLInst(Opcode::Load, ClassId::LLLoadPLId),
        _dst(std::move(dst)), _addr(std::move(addr)), _offset(std::move(offset)) {}

  LLOperandPtr dst()    { return _dst;    }
  LLOperandPtr addr()   { return _addr;   }
  LLOperandPtr offset() { return _offset; }

  LLOperandPtr  dest()     final { return dst(); }
  LLOperandList operands() final;
  void set_operand(const LLOperandPtr &opr, std::size_t index) override;
  void set_dst(const LLOperandPtr &dst) final {
    _dst = dst;
  }

  void SetDst(const LLOperandPtr &dst)       { _dst    = dst;    }
  void SetAddr(const LLOperandPtr &addr)     { _addr   = addr;   }
  void SetOffset(const LLOperandPtr &offset) { _offset = offset; }

  CLASSOF(LLLoadPL)
  CLASSOF_INST(LLLoadPL)
};

// ldr rd, [rs], #offset
//  => rd = *rs;
//  => rs += offset;
class LLLoadChangeBase : public LLInst {
private:
  LLOperandPtr _dst;
  LLOperandPtr _addr;
  LLOperandPtr _offset;

public:
  LLLoadChangeBase(LLOperandPtr dst, LLOperandPtr addr, LLOperandPtr offset)
    : LLInst(Opcode::Load, ClassId::LLLoadChangeBaseId),
      _dst(std::move(dst)), _addr(std::move(addr)), _offset(std::move(offset)) {}

  LLOperandPtr dst()    { return _dst;    }
  LLOperandPtr addr()   { return _addr;   }
  LLOperandPtr offset() { return _offset; }

  LLOperandPtr  dest()     final { return dst(); }
  LLOperandList operands() final;
  void set_operand(const LLOperandPtr &opr, std::size_t index) override;
  void set_dst(const LLOperandPtr &dst) final {
    _dst = dst;
  }

  void SetDst(const LLOperandPtr &dst)       { _dst    = dst;    }
  void SetAddr(const LLOperandPtr &addr)     { _addr   = addr;   }
  void SetOffset(const LLOperandPtr &offset) { _offset = offset; }

  CLASSOF(LLLoadChangeBase)
  CLASSOF_INST(LLLoadChangeBase)
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

// str rd, [rn], offset
class LLStoreChangeBase : public LLInst {
private:
  LLOperandPtr _data;
  LLOperandPtr _addr;
  LLOperandPtr _offset;

public:
  LLStoreChangeBase(LLOperandPtr data, LLOperandPtr addr, LLOperandPtr offset)
      : LLInst(Opcode::Store, ClassId::LLStoreChangeBaseId),
        _data(std::move(data)), _addr(std::move(addr)), _offset(std::move(offset)) {}

  LLOperandPtr data()   { return _data;   }
  LLOperandPtr addr()   { return _addr;   }
  LLOperandPtr offset() { return _offset; }

  LLOperandList operands() final;
  void set_operand(const LLOperandPtr &opr, std::size_t index) override;

  CLASSOF(LLStoreChangeBase)
  CLASSOF_INST(LLStoreChangeBase)
};

// cmp rn, <operand2>
class LLCompare : public LLInst {
private:
  ArmCond      _cond;
  LLOperandPtr _lhs;
  LLOperandPtr _rhs;
  bool         _is_pl;

public:
  LLCompare(ArmCond cond, LLOperandPtr lhs, LLOperandPtr rhs)
    : LLInst(Opcode::Compare, ClassId::LLCompareId),
      _cond(cond), _lhs(std::move(lhs)), _rhs(std::move(rhs)), _is_pl(false) {}

  ArmCond      cond() { return _cond; }
  LLOperandPtr rhs()  { return _rhs;  }
  LLOperandPtr lhs()  { return _lhs;  }

  bool IsPL() const    { return _is_pl; }
  void SetPL(bool val) { _is_pl = val;  }

  LLOperandList operands() final;
  void set_operand(const LLOperandPtr &opr, std::size_t index) override;

  CLASSOF(LLCompare)
  CLASSOF_INST(LLCompare)
};

// b{l} <label>
class LLCall : public LLInst {
private:
  mid::FuncPtr _function;
  bool         _is_tail_call;
  bool         _changed_mod;

public:
  LLCall(mid::FuncPtr function)
    : LLInst(Opcode::Call, ClassId::LLCallId),
      _function(std::move(function)), _is_tail_call(false), _changed_mod(false) {}

  bool IsTailCall() const        { return _is_tail_call;  }
  void SetIsTailCall(bool value) { _is_tail_call = value; }

  bool NeedChangeMode() const    { return _changed_mod;   }
  void SetChangeMode(bool val)   { _changed_mod = val;    }

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
  ArmCond      _cond;

public:
  LLFMA(Opcode opcode, const LLOperandPtr &dst, const LLOperandPtr &lhs, const LLOperandPtr &rhs, const LLOperandPtr &acc, ClassId classId)
    : LLInst(opcode, classId), _dst(dst), _lhs(lhs), _rhs(rhs), _acc(acc), _cond(ArmCond::Any) {}

  LLOperandPtr dst() { return _dst; }
  LLOperandPtr lhs() { return _lhs; }
  LLOperandPtr rhs() { return _rhs; }
  LLOperandPtr acc() { return _acc; }

  ArmCond cond() const        { return _cond; }
  void set_cond(ArmCond cond) { _cond = cond; }

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
      : LLFMA(Opcode::MLS, dst, lhs, rhs, acc, ClassId::LLMLSId) {}

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
