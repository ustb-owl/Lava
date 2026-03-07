#ifndef LAVA_SSA_H
#define LAVA_SSA_H

#include <utility>

#include "constant.h"
#include "define/ast.h"
#include "mid/ir/usedef/user.h"

namespace lava::mid {

class Module;

class BasicBlock : public Value {
private:
  InstList              _insts;
  std::vector<BlockPtr> _predecessors;
  std::string           _name;   // block name
  Function             *_parent; // block's getParent(function)

public:
  BasicBlock(Function *parent, std::string name)
      : Value(ClassId::BasicBlockId), _name(std::move(name)), _parent(parent) {}

  bool isInstruction() const final { return false; }
  bool isBlock() const final { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // dump CFG
  void Dump(std::ostream &os, IdManager &id_mgr,
            const std::string &separator) const;

  void setParent(Function *parent) { _parent = parent; }

  InstList::iterator AppendInst(const InstPtr &inst);

  InstList::iterator InsertInst(InstList::iterator pos, const InstPtr &inst);

  InstList::iterator InsertInstBefore(const InstPtr &insertBefore,
                                      const InstPtr &inst);

  InstList::iterator EraseInst(InstList::iterator pos);

  InstList::iterator EraseInst(const InstPtr &inst);

  void AppendInstsFrom(BasicBlock *other);

  void AddInstToEnd(const InstPtr &inst) {
    static_cast<void>(AppendInst(inst));
  }

  void AddInstBefore(const InstPtr &insertBefore, const SSAPtr &inst);

  bool HasPredecessor(const BasicBlock *pred) const;

  const std::vector<BlockPtr> &predecessors() const { return _predecessors; }

  std::size_t predecessor_count() const { return _predecessors.size(); }

  void AddPredecessor(const BlockPtr &pred);

  void RemovePredecessor(const BasicBlock *pred);

  bool ReplacePredecessor(const BasicBlock *oldPred, const BlockPtr &newPred);

  void ClearPredecessors();

  // remove all instructions
  void ClearInst();

  // delete it self
  void DeleteSelf();

  // getters
  InstList          &insts() { return _insts; }
  InstList::iterator inst_begin() { return _insts.begin(); }
  InstList::iterator inst_end() { return _insts.end(); }
  Function          *getParent() const { return _parent; }
  const std::string &name() const { return _name; }
  bool               empty() const { return _insts.empty(); }
  InstPtr            terminator() const {
    return _insts.empty() ? nullptr : _insts.back();
  }

  void SetBlockName(const std::string name) { _name = name; }

  std::vector<BasicBlock *> successors() const;

  // methods for dyn_cast
  static inline bool classof(BasicBlock *) { return true; }
  static inline bool classof(const BasicBlock *) { return true; }
  static bool        classof(Value *value);
  static bool        classof(const Value *value);
};

class Instruction : public User {
private:
  unsigned    _opcode;
  BasicBlock *_bb;

public:
  Instruction(unsigned opcode, unsigned operand_nums, ClassId classId);

  Instruction(unsigned opcode, unsigned operand_nums, const Operands &operands,
              ClassId classId);

  virtual ~Instruction() = default;

  void Dump(std::ostream &os, IdManager &id_mgr) const override {}

  // Accessor methods...
  unsigned opcode() const { return _opcode; }
  void     set_opcode(unsigned opcode) { _opcode = opcode; }

  std::string GetOpcodeAsString() const { return GetOpcodeAsString(opcode()); }

  static std::string GetOpcodeAsString(unsigned opcode);

  // Determine if the opcode is one of the terminators instruction.
  static inline bool isTerminator(unsigned OpCode) {
    return OpCode >= TermOpsBegin && OpCode < TermOpsEnd;
  }

