#ifndef LAVA_SSA_H
#define LAVA_SSA_H

#include <utility>

#include "constant.h"
#include "define/ast.h"
#include "mid/ir/usedef/user.h"

namespace lava::mid {

// operands: pred1, pred2 ...
class BasicBlock : public User {
private:
  SSAPtrList  _insts;
  std::string _name;    // block name
  FuncPtr     _parent;  // block's parent(function)

public:

  BasicBlock(FuncPtr parent, std::string name)
      : User(ClassId::BasicBlockId), _name(std::move(name)), _parent(std::move(parent)) {}

  bool isInstruction() const override { return false; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  void set_parent(const FuncPtr &parent) { _parent = parent; }

  void AddInstToEnd(const SSAPtr &inst) { _insts.emplace_back(inst); }

  void AddInstBefore(const SSAPtr &insertBefore, const SSAPtr &inst);

  // remove all instructions
  void ClearInst();

  // delete it self
  void DeleteSelf();

  //getters
  SSAPtrList          &insts()        { return _insts;         }
  SSAPtrList::iterator inst_begin()   { return _insts.begin(); }
  SSAPtrList::iterator inst_end()     { return _insts.end();   }
  const FuncPtr       &parent() const { return _parent;        }
  const std::string   &name()   const { return _name;          }

  std::vector<BasicBlock *> successors();

  // methods for dyn_cast
  static inline bool classof(BasicBlock *) { return true; }
  static inline bool classof(const BasicBlock *) { return true; }
  static bool classof(Value *value);
  static bool classof(const Value *value);
};

class Instruction : public User {
private:
  unsigned _opcode;

public:
  Instruction(unsigned opcode, unsigned operand_nums, ClassId classId);

  Instruction(unsigned opcode, unsigned operand_nums, const Operands &operands, ClassId classId);


  virtual ~Instruction() = default;

  void Dump(std::ostream &os, IdManager &id_mgr) const override {}

  // Accessor methods...
  unsigned opcode() const { return _opcode; }

  std::string GetOpcodeAsString() const {
    return GetOpcodeAsString(opcode());
  }

  static std::string GetOpcodeAsString(unsigned opcode) ;

  // Determine if the opcode is one of the terminators instruction.
  static inline bool isTerminator(unsigned OpCode) {
    return OpCode >= TermOpsBegin && OpCode < TermOpsEnd;
  }

  inline bool isTerminator() const {   // Instance of TerminatorInst?
    return isTerminator(opcode());
  }

  // Determine if the opcode is one of the BinaryOperator instruction.
  inline bool isBinaryOp() const {
    return opcode() >= BinaryOpsBegin && opcode() < BinaryOpsEnd;
  }

  // Determine if the Opcode is one of the shift instructions.
  static inline bool isShift(unsigned Opcode) {
    return Opcode >= Shl && Opcode <= AShr;
  }

  // Determine if the instruction's opcode is one of the shift instructions.
  inline bool isShift() const { return isShift(opcode()); }

  // isLogicalShift - Return true if this is a logical shift left or a logical shift right.
  inline bool isLogicalShift() const {
    return opcode() == Shl || opcode() == LShr;
  }

  // isLogicalShift - Return true if this is a logical shift left or a logical shift right.
  inline bool isArithmeticShift() const {
    return opcode() == AShr;
  }

  // Determine if the opcode is one of the CastInst instruction.
  static inline bool isCast(unsigned Opcode) {
    return Opcode >= CastOpsBegin && Opcode <= CastOpsEnd;
  }

  inline bool isCast() const {
    return isCast(opcode());
  }

  bool isInstruction() const override { return true; }

  bool NeedLoad() const;


  //----------------------------------------------------------------------
  // Exported opcode enumerations...
  // TermOps, BinaryOps, MemoryOps, CastOps, OtherOps
  //
#include "opcode.inc"

  static const int AssignSpain = AssAdd - Add;

  // methods for dyn_cast
  static inline bool classof(Instruction *) { return true; }
  static inline bool classof(const Instruction *) { return true; }
  static bool classof(Value *);
  static bool classof(const Value *);
};

//===----------------------------------------------------------------------===//
//                            TerminatorInst Class
//===----------------------------------------------------------------------===//

// TerminatorInst - Subclasses of this class are all able to terminate a basic
// block.  Thus, these are all the flow control type of operations.
//
class TerminatorInst : public Instruction {
private:
  std::vector<BlockPtr> _successors;
public:
  TerminatorInst(Instruction::TermOps opcode, unsigned operands_num)
      : Instruction(opcode, operands_num, static_cast<ClassId>(opcode)) {}

  TerminatorInst(Instruction::TermOps opcode, const Operands &operands,
                 unsigned operands_num, const SSAPtr &insertBefore = nullptr)
      : Instruction(opcode, operands_num, operands, static_cast<ClassId>(opcode)) {}

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override {}

  // get all successors
  const std::vector<BlockPtr> &GetSuccessors() const { return _successors; }

  /* Virtual methods - Terminators should overload these methods. */

  // Return the number of successors that this terminator has.
  virtual unsigned GetSuccessorNum() const = 0;

  virtual SSAPtr GetSuccessor(unsigned idx) const = 0;
  virtual void SetSuccessor(unsigned idx, const BlockPtr &BB) = 0;
  virtual void AddSuccessor(const BlockPtr &BB) { _successors.push_back(BB); }

  // methods for dyn_cast
  static inline bool classof(TerminatorInst *) { return true; }
  static inline bool classof(const TerminatorInst *) { return true; }
  static bool classof(Value *);
  static bool classof(const Value *);
};

//===----------------------------------------------------------------------===//
//                           BinaryOperator Class
//===----------------------------------------------------------------------===//

class BinaryOperator : public Instruction {
public:
  BinaryOperator(BinaryOps opcode, const SSAPtr &S1,
                 const SSAPtr &S2, const define::TypePtr &type);


  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  static unsigned GetNumOperands() { return 2; }

  static BinaryPtr Create(BinaryOps opcode, const SSAPtr &S1, const SSAPtr &S2);

  bool isInstruction() const override { return true; }

  // Create* - These methods just forward to create, and are useful when you
  // statically know what type of instruction you're going to create.  These
  // helpers just save some typing.
#define HANDLE_BINARY_INST(N, OPC, ClASS)                            \
  static BinaryPtr Create##OPC(const SSAPtr &V1, const SSAPtr &V2) { \
    return Create(Instruction::OPC, V1, V2);                         \
  }

#include "instruction.inc"


  /// Helper functions to construct and inspect unary operations (NEG and NOT)
  /// via binary operators SUB and XOR:
  ///
  /// createNeg, createNot - Create the NEG and NOT
  ///     instructions out of SUB and XOR instructions.
  ///
  static BinaryPtr createNeg(const SSAPtr &Op);
  static BinaryPtr createNot(const SSAPtr &Op);

  // methods for dyn_cast
  static inline bool classof(BinaryOperator *) { return true; }
  static inline bool classof(const BinaryOperator *) { return true; }
  static bool classof(Value *);
  static bool classof(const Value *);
};

// function definition
// operands: basic blocks
class Function : public User {
private:
  bool _is_decl;
  std::vector<SSAPtr> _args;
  std::string _function_name;

public:
  explicit Function(std::string name, bool is_decl = false)
    : User(ClassId::FunctionId), _is_decl(is_decl), _function_name(std::move(name)) {}