  inline bool isTerminator() const { // Instance of TerminatorInst?
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

  // isLogicalShift - Return true if this is a logical shift left or a logical
  // shift right.
  inline bool isLogicalShift() const {
    return opcode() == Shl || opcode() == LShr;
  }

  // isLogicalShift - Return true if this is a logical shift left or a logical
  // shift right.
  inline bool isArithmeticShift() const { return opcode() == AShr; }

  // Determine if the opcode is one of the CastInst instruction.
  static inline bool isCast(unsigned Opcode) {
    return Opcode >= CastOpsBegin && Opcode <= CastOpsEnd;
  }

  inline bool isCast() const { return isCast(opcode()); }

  bool isInstruction() const override { return true; }

  BasicBlock       *getParent();
  const BasicBlock *getParent() const;
  void              setParent(BasicBlock *bb);

  // remove this instruction from parent
  InstList::iterator RemoveFromParent();

  // clear operands and erase this instruction from parent
  InstList::iterator EraseFromParent();

  // get position in the instruction list
  InstList::iterator GetPosition();

  // move this instruction before another instruction
  void MoveBefore(const InstPtr &insertBefore);

  // move this instruction to the end of a block, or before its terminator
  void MoveBeforeTerminator(BasicBlock *bb);

  //----------------------------------------------------------------------
  // Exported opcode enumerations...
  // TermOps, BinaryOps, MemoryOps, CastOps, OtherOps
  //
#include "opcode.inc"

  static const int AssignSpain = AssAdd - Add;

  // methods for dyn_cast
  static inline bool classof(Instruction *) { return true; }
  static inline bool classof(const Instruction *) { return true; }
  static bool        classof(Value *);
  static bool        classof(const Value *);
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

protected:
  void SetStoredSuccessor(unsigned idx, const BlockPtr &BB) {
    if (idx >= _successors.size())
      _successors.resize(idx + 1);
    _successors[idx] = BB;
  }

public:
  TerminatorInst(Instruction::TermOps opcode, unsigned operands_num)
      : Instruction(opcode, operands_num, static_cast<ClassId>(opcode)) {}

  TerminatorInst(Instruction::TermOps opcode, const Operands &operands,
                 unsigned operands_num, const SSAPtr &insertBefore = nullptr)
      : Instruction(opcode, operands_num, operands,
                    static_cast<ClassId>(opcode)) {}

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override {}

  // get all successors
  const std::vector<BlockPtr> &GetSuccessors() const { return _successors; }

  /* Virtual methods - Terminators should overload these methods. */

  // Return the number of successors that this terminator has.
  virtual unsigned GetSuccessorNum() const = 0;

  virtual BlockPtr GetSuccessor(unsigned idx) const               = 0;
  virtual void     SetSuccessor(unsigned idx, const BlockPtr &BB) = 0;
  virtual void AddSuccessor(const BlockPtr &BB) { _successors.push_back(BB); }
  virtual bool ReplaceSuccessor(BasicBlock     *oldBlock,
                                const BlockPtr &newBlock) = 0;

  // methods for dyn_cast
  static inline bool classof(TerminatorInst *) { return true; }
  static inline bool classof(const TerminatorInst *) { return true; }
  static bool        classof(Value *);
  static bool        classof(const Value *);
};

//===----------------------------------------------------------------------===//
//                           BinaryOperator Class
//===----------------------------------------------------------------------===//

class BinaryOperator : public Instruction {
public:
  BinaryOperator(BinaryOps opcode, const SSAPtr &S1, const SSAPtr &S2,
                 const define::TypePtr &type);

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  static unsigned GetNumOperands() { return 2; }

  static BinaryPtr Create(BinaryOps opcode, const SSAPtr &S1, const SSAPtr &S2);

  bool isInstruction() const override { return true; }

  // swap lhs and rhs
  bool swapOperand();

  // try to eval when both lhs and rhs are constant value
  SSAPtr EvalArithOnConst();

  // return true if fold successfully
  int TryToFold();

  SSAPtr OptimizedValue();

  // Create* - These methods just forward to create, and are useful when you
  // statically know what type of instruction you're going to create.  These
  // helpers just save some typing.
#define HANDLE_BINARY_INST(N, OPC, ClASS)                                      \
  static BinaryPtr Create##OPC(const SSAPtr &V1, const SSAPtr &V2) {           \
    return Create(Instruction::OPC, V1, V2);                                   \
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

  // getter/sertter
  const SSAPtr &LHS() const { return (*this)[0].value(); }
  const SSAPtr &RHS() const { return (*this)[1].value(); }
  BinaryOps     opcode() const { return BinaryOps(Instruction::opcode()); }
  void          SetLHS(const SSAPtr &value) { SetOperand(0, value); }
  void          SetRHS(const SSAPtr &value) { SetOperand(1, value); }

  // methods for dyn_cast
  static inline bool classof(BinaryOperator *) { return true; }
  static inline bool classof(const BinaryOperator *) { return true; }
  static bool        classof(Value *);
  static bool        classof(const Value *);
};

class Function : public Value {
private:
  using BlockStorage = std::vector<BlockPtr>;

  BlockStorage        _blocks;
  bool                _is_decl;
  bool                _is_copied;
  bool                _is_tail_recursion;
  std::vector<SSAPtr> _args;
  std::string         _function_name;
  Module             *_module;

public:
  explicit Function(std::string name, bool is_decl = false,
                    Module *module = nullptr)
      : Value(ClassId::FunctionId), _is_decl(is_decl), _is_copied(false),
        _is_tail_recursion(false), _function_name(std::move(name)),
        _module(module) {}

  bool isInstruction() const override { return false; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // setters
  void set_arg(std::size_t i, const SSAPtr &arg) {
    _args.resize(i + 1);
    _args[i] = arg;
  }

  void SetName(const std::string &name) { _function_name = name; }

  Module *getParent() { return _module; }

  const Module *getParent() const { return _module; }

  void setParent(Module *module) { _module = module; }

  void RemoveFromParent();
  void AppendBlock(const BlockPtr &block);
  void RemoveBlock(const BasicBlock *block);
  void RemoveNullBlocks();
  void ClearBlocks();

  void SetIsRecursion(bool value) { _is_tail_recursion = value; }

  // getters
  const std::string &GetFunctionName() const { return _function_name; }

  const BlockPtr &entry() const;
  BlockPtr        entry_block() const;

  std::vector<SSAPtr> &args() { return _args; }

  const std::vector<SSAPtr> &args() const { return _args; }

  BlockStorage &blocks() { return _blocks; }

  const BlockStorage &blocks() const { return _blocks; }

  auto begin() { return _blocks.begin(); }
  auto end() { return _blocks.end(); }
  auto begin() const { return _blocks.begin(); }
  auto end() const { return _blocks.end(); }

  BlockPtr &operator[](std::size_t pos) { return _blocks[pos]; }

  const BlockPtr &operator[](std::size_t pos) const { return _blocks[pos]; }

  std::size_t size() const { return _blocks.size(); }

  bool empty() const { return _blocks.empty(); }

  bool is_decl() const { return _is_decl; }
  bool is_tail_recursion() const { return _is_tail_recursion; }
  bool is_copied() const { return _is_copied; }
  void is_copied(bool val) { _is_copied = val; }

  using BlockList = std::vector<BlockPtr>;
  BlockList GetBlockList();

  // methods for dyn_cast
  static inline bool classof(Function *) { return true; }
  static inline bool classof(const Function *) { return true; }
  static bool        classof(Value *value) {
    if (value->classId() == ClassId::FunctionId)
      return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::FunctionId)
      return true;
    return false;
  }
};

class JumpInst : public TerminatorInst {
public:
  explicit JumpInst(const BlockPtr &target);

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // virtual functions of TerminatorInst
  unsigned GetSuccessorNum() const override { return 1; }

  BlockPtr GetSuccessor(unsigned idx) const override {
    DBG_ASSERT(idx == 0, "index out of range");
    return target();
  };