  bool isInstruction() const override { return false; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // setters
  void set_arg(std::size_t i, const SSAPtr &arg) {
    _args.resize(i + 1);
    _args[i] = arg;
  }

  // getters
  const std::string &GetFunctionName() const { return _function_name; }

  const SSAPtr      &entry() { return (*this)[0].value(); }

  const std::vector<SSAPtr> &args() { return _args; }

  bool is_decl()              const { return _is_decl; }


  // methods for dyn_cast
  static inline bool classof(Function *) { return true; }
  static inline bool classof(const Function *) { return true; }
  static bool classof(Value *value) {
    if (value->classId() == ClassId::FunctionId) return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::FunctionId) return true;
    return false;
  }
};

class JumpInst : public TerminatorInst {
public:
  explicit JumpInst(const SSAPtr &target)
      : TerminatorInst(Instruction::TermOps::Jmp,1) {
    AddValue(target);
  }

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // virtual functions of TerminatorInst
  unsigned GetSuccessorNum() const override { return 1; }

  SSAPtr GetSuccessor(unsigned idx) const override {
    DBG_ASSERT(idx == 0, "index out of range");
    return target();
  };

  void SetSuccessor(unsigned idx, const BlockPtr &B) override {
    DBG_ASSERT(idx == 0, "index out of range");
    this->SetOperand(0, B);
  }

  const SSAPtr &target() const { return (*this)[0].value(); }

  // methods for dyn_cast
  static inline bool classof(JumpInst *) { return true; }
  static inline bool classof(const JumpInst *) { return true; }
  static bool classof(Value *value) {
    if (value->classId() == ClassId::JumpInstId) return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::JumpInstId) return true;
    return false;
  }
};


// return from function
// operand: value
class ReturnInst : public TerminatorInst {
public:
  explicit ReturnInst(const SSAPtr &value)
      : TerminatorInst(Instruction::TermOps::Ret,1)
  { AddValue(value); }

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // virtual functions of TerminatorInst
  unsigned GetSuccessorNum() const override { return 1; }

  SSAPtr GetSuccessor(unsigned idx) const override {
    DBG_ASSERT(idx == 0, "index out of range");
    auto succs = GetSuccessors();
    DBG_ASSERT(succs.size() == 1, "successors size error");
    return succs[idx];
  };

  void SetSuccessor(unsigned idx, const BlockPtr &BB) override {
    DBG_ASSERT(idx == 0, "index out of range");
    auto succs = GetSuccessors();
    succs[idx] = BB;
  }

  // getter/setter
  const SSAPtr &RetVal()    const { return (*this)[0].value(); }
  void SetRetVal(const SSAPtr &value) { (*this)[0].set(value); }

  // methods for dyn_cast
  static inline bool classof(ReturnInst *) { return true; }
  static inline bool classof(const ReturnInst *) { return true; }
  static bool classof(Value *value) {
    if (value->classId() == ClassId::ReturnInstId) return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::ReturnInstId) return true;
    return false;
  }
};

// branch with condition
// operands: cond true_block false_block
class BranchInst : public TerminatorInst {
public:
  BranchInst(const SSAPtr &cond, const SSAPtr &true_block, const SSAPtr &false_block);

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // virtual functions of TerminatorInst
  unsigned GetSuccessorNum() const override { return 3; }

  SSAPtr GetSuccessor(unsigned idx) const override {
    DBG_ASSERT(idx < 2, "index out of range");
    auto succs = GetSuccessors();
    DBG_ASSERT(succs.size() == 2, "successors size error");
    return succs[idx];
  };

  void SetSuccessor(unsigned idx, const BlockPtr &BB) override {
    DBG_ASSERT(idx < 2, "index out of range");
    auto succs = GetSuccessors();
    succs[idx] = BB;
  }

  // getter/setter
  const SSAPtr &cond()        const       { return (*this)[0].value(); }
  const SSAPtr &true_block()  const       { return (*this)[1].value(); }
  const SSAPtr &false_block() const       { return (*this)[2].value(); }
  void SetCond(const SSAPtr &value)       { (*this)[0].set(value);   }
  void SetTrueBlock(const SSAPtr &value)  { (*this)[1].set(value);   }
  void SetFalseBlock(const SSAPtr &value) { (*this)[2].set(value);   }

  // methods for dyn_cast
  static inline bool classof(BranchInst *) { return true; }
  static inline bool classof(const BranchInst *) { return true; }
  static bool classof(Value *value) {
    if (value->classId() == ClassId::BranchInstId) return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::BranchInstId) return true;
    return false;
  }
};