  void SetSuccessor(unsigned idx, const BlockPtr &B) override {
    DBG_ASSERT(idx == 0, "index out of range");
    SetStoredSuccessor(0, B);
  }

  bool ReplaceSuccessor(BasicBlock     *oldBlock,
                        const BlockPtr &newBlock) override {
    if (target().get() != oldBlock)
      return false;
    SetSuccessor(0, newBlock);
    return true;
  }

  BlockPtr target() const { return GetSuccessors()[0]; }

  // methods for dyn_cast
  static inline bool classof(JumpInst *) { return true; }
  static inline bool classof(const JumpInst *) { return true; }
  static bool        classof(Value *value) {
    if (value->classId() == ClassId::JumpInstId)
      return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::JumpInstId)
      return true;
    return false;
  }
};

// return from function
// operand: value
class ReturnInst : public TerminatorInst {
public:
  explicit ReturnInst(const SSAPtr &value)
      : TerminatorInst(Instruction::TermOps::Ret, 1) {
    AppendOperand(value);
  }

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // virtual functions of TerminatorInst
  unsigned GetSuccessorNum() const override { return 0; }

  BlockPtr GetSuccessor(unsigned idx) const override {
    static_cast<void>(idx);
    DBG_ASSERT(false, "return instruction has no successors");
    return nullptr;
  };

  void SetSuccessor(unsigned idx, const BlockPtr &BB) override {
    static_cast<void>(idx);
    static_cast<void>(BB);
    DBG_ASSERT(false, "return instruction has no successors");
  }

  bool ReplaceSuccessor(BasicBlock     *oldBlock,
                        const BlockPtr &newBlock) override {
    static_cast<void>(oldBlock);
    static_cast<void>(newBlock);
    return false;
  }

  // getter/setter
  const SSAPtr &RetVal() const { return (*this)[0].value(); }
  void          SetRetVal(const SSAPtr &value) { SetOperand(0, value); }

  // methods for dyn_cast
  static inline bool classof(ReturnInst *) { return true; }
  static inline bool classof(const ReturnInst *) { return true; }
  static bool        classof(Value *value) {
    if (value->classId() == ClassId::ReturnInstId)
      return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::ReturnInstId)
      return true;
    return false;
  }
};

// branch with condition
class BranchInst : public TerminatorInst {
public:
  BranchInst(const SSAPtr &cond, const BlockPtr &true_block,
             const BlockPtr &false_block);

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // virtual functions of TerminatorInst
  unsigned GetSuccessorNum() const override { return 2; }

  BlockPtr GetSuccessor(unsigned idx) const override {
    DBG_ASSERT(idx < 2, "index out of range");
    return (idx == 0) ? true_block() : false_block();
  };

  void SetSuccessor(unsigned idx, const BlockPtr &BB) override {
    DBG_ASSERT(idx < 2, "index out of range");
    if (idx == 0)
      SetTrueBlock(BB);
    else
      SetFalseBlock(BB);
  }

  bool ReplaceSuccessor(BasicBlock     *oldBlock,
                        const BlockPtr &newBlock) override {
    bool replaced = false;
    if (true_block().get() == oldBlock) {
      SetTrueBlock(newBlock);
      replaced = true;
    }
    if (false_block().get() == oldBlock) {
      SetFalseBlock(newBlock);
      replaced = true;
    }
    return replaced;
  }

  // getter/setter
  const SSAPtr &cond() const { return (*this)[0].value(); }
  BlockPtr      true_block() const { return GetSuccessors()[0]; }
  BlockPtr      false_block() const { return GetSuccessors()[1]; }
  void          SetCond(const SSAPtr &value) { SetOperand(0, value); }
  void          SetTrueBlock(const BlockPtr &value);
  void          SetFalseBlock(const BlockPtr &value);