// store to alloc
// operands: value, pointer
class StoreInst : public Instruction {
public:
  StoreInst(const SSAPtr &V, const SSAPtr &P)
      : Instruction(Instruction::MemoryOps::Store, 2, ClassId::StoreInstId) {
    AddValue(V);
    AddValue(P);
  }

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // getters
  const SSAPtr &data() const { return (*this)[0].value(); }

  const SSAPtr &pointer() const { return (*this)[1].value(); }

  // methods for dyn_cast
  static inline bool classof(StoreInst *) { return true; }
  static inline bool classof(const StoreInst *) { return true; }
  static bool classof(Value *value) {
    if (value->classId() == ClassId::StoreInstId) return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::StoreInstId) return true;
    return false;
  }
};

// alloc on stack
class AllocaInst : public Instruction {
private:
  std::string _name;
public:
  AllocaInst()
      : Instruction(Instruction::MemoryOps::Alloca, 0, ClassId::AllocaInstId),
        _name(std::string("")) {}

  explicit AllocaInst(const std::string &name)
      : Instruction(Instruction::MemoryOps::Alloca, 0, ClassId::AllocaInstId),
        _name(name) {}

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  const std::string &name() const { return _name; }
  void set_name(const std::string &name) { _name = name; }

  // methods for dyn_cast
  static inline bool classof(AllocaInst *) { return true; }
  static inline bool classof(const AllocaInst *) { return true; }
  static bool classof(Value *value) {
    if (value->classId() == ClassId::AllocaInstId) return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::AllocaInstId) return true;
    return false;
  }
};

// load from pointer
// operands: pointer
// TODO: need extend or trunc for operands
class LoadInst : public Instruction {
private:
  std::string _name;
  std::weak_ptr<Value> _pointer;
public:
  LoadInst(const SSAPtr &ptr)
      : Instruction(Instruction::MemoryOps::Load, 1, ClassId::LoadInstId), _pointer(ptr) {
    AddValue(ptr);
  }

  bool isInstruction() const override { return true; }

  SSAPtr GetAddr() const override { return _pointer.lock(); }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // getter/setter
  void SetPointer(const SSAPtr &ptr)    { (*this)[0].set(ptr); }
  const SSAPtr &Pointer()         const { return (*this)[0].value(); }

  const std::string &name() const { return _name; }
  void set_name(const std::string &name) { _name = name; }

  // methods for dyn_cast
  static inline bool classof(LoadInst *) { return true; }
  static inline bool classof(const LoadInst *) { return true; }
  static bool classof(Value *value) {
    if (value->classId() == ClassId::LoadInstId) return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::LoadInstId) return true;
    return false;
  }
};


// argument reference
class ArgRefSSA : public Value {
private:
  SSAPtr      _func;
  std::size_t _index;
  std::string _arg_name;

public:
  ArgRefSSA(SSAPtr func, std::size_t index, std::string name)
      : Value(ClassId::ArgRefSSAId), _func(std::move(func)), _index(index), _arg_name(std::move(name)) {}

  bool isInstruction() const override { return false; }

  bool IsArgument() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // getter
  const SSAPtr      &func()    const { return _func;  }
  std::size_t       index()    const { return _index; }
  std::string arg_name() const { return _arg_name; }

  // methods for dyn_cast
  static inline bool classof(ArgRefSSA *) { return true; }
  static inline bool classof(const ArgRefSSA *) { return true; }
  static bool classof(Value *value) {
    if (value->classId() == ClassId::ArgRefSSAId) return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::ArgRefSSAId) return true;
    return false;
  }
};

// function call
// operands: callee, parameters
class CallInst : public Instruction {
public:
  CallInst(const SSAPtr &callee, const std::vector<SSAPtr> &args);

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // getter/setter
  const SSAPtr & Callee() const { return (*this)[0].value(); }