  // methods for dyn_cast
  static inline bool classof(BranchInst *) { return true; }
  static inline bool classof(const BranchInst *) { return true; }
  static bool        classof(Value *value) {
    if (value->classId() == ClassId::BranchInstId)
      return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::BranchInstId)
      return true;
    return false;
  }
};

// store to alloc
// operands: value, pointer
class StoreInst : public Instruction {
public:
  StoreInst(const SSAPtr &V, const SSAPtr &P)
      : Instruction(Instruction::MemoryOps::Store, 2, ClassId::StoreInstId) {
    AppendOperand(V);
    AppendOperand(P);
  }

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // getters
  const SSAPtr &data() const { return (*this)[0].value(); }

  const SSAPtr &pointer() const { return (*this)[1].value(); }
  void          SetData(const SSAPtr &value) { SetOperand(0, value); }
  void          SetPointer(const SSAPtr &value) { SetOperand(1, value); }

  // methods for dyn_cast
  static inline bool classof(StoreInst *) { return true; }
  static inline bool classof(const StoreInst *) { return true; }
  static bool        classof(Value *value) {
    if (value->classId() == ClassId::StoreInstId)
      return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::StoreInstId)
      return true;
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
  void               set_name(const std::string &name) { _name = name; }

  // methods for dyn_cast
  static inline bool classof(AllocaInst *) { return true; }
  static inline bool classof(const AllocaInst *) { return true; }
  static bool        classof(Value *value) {
    if (value->classId() == ClassId::AllocaInstId)
      return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::AllocaInstId)
      return true;
    return false;
  }
};

// load from pointer
// operands: pointer
// TODO: need extend or trunc for operands
class LoadInst : public Instruction {
private:
  std::string          _name;
  std::weak_ptr<Value> _pointer;

public:
  LoadInst(const SSAPtr &ptr)
      : Instruction(Instruction::MemoryOps::Load, 1, ClassId::LoadInstId),
        _pointer(ptr) {
    AppendOperand(ptr);
  }

  bool isInstruction() const override { return true; }

  SSAPtr GetAddr() const override { return _pointer.lock(); }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // getter/setter
  void          SetPointer(const SSAPtr &ptr) { SetOperand(0, ptr); }
  const SSAPtr &Pointer() const { return (*this)[0].value(); }

  const std::string &name() const { return _name; }
  void               set_name(const std::string &name) { _name = name; }

  // methods for dyn_cast
  static inline bool classof(LoadInst *) { return true; }
  static inline bool classof(const LoadInst *) { return true; }
  static bool        classof(Value *value) {
    if (value->classId() == ClassId::LoadInstId)
      return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::LoadInstId)
      return true;
    return false;
  }
};

// argument reference
class ArgRefSSA : public Value {
private:
  FuncPtr     _func;
  std::size_t _index;
  std::string _arg_name;

public:
  ArgRefSSA(FuncPtr func, std::size_t index, std::string name)
      : Value(ClassId::ArgRefSSAId), _func(std::move(func)), _index(index),
        _arg_name(std::move(name)) {}

  bool isInstruction() const override { return false; }

  bool IsArgument() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // getter
  const FuncPtr &func() const { return _func; }
  std::size_t    index() const { return _index; }
  std::string    arg_name() const { return _arg_name; }

  // methods for dyn_cast
  static inline bool classof(ArgRefSSA *) { return true; }
  static inline bool classof(const ArgRefSSA *) { return true; }
  static bool        classof(Value *value) {
    if (value->classId() == ClassId::ArgRefSSAId)
      return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::ArgRefSSAId)
      return true;
    return false;
  }
};

// function call
// operands: parameters
class CallInst : public Instruction {
private:
  FuncPtr _callee;
  bool    _is_tail_call;

public:
  CallInst(const FuncPtr &callee, const std::vector<SSAPtr> &args);

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // getter/setter
  const FuncPtr &Callee() const { return _callee; }
  const SSAPtr  &Param(int i) const { return (*this)[i].value(); }
  int            param_size() const { return size(); }

  void AddParam(const SSAPtr &param);