  // methods for dyn_cast
  static inline bool classof(CallInst *) { return true; }
  static inline bool classof(const CallInst *) { return true; }
  static bool classof(Value *value) {
    if (value->classId() == ClassId::CallInstId) return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::CallInstId) return true;
    return false;
  }
};

class ICmpInst : public Instruction {
private:
  using Operator = front::Operator;
  Operator _op;
public:

  ICmpInst(Operator op, const SSAPtr &lhs, const SSAPtr &rhs);

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  static unsigned GetNumOperands() { return 2; }

  // getter/setter
  Operator             op()   const { return _op;              }
  const SSAPtr       &LHS()   const { return (*this)[0].value(); }
  const SSAPtr       &RHS()   const { return (*this)[1].value(); }
  std::string         opStr() const;

  // methods for dyn_cast
  static inline bool classof(ICmpInst *) { return true; }
  static inline bool classof(const ICmpInst *) { return true; }
  static bool classof(Value *value) {
    if (value->classId() == ClassId::ICmpInstId) return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::ICmpInstId) return true;
    return false;
  }
};

// type casting
// operands: opr
class  CastInst : public Instruction {
public:
  explicit CastInst(CastOps op, const SSAPtr &opr)
  : Instruction(op, 1, ClassId::CallInstId) {
    AddValue(opr);
  }

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // getter/setter
  const SSAPtr &operand() const { return (*this)[0].value(); }

  // methods for dyn_cast
  static inline bool classof(CastInst *) { return true; }
  static inline bool classof(const CastInst *) { return true; }
  static bool classof(Value *value) {
    if (value->classId() == ClassId::CastInstId) return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::CastInstId) return true;
    return false;
  }
};

// global variable definition/declaration
// operands: initializer
class GlobalVariable : public User {
private:
  bool        _is_var;
  std::string _name;

public:
  GlobalVariable(bool is_var, const std::string &name, const SSAPtr &init)
    : User(ClassId::GlobalVariableId), _is_var(is_var), _name(name) {
    AddValue(init);
  }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // getter/setter
  bool               isVar()  const { return _is_var;          }
  const SSAPtr      &init()   const { return (*this)[0].value(); }
  const std::string &name()   const { return _name;            }

  void set_is_var(bool is_var)      { _is_var = is_var;        }
  void set_init(const SSAPtr &init) { (*this)[0].set(init);    }

  // methods for dyn_cast
  static inline bool classof(GlobalVariable *) { return true; }
  static inline bool classof(const GlobalVariable *) { return true; }
  static bool classof(Value *value) {
    if (value->classId() == ClassId::GlobalVariableId) return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::GlobalVariableId) return true;
    return false;
  }
};

bool IsCmp(const SSAPtr &ptr);
bool IsCallInst(const SSAPtr &ptr);
bool IsBinaryOperator(const SSAPtr &ptr);


// element accessing (load effective address)
// operands: ptr, index1, multiplier, ...
class AccessInst : public Instruction {
public:
  enum class AccessType { Pointer, Element };

private:
  AccessType _acc_type;

public:
  AccessInst(AccessType acc_type, const SSAPtr &ptr, const SSAPtrList &indexs);

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // getter/setter
  AccessType acc_type()      const { return _acc_type;        }
  const SSAPtr &ptr()        const { return (*this)[0].value(); }
  const SSAPtr &index()      const { return (*this)[1].value(); }
  const SSAPtr &index(int n) const { return (*this)[n].value(); }
  const SSAPtr &multiplier() const { return (*this)[2].value(); }
  void set_ptr(const SSAPtr &ptr)          { (*this)[0].set(ptr); }
  void set_index(const SSAPtr &idx, int n) { (*this)[n].set(idx); }

  // methods for dyn_cast
  static inline bool classof(AccessInst *) { return true; }
  static inline bool classof(const AccessInst *) { return true; }
  static bool classof(Value *value) {
    if (value->classId() == ClassId::AccessInstId) return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::AccessInstId) return true;
    return false;
  }
};

}
#endif //LAVA_SSA_H