  bool IsTailCall() const { return _is_tail_call; }
  void SetIsTailCall(bool value) { _is_tail_call = value; }

  // methods for dyn_cast
  static inline bool classof(CallInst *) { return true; }
  static inline bool classof(const CallInst *) { return true; }
  static bool        classof(Value *value) {
    if (value->classId() == ClassId::CallInstId)
      return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::CallInstId)
      return true;
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

  SSAPtr EvalArithOnConst();

  // getter/setter
  Operator      op() const { return _op; }
  const SSAPtr &LHS() const { return (*this)[0].value(); }
  const SSAPtr &RHS() const { return (*this)[1].value(); }
  std::string   opStr() const;
  void          SetOp(Operator op) { _op = op; }
  void          SetLHS(const SSAPtr &value) { SetOperand(0, value); }
  void          SetRHS(const SSAPtr &value) { SetOperand(1, value); }

  // methods for dyn_cast
  static inline bool classof(ICmpInst *) { return true; }
  static inline bool classof(const ICmpInst *) { return true; }
  static bool        classof(Value *value) {
    if (value->classId() == ClassId::ICmpInstId)
      return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::ICmpInstId)
      return true;
    return false;
  }
};

// type casting
// operands: opr
class CastInst : public Instruction {
public:
  explicit CastInst(CastOps op, const SSAPtr &opr)
      : Instruction(op, 1, ClassId::CastInstId) {
    AppendOperand(opr);
  }

  bool isInstruction() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // getter/setter
  const SSAPtr &operand() const { return (*this)[0].value(); }

  // methods for dyn_cast
  static inline bool classof(CastInst *) { return true; }
  static inline bool classof(const CastInst *) { return true; }
  static bool        classof(Value *value) {
    if (value->classId() == ClassId::CastInstId)
      return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::CastInstId)
      return true;
    return false;
  }
};

// global variable definition/declaration
// operands: initializer
class GlobalVariable : public User {
private:
  bool        _is_var;
  std::string _name;
  Module     *_module;

public:
  GlobalVariable(bool is_var, const std::string &name, const SSAPtr &init,
                 Module *module = nullptr)
      : User(ClassId::GlobalVariableId), _is_var(is_var), _name(name),
        _module(module) {
    AppendOperand(init);
  }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // getter/setter
  bool               isVar() const { return _is_var; }
  const SSAPtr      &init() const { return (*this)[0].value(); }
  const std::string &name() const { return _name; }

  void set_is_var(bool is_var) { _is_var = is_var; }
  void set_init(const SSAPtr &init) { SetOperand(0, init); }

  Module *getParent() { return _module; }

  const Module *getParent() const { return _module; }

  void setParent(Module *module) { _module = module; }

  // methods for dyn_cast
  static inline bool classof(GlobalVariable *) { return true; }
  static inline bool classof(const GlobalVariable *) { return true; }
  static bool        classof(Value *value) {
    if (value->classId() == ClassId::GlobalVariableId)
      return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::GlobalVariableId)
      return true;
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
  AccessType    acc_type() const { return _acc_type; }
  const SSAPtr &ptr() const { return (*this)[0].value(); }
  const SSAPtr &index() const { return (*this)[1].value(); }
  const SSAPtr &index(int n) const { return (*this)[n].value(); }
  const SSAPtr &multiplier() const { return (*this)[2].value(); }
  void          set_ptr(const SSAPtr &ptr) { SetOperand(0, ptr); }
  void          set_index(const SSAPtr &idx, int n) { SetOperand(n, idx); }

  bool has_multiplier() const { return this->size() == 3; }

  // methods for dyn_cast
  static inline bool classof(AccessInst *) { return true; }
  static inline bool classof(const AccessInst *) { return true; }
  static bool        classof(Value *value) {
    if (value->classId() == ClassId::AccessInstId)
      return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::AccessInstId)
      return true;
    return false;
  }
};

class UnDefineValue : public Value {
public:
  UnDefineValue() : Value(ClassId::UnDefineValueId) {}

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  // methods for dyn_cast
  static inline bool classof(UnDefineValue *) { return true; }
  static inline bool classof(const UnDefineValue *) { return true; }
  static bool        classof(Value *value) {
    if (value->classId() == ClassId::UnDefineValueId)
      return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::UnDefineValueId)
      return true;
    return false;
  }
};

// phi node
// incoming edges: [pred, value], ...
class PhiNode : public Instruction {
public:
  using IncomingValue  = std::pair<BlockPtr, SSAPtr>;
  using IncomingValues = std::vector<IncomingValue>;

private:
  struct Incoming {
    BlockPtr pred;
    Use      value;

    Incoming(PhiNode *phi, const BlockPtr &incoming_pred,
             const SSAPtr &incoming_value)
        : pred(incoming_pred), value(incoming_value, phi) {}
  };

  std::vector<Incoming> _incoming;

public:
  explicit PhiNode(BasicBlock *BB)
      : Instruction(Instruction::OtherOps::PHI, 0, ClassId::PHINodeId) {
    setParent(BB);
    for (const auto &pred : BB->predecessors()) {
      _incoming.emplace_back(this, pred, nullptr);
    }
  }

  std::vector<BlockPtr> blocks() const;

  std::vector<BlockPtr> incomingBlocks() const { return blocks(); }

  IncomingValues incomingValues() const;

  unsigned operandNum() const override {
    return static_cast<unsigned>(_incoming.size());
  }

  unsigned size() const override {
    return static_cast<unsigned>(_incoming.size());
  }

  bool empty() const override { return _incoming.empty(); }

  SSAPtr GetOperand(unsigned i) const override {
    DBG_ASSERT(i < _incoming.size(), "getOperand() out of range");
    return _incoming[i].value.value();
  }

  void SetOperand(unsigned i, const SSAPtr &V) override {
    DBG_ASSERT(i < _incoming.size(), "setOperand() out of range");
    _incoming[i].value.set(V);
  }

  Use &GetOperandUse(unsigned i) override {
    DBG_ASSERT(i < _incoming.size(), "getOperandUse() out of range");
    return _incoming[i].value;
  }

  const Use &GetOperandUse(unsigned i) const override {
    DBG_ASSERT(i < _incoming.size(), "getOperandUse() out of range");
    return _incoming[i].value;
  }

  void ResetIncoming(const IncomingValues &incoming);

  void addIncoming(const BlockPtr &pred, const SSAPtr &value);

  BlockPtr getIncomingBlock(unsigned int i) const {
    DBG_ASSERT(i < _incoming.size(), "PHI index out of bound");
    return _incoming[i].pred;
  }

  BlockPtr getIncomingBlock(const Use &val) const;

  int incomingIndexOf(const BasicBlock *pred) const;

  SSAPtr getIncomingValueAt(unsigned int i) const;

  SSAPtr getIncomingValue(const BasicBlock *pred) const;

  void setIncomingValueAt(unsigned int i, const SSAPtr &value);

  void setIncomingValue(const BasicBlock *pred, const SSAPtr &value);

  void replaceIncomingBlock(const BasicBlock *oldPred, const BlockPtr &newPred);

  void removeIncoming(const BasicBlock *pred);

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  bool isInstruction() const override { return true; }

  // methods for dyn_cast
  static inline bool classof(PhiNode *) { return true; }
  static inline bool classof(const PhiNode *) { return true; }
  static bool        classof(Value *value) {
    if (value->classId() == ClassId::PHINodeId)
      return true;
    return false;
  }
  static bool classof(const Value *value) {
    if (value->classId() == ClassId::PHINodeId)
      return true;
    return false;
  }
};

void DumpBlockName(std::ostream &os, IdManager &id_mgr,
                   const BasicBlock *block);

} // namespace lava::mid
#endif // LAVA_SSA_H
