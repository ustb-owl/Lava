#include "ssa.h"

#include <algorithm>
#include <iostream>
#include <queue>

#include "common/casting.h"
#include "common/idmanager.h"
#include "constant.h"
#include "define/ast.h"
#include "define/type.h"
#include "lib/guard.h"
#include "mid/ir/module.h"

namespace lava::mid {

Function::BlockList Function::GetBlockList() { return _blocks; }

InstList::iterator BasicBlock::AppendInst(const InstPtr &inst) {
  return InsertInst(_insts.end(), inst);
}

InstList::iterator BasicBlock::InsertInst(InstList::iterator pos,
                                          const InstPtr     &inst) {
  auto inserted = _insts.insert(pos, inst);
  inst->setParent(this);
  return inserted;
}

InstList::iterator BasicBlock::InsertInstBefore(const InstPtr &insertBefore,
                                                const InstPtr &inst) {
  auto pos = std::find_if(_insts.begin(), _insts.end(),
                          [&insertBefore](const InstPtr &current) {
                            return current == insertBefore;
                          });
  DBG_ASSERT(pos != _insts.end(),
             "insertBefore instruction is not in this block");
  return InsertInst(pos, inst);
}

void BasicBlock::AddInstBefore(const InstPtr &insertBefore,
                               const SSAPtr  &inst) {
  auto instruction = dyn_cast<Instruction>(inst);
  DBG_ASSERT(instruction != nullptr, "inserted value is not an instruction");
  static_cast<void>(InsertInstBefore(insertBefore, instruction));
}

InstList::iterator BasicBlock::EraseInst(InstList::iterator pos) {
  DBG_ASSERT(pos != _insts.end(), "erase position is out of range");
  (*pos)->setParent(nullptr);
  return _insts.erase(pos);
}

InstList::iterator BasicBlock::EraseInst(const InstPtr &inst) {
  auto pos =
      std::find_if(_insts.begin(), _insts.end(),
                   [&inst](const InstPtr &current) { return current == inst; });
  DBG_ASSERT(pos != _insts.end(), "instruction is not in this block");
  return EraseInst(pos);
}

void BasicBlock::AppendInstsFrom(BasicBlock *other) {
  if ((other == nullptr) || (other == this) || other->_insts.empty())
    return;
  for (auto &inst : other->_insts) {
    inst->setParent(this);
  }
  _insts.splice(_insts.end(), other->_insts);
}

bool BasicBlock::HasPredecessor(const BasicBlock *pred) const {
  return std::any_of(
      _predecessors.begin(), _predecessors.end(),
      [pred](const BlockPtr &candidate) { return candidate.get() == pred; });
}

void BasicBlock::AddPredecessor(const BlockPtr &pred) {
  if (!pred || HasPredecessor(pred.get()))
    return;
  _predecessors.push_back(pred);
}

void BasicBlock::RemovePredecessor(const BasicBlock *pred) {
  std::erase_if(_predecessors, [pred](const BlockPtr &candidate) {
    return candidate.get() == pred;
  });
}

bool BasicBlock::ReplacePredecessor(const BasicBlock *oldPred,
                                    const BlockPtr   &newPred) {
  bool replaced = false;
  for (auto &pred : _predecessors) {
    if (pred.get() != oldPred)
      continue;
    pred     = newPred;
    replaced = true;
  }
  for (auto inst_it = inst_begin(); inst_it != inst_end(); ++inst_it) {
    auto phi = dyn_cast<PhiNode>(*inst_it);
    if (!phi)
      break;
    phi->replaceIncomingBlock(oldPred, newPred);
  }
  return replaced;
}

void BasicBlock::ClearPredecessors() { _predecessors.clear(); }

void Function::RemoveFromParent() {
  auto &functions = _module->Functions();
  for (auto it = functions.begin(); it != functions.end(); it++) {
    if (it->get() == this) {
      functions.erase(it);
      return;
    }
  }
}

void Function::AppendBlock(const BlockPtr &block) {
  DBG_ASSERT(block != nullptr, "appended block is nullptr");
  block->setParent(this);
  _blocks.push_back(block);
}

void Function::RemoveBlock(const BasicBlock *block) {
  std::erase_if(_blocks, [block](const BlockPtr &candidate) {
    if (!candidate || candidate.get() != block)
      return false;
    candidate->setParent(nullptr);
    return true;
  });
}

void Function::RemoveNullBlocks() { std::erase(_blocks, nullptr); }

void Function::ClearBlocks() {
  for (auto &block : _blocks) {
    if (block)
      block->setParent(nullptr);
  }
  _blocks.clear();
}

const BlockPtr &Function::entry() const {
  DBG_ASSERT(!_blocks.empty(), "function has no entry block");
  return _blocks.front();
}

BlockPtr Function::entry_block() const {
  return empty() ? nullptr : _blocks.front();
}

InstList::iterator Instruction::RemoveFromParent() {
  DBG_ASSERT(size() == 0, "The operand of this instruction is not cleared");
  auto BB = getParent();
  for (auto it = BB->inst_begin(); it != BB->inst_end(); it++) {
    if (it->get() == this) {
      return BB->EraseInst(it);
    }
  }
  return BB->inst_end();
}

InstList::iterator Instruction::EraseFromParent() {
  DBG_ASSERT(uses().empty(), "can't erase instruction with live users");
  Clear();
  return RemoveFromParent();
}

InstList::iterator Instruction::GetPosition() {
  auto BB = getParent();
  for (auto it = BB->inst_begin(); it != BB->inst_end(); it++) {
    if (it->get() == this) {
      return it;
    }
  }
  return BB->inst_end();
}

void Instruction::MoveBefore(const InstPtr &insertBefore) {
  DBG_ASSERT(insertBefore != nullptr, "insertBefore instruction is nullptr");
  auto *srcBB  = getParent();
  auto *dstBB  = insertBefore->getParent();
  auto  srcPos = GetPosition();
  auto  dstPos = insertBefore->GetPosition();
  DBG_ASSERT(srcPos != srcBB->inst_end(),
             "source instruction is not in parent block");
  DBG_ASSERT(dstPos != dstBB->inst_end(),
             "target instruction is not in parent block");
  if ((srcBB == dstBB) && (srcPos == dstPos))
    return;
  dstBB->insts().splice(dstPos, srcBB->insts(), srcPos);
  setParent(dstBB);
}

void Instruction::MoveBeforeTerminator(BasicBlock *bb) {
  DBG_ASSERT(bb != nullptr, "target block is nullptr");
  auto terminator = dyn_cast<Instruction>(bb->terminator());
  if (terminator != nullptr) {
    if (terminator.get() == this)
      return;
    MoveBefore(terminator);
    return;
  }

  auto *srcBB  = getParent();
  auto  srcPos = GetPosition();
  DBG_ASSERT(srcPos != srcBB->inst_end(),
             "source instruction is not in parent block");
  if ((srcBB == bb) && (std::next(srcPos) == srcBB->inst_end()))
    return;
  bb->insts().splice(bb->inst_end(), srcBB->insts(), srcPos);
  setParent(bb);
}

Instruction::Instruction(unsigned opcode, unsigned operand_nums,
                         ClassId classId)
    : User(classId, operand_nums), _opcode(opcode), _bb(nullptr) {}

Instruction::Instruction(unsigned opcode, unsigned operand_nums,
                         const Operands &operands, ClassId classId)
    : User(classId, operand_nums, operands), _opcode(opcode), _bb(nullptr) {}

BasicBlock *Instruction::getParent() {
  DBG_ASSERT(_bb != nullptr, "The getParent basic block is nullptr");
  return _bb;
}

const BasicBlock *Instruction::getParent() const {
  DBG_ASSERT(_bb != nullptr, "The getParent basic block is nullptr");
  return _bb;
}

void Instruction::setParent(lava::mid::BasicBlock *bb) { _bb = bb; }

bool Instruction::classof(Value *value) {
  switch (value->classId()) {
  case ClassId::UndefId:
  case ClassId::FunctionId:
  case ClassId::BasicBlockId:
  case ClassId::ArgRefSSAId:
  case ClassId::GlobalVariableId:
  case ClassId::ConstantIntId:
  case ClassId::ConstantArrayId:
  case ClassId::ConstantStringId:
    return false;
  default:
    return true;
  }
}

bool Instruction::classof(const Value *value) {
  switch (value->classId()) {
  case ClassId::UndefId:
  case ClassId::FunctionId:
  case ClassId::BasicBlockId:
  case ClassId::ArgRefSSAId:
  case ClassId::GlobalVariableId:
  case ClassId::ConstantIntId:
  case ClassId::ConstantArrayId:
  case ClassId::ConstantStringId:
    return false;
  default:
    return true;
  }
}

std::string Instruction::GetOpcodeAsString(unsigned int opcode) {
  switch (opcode) {
  // Terminators
  case Br:
    return "br";
  case Ret:
    return "ret";
  case Jmp:
    return "br";

    // Standard binary operators...
  case Add:
    return "add";
  case Sub:
    return "sub";
  case Mul:
    return "mul";
  case UDiv:
    return "udiv";
  case SDiv:
    return "sdiv";
    //      case FDiv: return "fdiv";
  case URem:
    return "urem";
  case SRem:
    return "srem";
    //      case FRem: return "frem";

    // Logical operators...
  case And:
    return "and";
  case Or:
    return "or";
  case Xor:
    return "xor";

    // Memory instructions...
  case Malloc:
    return "malloc";
  case Free:
    return "free";
  case Alloca:
    return "alloca";
  case Load:
    return "load";
  case Store:
    return "store";
    //      case GetElementPtr: return "getelementptr";

    // Convert instructions...
  case Trunc:
    return "trunc";
  case ZExt:
    return "zext";
  case SExt:
    return "sext";
#if 0
      case FPTrunc:   return "fptrunc";
      case FPExt:     return "fpext";
      case FPToUI:    return "fptoui";
      case FPToSI:    return "fptosi";
      case UIToFP:    return "uitofp";
      case SIToFP:    return "sitofp";
#endif
  case IntToPtr:
    return "inttoptr";
  case PtrToInt:
    return "ptrtoint";
  case BitCast:
    return "bitcast";

    // Other instructions...
  case ICmp:
    return "icmp";
    //      case FCmp:           return "fcmp";
  case PHI:
    return "phi";
  case Select:
    return "select";
  case Call:
    return "call";
  case Shl:
    return "shl";
  case LShr:
    return "lshr";
  case AShr:
    return "ashr";
  case VAArg:
    return "va_arg";
  case ExtractElement:
    return "extractelement";
  case InsertElement:
    return "insertelement";
  case ShuffleVector:
    return "shufflevector";

  default:
    return "<Invalid operator> ";
  }
  return "";
}

bool TerminatorInst::classof(Value *value) {
  switch (value->classId()) {
  case ClassId::ReturnInstId:
  case ClassId::BranchInstId:
  case ClassId::JumpInstId:
    return true;
  default:
    return false;
  }
}

bool TerminatorInst::classof(const Value *value) {
  switch (value->classId()) {
  case ClassId::ReturnInstId:
  case ClassId::BranchInstId:
  case ClassId::JumpInstId:
    return true;
  default:
    return false;
  }
}

//===----------------------------------------------------------------------===//
//                           BinaryOperator Class
//===----------------------------------------------------------------------===//

constexpr static std::pair<BinaryOperator::BinaryOps, BinaryOperator::BinaryOps>
    swapableOperators[11] = {
        {BinaryOperator::BinaryOps::Add, BinaryOperator::BinaryOps::Add},
        //    {BinaryOperator::BinaryOps::Sub, BinaryOperator::BinaryOps::Rsb},
        {BinaryOperator::BinaryOps::Mul, BinaryOperator::BinaryOps::Mul},
        {BinaryOperator::BinaryOps::And, BinaryOperator::BinaryOps::And},
        {BinaryOperator::BinaryOps::Or, BinaryOperator::BinaryOps::Or},
};

BinaryOperator::BinaryOperator(Instruction::BinaryOps opcode, const SSAPtr &S1,
                               const SSAPtr &S2, const define::TypePtr &type)
    : Instruction(opcode, 2, ClassId::BinaryOperatorId) {
  set_type(type);
  AppendOperand(S1);
  AppendOperand(S2);
}

bool BinaryOperator::swapOperand() {
  for (auto [before, after] : swapableOperators) {
    if (opcode() == before) {
      std::swap((*this)[0], (*this)[1]);
      return true;
    }
  }
  return false;
}

SSAPtr BinaryOperator::EvalArithOnConst() {
  DBG_ASSERT(LHS()->classId() == ClassId::ConstantIntId,
             "lhs value is not constant int");
  DBG_ASSERT(RHS()->classId() == ClassId::ConstantIntId,
             "rhs value is not constant int");

  auto lhs_imm = dyn_cast<ConstantInt>(LHS())->value();
  auto rhs_imm = dyn_cast<ConstantInt>(RHS())->value();
  int  result  = 0;
  switch (opcode()) {
  case Add:
    result = lhs_imm + rhs_imm;
    break;
  case Sub:
    result = lhs_imm - rhs_imm;
    break;
  case Mul:
    result = lhs_imm * rhs_imm;
    break;
  case SDiv:
    result = lhs_imm / rhs_imm;
    break;
  case SRem:
    result = lhs_imm % rhs_imm;
    break;
  case And:
    result = lhs_imm & rhs_imm;
    break;
  case Or:
    result = lhs_imm | rhs_imm;
    break;
  case Xor:
    result = lhs_imm ^ rhs_imm;
    break;
  case Shl:
    result = lhs_imm << rhs_imm;
    break;
  case AShr:
    result = lhs_imm >> rhs_imm;
    break;
  default:
    ERROR("should not reach here");
  }
  auto const_int = std::make_shared<ConstantInt>(result);
  DBG_ASSERT(const_int != nullptr, "create const fold value failed");
  const_int->set_type(type());
  return const_int;
}

/*
 * Add:                           <---|
 * b = a + 1; c = b + 2;              |
 *   => c = a + 3                     |
 * Sub                                |
 * b = a - 1; c = b - 2;              |
 *   => b = a + (-1); c = b + (-2) ---|
 *
 * b = 1 - a; c = b - 2;
 *   => c = -1 - a
 *
 * Mul
 * b = a * 2; c = b * 3;
 *   => c = a * 6;
 */

int BinaryOperator::TryToFold() {
  bool res = false;
  if (auto rhs = dyn_cast<ConstantInt>(RHS())) {
    // a - 1 ---> a + (-1)
    if (opcode() == BinaryOps::Sub) {
      auto neg_rhs = std::make_shared<ConstantInt>(-rhs->value());
      neg_rhs->set_type(rhs->type());

      // replace rhs with negitive value
      SetRHS(neg_rhs);
      // change sub to add
      set_opcode(BinaryOps::Add);
    }

    if (auto lhs_bin_inst = dyn_cast<BinaryOperator>(LHS())) {
      // 1. Add: b = a + 1; c = b + 2; ---> c = a + 3;
      // 2. Mul: b = a * 2; c = b * 3; ---> c = a * 6;
      if (auto lhs_bin_rhs = dyn_cast<ConstantInt>(lhs_bin_inst->RHS())) {
        // a - 1 ---> a + (-1)
        if (lhs_bin_inst->opcode() == BinaryOps::Sub) {
          auto neg_rhs = std::make_shared<ConstantInt>(-lhs_bin_rhs->value());
          neg_rhs->set_type(lhs_bin_rhs->type());

          lhs_bin_inst->SetRHS(neg_rhs);
          set_opcode(BinaryOps::Add);
        }

        if ((opcode() == BinaryOps::Add) &&
            (lhs_bin_inst->opcode() == BinaryOps::Add)) {
          SetLHS(lhs_bin_inst->LHS());
          int value =
              lhs_bin_rhs->value() + dyn_cast<ConstantInt>(RHS())->value();
          auto new_rhs = std::make_shared<ConstantInt>(value);
          new_rhs->set_type(RHS()->type());
          SetRHS(new_rhs);
          res = true;
        } else if ((opcode() == BinaryOps::Mul) &&
                   (lhs_bin_inst->opcode() == BinaryOps::Mul)) {
          SetLHS(lhs_bin_inst->LHS());
          int value =
              lhs_bin_rhs->value() * dyn_cast<ConstantInt>(RHS())->value();
          auto new_rhs = std::make_shared<ConstantInt>(value);
          new_rhs->set_type(RHS()->type());
          SetRHS(new_rhs);
          res = true;
        }
      } else if (auto lhs_bin_lhs =
                     dyn_cast<ConstantInt>(lhs_bin_inst->LHS())) {
        // 3. Sub: b = 3 - a; c = b + 4 ---> c = 7 - a;
        if (lhs_bin_inst->opcode() == BinaryOps::Sub) {
          if (opcode() == BinaryOps::Add) {
            int value =
                lhs_bin_lhs->value() + dyn_cast<ConstantInt>(RHS())->value();
            auto new_lhs = std::make_shared<ConstantInt>(value);
            new_lhs->set_type(LHS()->type());
            SetLHS(new_lhs);
            SetRHS(lhs_bin_inst->RHS());
            set_opcode(BinaryOps::Sub);
            res = true;
          }
        }
      }
    }
  } else if (auto lhs = dyn_cast<ConstantInt>(LHS())) {
    // 4. Sub: b = 1 - a; c = 3 - b; ---> c = a + 2;
    if (auto rhs_bin_inst = dyn_cast<BinaryOperator>(RHS())) {
      if (auto rhs_bin_lhs = dyn_cast<ConstantInt>(rhs_bin_inst->LHS())) {
        if ((opcode() == BinaryOps::Sub) &&
            (rhs_bin_inst->opcode() == BinaryOps::Sub)) {
          SetLHS(rhs_bin_inst->RHS());
          auto new_lhs = std::make_shared<ConstantInt>(lhs->value() -
                                                       rhs_bin_lhs->value());
          new_lhs->set_type(lhs->type());
          SetRHS(new_lhs);
          set_opcode(BinaryOps::Add);
          res = true;
        }
      }
    }
  }

  return res;
}

SSAPtr BinaryOperator::OptimizedValue() {
  SSAPtr result = nullptr;
  if (auto rhs = dyn_cast<ConstantInt>(RHS())) {
    switch (opcode()) {
    case Add:
    case Sub: {
      result = (rhs->IsZero()) ? LHS() : nullptr;
      break;
    }
    case Mul: {
      if (rhs->IsZero()) {
        result = std::make_shared<ConstantInt>(0);
        result->set_type(rhs->type());
      } else if (rhs->value() == 1) {
        result = LHS();
      }
      break;
    }
    case SDiv: {
      if (rhs->value() == 1) {
        result = LHS();
      }
      break;
    }
    case SRem: {
      if (rhs->value() == 1) {
        result = std::make_shared<ConstantInt>(0);
        result->set_type(rhs->type());
      }
      break;
    }
    case And: {
      if (rhs->value() == 0) {
        result = std::make_shared<ConstantInt>(0);
        result->set_type(rhs->type());
      }
      break;
    }
    case Or: {
      if (rhs->value() == 1) {
        result = std::make_shared<ConstantInt>(1);
        result->set_type(rhs->type());
      }
      break;
    }
    case Xor:
    case Shl:
    case AShr:
      break;
    default:
      ERROR("should not reach here");
    }
  }
  return result;
}

BinaryPtr BinaryOperator::Create(Instruction::BinaryOps opcode,
                                 const SSAPtr &S1, const SSAPtr &S2) {
  auto s1_type = S1->type();
  auto s2_type = S2->type();
  DBG_ASSERT(s1_type->IsPrime(), "S1 is not prime type");
  DBG_ASSERT(s1_type->IsInteger(),
             "binary operator can only being performed on int");
  DBG_ASSERT(s1_type->GetSize() == s2_type->GetSize(),
             "S1 has different type with S2");

  return std::make_shared<BinaryOperator>(opcode, S1, S2, s1_type);
}

BinaryPtr BinaryOperator::createNeg(const SSAPtr &Op) {
  auto typeInfo = Op->type();
  DBG_ASSERT(typeInfo->IsInteger(), "Neg operator is not integer");
  auto zero = GetZeroValue(typeInfo->GetType());
  return std::make_shared<BinaryOperator>(Instruction::Sub, zero, Op, typeInfo);
}

BinaryPtr BinaryOperator::createNot(const SSAPtr &Op) {
  auto typeInfo = Op->type();
  DBG_ASSERT(typeInfo->IsInteger(), "Not operator is not integer");
  auto C = GetAllOneValue(typeInfo->GetType());
  return std::make_shared<BinaryOperator>(Instruction::Xor, Op, C, typeInfo);
}

bool BinaryOperator::classof(Value *value) {
  if (value->classId() == ClassId::BinaryOperatorId)
    return true;
  return false;
}

bool BinaryOperator::classof(const Value *value) {
  if (value->classId() == ClassId::BinaryOperatorId)
    return true;
  return false;
}

std::vector<BasicBlock *> BasicBlock::successors() const {
  std::vector<BasicBlock *> succes;
  auto                      term = dyn_cast<TerminatorInst>(terminator());
  if (term == nullptr)
    return succes;
  for (const auto &succ : term->GetSuccessors()) {
    if (succ)
      succes.push_back(succ.get());
  }
  return succes;
}

bool BasicBlock::classof(Value *value) {
  switch (value->classId()) {
  case ClassId::BasicBlockId:
    return true;
  default:
    return false;
  }
}

bool BasicBlock::classof(const Value *value) {
  switch (value->classId()) {
  case ClassId::BasicBlockId:
    return true;
  default:
    return false;
  }
}

JumpInst::JumpInst(const BlockPtr &target)
    : TerminatorInst(Instruction::TermOps::Jmp, 0) {
  SetStoredSuccessor(0, target);
}

void BasicBlock::ClearInst() {
  for (const auto &i : _insts) {
    // remove all of its uses
    auto inst = dyn_cast<Instruction>(i);
    inst->ClearOperands();
    inst->setParent(nullptr);
  }

  // clear instruction list
  _insts.clear();
}

void BasicBlock::DeleteSelf() {
  _parent = nullptr;
  _predecessors.clear();
  this->ClearInst();
}

BranchInst::BranchInst(const SSAPtr &cond, const BlockPtr &true_block,
                       const BlockPtr &false_block)
    : TerminatorInst(Instruction::TermOps::Br, 1) {
  AppendOperand(cond);
  SetStoredSuccessor(0, true_block);
  SetStoredSuccessor(1, false_block);
}

void BranchInst::SetTrueBlock(const BlockPtr &value) {
  DBG_ASSERT(value != nullptr, "true block is not a basic block");
  SetStoredSuccessor(0, value);
}

void BranchInst::SetFalseBlock(const BlockPtr &value) {
  DBG_ASSERT(value != nullptr, "false block is not a basic block");
  SetStoredSuccessor(1, value);
}

CallInst::CallInst(const FuncPtr &callee, const std::vector<SSAPtr> &args)
    : Instruction(Instruction::OtherOps::Call, args.size(),
                  ClassId::CallInstId),
      _callee(callee), _is_tail_call(false) {
  for (const auto &it : args)
    AppendOperand(it);
}

void CallInst::AddParam(const SSAPtr &param) {
  this->SetOperandNum(size() + 1);
  this->AppendOperand(param);
}

ICmpInst::ICmpInst(Operator op, const SSAPtr &lhs, const SSAPtr &rhs)
    : Instruction(Instruction::OtherOps::ICmp, 2, ClassId::ICmpInstId),
      _op(op) {
  AppendOperand(lhs);
  AppendOperand(rhs);
}

SSAPtr ICmpInst::EvalArithOnConst() {
  bool value;
  auto lhs_imm = dyn_cast<ConstantInt>(LHS())->value();
  auto rhs_imm = dyn_cast<ConstantInt>(RHS())->value();
  switch (_op) {
  case Operator::Equal:
    value = (lhs_imm == rhs_imm);
    break;
  case Operator::NotEqual:
    value = (lhs_imm != rhs_imm);
    break;
  case Operator::SLess:
    value = (lhs_imm < rhs_imm);
    break;
  case Operator::SLessEq:
    value = (lhs_imm <= rhs_imm);
    break;
  case Operator::SGreat:
    value = (lhs_imm > rhs_imm);
    break;
  case Operator::SGreatEq:
    value = (lhs_imm >= rhs_imm);
    break;
  default:
    ERROR("should not reach here");
  }
  auto const_bool = std::make_shared<ConstantInt>(value);
  const_bool->set_type(define::MakePrimType(define::Type::Bool, true));
  DBG_ASSERT(const_bool != nullptr, "create const bool value failed");
  return const_bool;
}

AccessInst::AccessInst(AccessType acc_type, const SSAPtr &ptr,
                       const SSAPtrList &indexs)
    : Instruction(Instruction::MemoryOps::Access, 0, ClassId::AccessInstId),
      _acc_type(acc_type) {
  AppendOperand(ptr);
  DBG_ASSERT(indexs.size() <= 2, "index and multiplier out of range");
  for (const auto &it : indexs) {
    AppendOperand(it);
  }
}

std::string ICmpInst::opStr() const {
  std::string op;
  switch (_op) {
  case Operator::Equal:
    op = "eq";
    break;
  case Operator::NotEqual:
    op = "ne";
    break;
  case Operator::SLess:
    op = "slt";
    break;
  case Operator::ULess:
    op = "ult";
    break;
  case Operator::SLessEq:
    op = "sle";
    break;
  case Operator::ULessEq:
    op = "ule";
    break;
  case Operator::SGreat:
    op = "sgt";
    break;
  case Operator::UGreat:
    op = "ugt";
    break;
  case Operator::SGreatEq:
    op = "sge";
    break;
  case Operator::UGreatEq:
    op = "uge";
    break;
  default:
    DBG_ASSERT(0, "compare op is error");
  }

  return op;
}

/* ---------------------------- Methods of Constant Value
 * ------------------------------- */

SSAPtr GetZeroValue(define::Type type) {
  using define::Type;
  auto zero = std::make_shared<ConstantInt>(0);
  switch (type) {
  case Type::Void:
    zero->set_type(define::MakeVoid());
    break;
  case Type::Int8:
    zero->set_type(define::MakeConst(Type::Int8));
    break;
  case Type::UInt8:
    zero->set_type(define::MakeConst(Type::UInt8, true));
    break;
  case Type::Int32:
    zero->set_type(define::MakeConst(Type::Int32, true));
    break;
  case Type::UInt32:
    zero->set_type(define::MakeConst(Type::UInt32, true));
    break;
  case Type::Bool:
    zero->set_type(define::MakeConst(Type::Bool, true));
    break;
  default:
    DBG_ASSERT(0, "Get error zero type");
  }
  return zero;
}

SSAPtr GetAllOneValue(define::Type type) {
  using define::Type;
  auto allOne = std::make_shared<ConstantInt>(-1);
  switch (type) {
  case Type::Void:
    allOne->set_type(define::MakeVoid());
    break;
  case Type::Int8:
    allOne->set_type(define::MakePrimType(Type::Int8, true));
    break;
  case Type::UInt8:
    allOne->set_type(define::MakePrimType(Type::UInt8, true));
    break;
  case Type::Int32:
    allOne->set_type(define::MakePrimType(Type::Int32, true));
    break;
  case Type::UInt32:
    allOne->set_type(define::MakePrimType(Type::UInt32, true));
    break;
  default:
    DBG_ASSERT(0, "Get error all-one type");
  }
  return allOne;
}

bool IsCallInst(const SSAPtr &ptr) {
  if (ptr->classId() == ClassId::CallInstId) {
    return true;
  }
  return false;
}

bool IsBinaryOperator(const SSAPtr &ptr) {
  if (ptr->classId() == ClassId::BinaryOperatorId) {
    return true;
  }
  return false;
}

bool IsCmp(const SSAPtr &ptr) {
  if (ptr->classId() == ClassId::ICmpInstId) {
    return true;
  }
  return false;
}

std::vector<BlockPtr> PhiNode::blocks() const { return _incoming_blocks; }

void PhiNode::ResetIncomingBlocks(const std::vector<BlockPtr> &blocks) {
  _incoming_blocks = blocks;
  ClearOperands();
  SetOperandNum(_incoming_blocks.size());
  ReserveOperands();
}

void PhiNode::addIncoming(const BlockPtr &pred, const SSAPtr &value) {
  _incoming_blocks.push_back(pred);
  AppendOperand(value);
}

BlockPtr PhiNode::getIncomingBlock(const Use &val) const {
  DBG_ASSERT(val.getUser() == this, "val is not PHI's use");
  unsigned idx = 0;
  for (const auto &it : (*this)) {
    if (&it != &val)
      idx++;
    else
      break;
  }
  DBG_ASSERT(idx < size(), "PHI index out of bound");
  return blocks()[idx];
}

int PhiNode::incomingIndexOf(const BasicBlock *pred) const {
  for (std::size_t i = 0; i < _incoming_blocks.size(); ++i) {
    if (_incoming_blocks[i].get() == pred)
      return static_cast<int>(i);
  }
  return -1;
}

SSAPtr PhiNode::getIncomingValueAt(unsigned int i) const {
  DBG_ASSERT(i < size(), "PHI index out of bound");
  return GetOperand(i);
}

SSAPtr PhiNode::getIncomingValue(const BasicBlock *pred) const {
  auto idx = incomingIndexOf(pred);
  DBG_ASSERT(idx >= 0, "incoming predecessor not found");
  return getIncomingValueAt(static_cast<unsigned>(idx));
}

void PhiNode::setIncomingValueAt(unsigned int i, const SSAPtr &value) {
  DBG_ASSERT(i < size(), "PHI index out of bound");
  SetOperand(i, value);
}

void PhiNode::setIncomingValue(const BasicBlock *pred, const SSAPtr &value) {
  auto idx = incomingIndexOf(pred);
  DBG_ASSERT(idx >= 0, "incoming predecessor not found");
  setIncomingValueAt(static_cast<unsigned>(idx), value);
}

void PhiNode::replaceIncomingBlock(const BasicBlock *oldPred,
                                   const BlockPtr   &newPred) {
  for (auto &pred : _incoming_blocks) {
    if (pred.get() == oldPred)
      pred = newPred;
  }
}

void PhiNode::removeIncoming(const BasicBlock *pred) {
  auto idx = incomingIndexOf(pred);
  if (idx < 0)
    return;
  _incoming_blocks.erase(_incoming_blocks.begin() + idx);
  RemoveOperand(static_cast<unsigned>(idx));
}

/* ---------------------------- Methods of dumping IR
 * ------------------------------- */

const char *xIndent = "  ";

// indicate if is in expression
int in_expr = 0;

xstl::Guard InExpr() {
  ++in_expr;
  return xstl::Guard([] { --in_expr; });
}

// in_branch
int in_branch = 0;

xstl::Guard InBranch() {
  ++in_branch;
  return xstl::Guard([] { --in_branch; });
}

void DumpType(std::ostream &os, const define::TypePtr &type) {
  if (type->IsPointer()) {
    os << "ptr";
    return;
  }
  os << type->GetTypeId();
}

void DumpValue(std::ostream &os, IdManager &id_mgr, const SSAPtr &value) {
  if (value == nullptr)
    os << "nullptr";
  else
    value->Dump(os, id_mgr);
}

void DumpValue(std::ostream &os, IdManager &id_mgr, const Use &operand) {
  DumpValue(os, id_mgr, operand.value());
}

template <typename It>
inline void DumpValue(std::ostream &os, IdManager &id_mgr, It begin, It end) {
  for (auto it = begin; it != end; ++it) {
    if (it != begin)
      os << ", ";
    DumpValue(os, id_mgr, *it);
  }
}

void DumpBlockName(std::ostream &os, IdManager &id_mgr,
                   const BasicBlock *block) {
  auto &npos = std::string::npos;
  auto  name = block->name();
  if (name.find("if.cond") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_IF_COND);
  } else if (name.find("if.then") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_THEN);
  } else if (name.find("if.else") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_ELSE);
  } else if (name.find("if.end") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_IF_END);
  } else if (name.find("while.cond") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_WHILE_COND);
  } else if (name.find("loop.body") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_LOOP_BODY);
  } else if (name.find("while.end") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_WHILE_END);
  } else if (name.find("lhs.true") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_LHS_TRUE);
  } else if (name.find("lhs.false") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_LHS_FALSE);
  } else if (name.find("land.end") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_LAND_END);
  } else if (name.find("lor.end") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_LOR_END);
  } else if (name.find("block") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_BLOCK);
  } else {
    os << name;
  }
  //  os << "size:" << const_cast<BasicBlock *>(block)->insts().size();
}

void PrintId(std::ostream &os, IdManager &id_mgr, const Value *value) {
  os << "%t" << id_mgr.GetId(value);
}

void PrintId(std::ostream &os, IdManager &id_mgr, const std::string &name) {
  os << "%" << name;
}

inline void DumpWithType(std::ostream &os, IdManager &id_mgr,
                         const SSAPtr &val) {
  DumpType(os, val->type());
  os << ' ';
  DumpValue(os, id_mgr, val);
}

// print indent, id and assign
// return true if in expression
inline bool PrintPrefix(std::ostream &os, IdManager &id_mgr, const Value *val) {
  if (!in_expr)
    os << xIndent;
  PrintId(os, id_mgr, val);
  if (!in_expr)
    os << " = ";
  return in_expr;
}

inline bool PrintPrefix(std::ostream &os, IdManager &id_mgr,
                        const std::string &name) {
  if (!in_expr)
    os << xIndent;
  PrintId(os, id_mgr, name);
  if (!in_expr)
    os << " = ";
  return in_expr;
}

void BinaryOperator::Dump(std::ostream &os, IdManager &id_mgr) const {
  if (PrintPrefix(os, id_mgr, this))
    return;

  auto guard = InExpr();

  os << this->GetOpcodeAsString() << " ";
  DumpType(os, type());
  os << " ";
  DumpValue(os, id_mgr, begin(), end());
}

void BasicBlock::Dump(std::ostream &os, IdManager &id_mgr,
                      const std::string &separator) const {
  bool dump_cfg = false;
  if (!separator.empty())
    dump_cfg = true;

  if (!dump_cfg) {
    if (!_name.empty()) {
      if (in_branch)
        os << "%"; // add '%' if in branch instructions
      DumpBlockName(os, id_mgr, this);
    } else {
      PrintId(os, id_mgr, this);
    }
    if (in_expr)
      return;

    os << ":";

    // dump predecessors
    if (!_predecessors.empty()) {
      auto guard = InExpr();
      os << " ; preds: ";
      DumpValue(os, id_mgr, _predecessors.begin(), _predecessors.end());
    }
  }

  os << std::endl;
  // dump each statements
  for (const auto &it : _insts) {
    DumpValue(os, id_mgr, it);
    if (dump_cfg)
      os << separator;
    else
      os << std::endl;
  }
}

void BasicBlock::Dump(std::ostream &os, IdManager &id_mgr) const {
  Dump(os, id_mgr, "");
}

void Function::Dump(std::ostream &os, IdManager &id_mgr) const {
  //  if (define::IsBuiltinFunction(_function_name)) return;
  id_mgr.Reset();
  id_mgr.RecordName(this, _function_name);
  if (_is_decl) {
    os << "declare ";
  } else {
    os << "define ";
  }

  // dump ret type
  auto func_type = type();
  auto args_type = func_type->GetArgsType().value();
  DumpType(os, func_type->GetReturnType(args_type));

  // dump function name
  os << " @" << _function_name;

  // dump args
  os << "(";
  if (!args_type.empty()) {
    for (std::size_t i = 0; i < args_type.size(); i++) {
      if (args_type[i]->IsPointer() || args_type[i]->IsArray()) {
        os << "ptr";
      } else {
        DumpType(os, args_type[i]);
      }

      if (!_is_decl) {
        os << " "; // span between type and name
        _args[i]->Dump(os, id_mgr);
      }

      // separate each parameters
      if (i != args_type.size() - 1)
        os << ", ";
    }
  }
  os << ")";

  if (_is_decl) {
    os << "\n" << std::endl;
    return;
  }

  os << " {\n";

  std::vector<BlockPtr>       bbs;
  std::unordered_set<Value *> visited;
  std::queue<BlockPtr>        worklist;
  worklist.push(entry());
  BlockPtr func_exit = nullptr;
  while (!worklist.empty()) {
    auto it = worklist.front();
    worklist.pop();
    auto block = it;
    if (!block || block->insts().empty()) {
      bbs.push_back(block);
      continue;
    }
    if (block->name() == "func_exit" ||
        IsSSA<ReturnInst>(block->insts().back())) {
      func_exit = it;
      bbs.push_back(nullptr);
    } else {
      bbs.push_back(block);
    }

    auto back = block->insts().back();
    if (auto jump_inst = dyn_cast<JumpInst>(back)) {
      auto target = jump_inst->target();
      if (target && !visited.count(target.get())) {
        visited.insert(target.get());
        worklist.push(target);
      }
    } else if (auto branch_inst = dyn_cast<BranchInst>(back)) {
      auto true_block = branch_inst->true_block();
      if (true_block && !visited.count(true_block.get())) {
        visited.insert(true_block.get());
        worklist.push(true_block);
      }
      auto false_block = branch_inst->false_block();
      if (false_block && !visited.count(false_block.get())) {
        visited.insert(false_block.get());
        worklist.push(false_block);
      }
    } else if (auto ret_inst = dyn_cast<ReturnInst>(back)) {
      // do nothing
    } else {
      ERROR("last instruction of current block is not terminate instruction");
    }
  }
  DBG_ASSERT(bbs.size() == this->size(), "block size is error");
  for (auto &bb : bbs) {
    if (bb == nullptr)
      continue;
    DumpValue(os, id_mgr, bb);
    os << std::endl;
  }

  if (func_exit) {
    DumpValue(os, id_mgr, func_exit);
  }

  //   end of function
  os << "}\n" << std::endl;
}

void JumpInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  auto eguard = InExpr();
  auto bguard = InBranch();
  os << xIndent << "br label ";
  DumpValue(os, id_mgr, target());
}

void ReturnInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  auto guard = InExpr();
  os << xIndent << "ret ";
  if (!RetVal())
    os << "void";
  else
    DumpWithType(os, id_mgr, RetVal());
}

void BranchInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  auto eguard = InExpr();
  auto bguard = InBranch();
  os << xIndent << "br ";
  DumpWithType(os, id_mgr, cond());
  os << ", label ";
  DumpValue(os, id_mgr, true_block());
  os << ", label ";
  DumpValue(os, id_mgr, false_block());
}

void StoreInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  auto guard = InExpr();
  os << xIndent << "store ";
  DumpWithType(os, id_mgr, data());
  os << ", ";
  DumpWithType(os, id_mgr, pointer());
}

void AllocaInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  bool ret;
  if (_name.empty()) {
    ret = PrintPrefix(os, id_mgr, this);
  } else {
    ret = PrintPrefix(os, id_mgr, _name);
  }

  if (ret)
    return;

  auto guard = InExpr();
  os << "alloca ";
  DumpType(os, type()->GetDerefedType());
}

void LoadInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  bool ret;
  if (_name.empty()) {
    ret = PrintPrefix(os, id_mgr, this);
  } else {
    ret = PrintPrefix(os, id_mgr, _name);
  }

  if (ret)
    return;

  auto guard = InExpr();
  os << "load ";
  DumpType(os, type());
  os << ", ";
  DumpWithType(os, id_mgr, Pointer());
}

void ArgRefSSA::Dump(std::ostream &os, IdManager &id_mgr) const {
  os << "%" << _arg_name;
}

void ConstantInt::Dump(std::ostream &os, IdManager &id_mgr) const {
  os << _value;
}

void ConstantString::Dump(std::ostream &os, IdManager &id_mgr) const {
  os << _str;
}

void ConstantArray::Dump(std::ostream &os, IdManager &id_mgr) const {
  Dump(os, id_mgr, "= global");
}

void ConstantArray::Dump(std::ostream &os, IdManager &id_mgr,
                         const std::string &separator) const {
  if (!_name.empty()) {
    os << _name;
    if (in_expr)
      return;
  }

  os << separator;
  DumpType(os, type()->GetDerefedType());
  os << " [";
  for (std::size_t i = 0; i < this->size(); i++) {
    DumpWithType(os, id_mgr, (*this)[i].value());
    if (i != this->size() - 1)
      os << ", ";
  }
  os << "]" << std::endl;
}

void CallInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  bool is_void = type()->IsVoid();

  // do not print name if its return type is void
  if (!is_void && PrintPrefix(os, id_mgr, this))
    return;

  auto guard = InExpr();

  if (is_void)
    os << xIndent; // print indent if return void

  os << "call ";
  // dump return type
  DumpType(os, type());
  os << " @";

  // dump callee name
  auto name = Callee()->GetFunctionName();
  os << name << "(";

  for (std::size_t i = 0; i < size(); i++) {
    auto arg = (*this)[i].value();
    DumpWithType(os, id_mgr, arg);
    if (i != size() - 1)
      os << ", ";
  }

  os << ")";
}

void ICmpInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  if (PrintPrefix(os, id_mgr, this))
    return;
  auto guard = InExpr();
  os << "icmp " << opStr() << " ";
  DumpType(os, LHS()->type());
  os << " ";
  DumpValue(os, id_mgr, begin(), end());
}

void CastInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  if (PrintPrefix(os, id_mgr, this))
    return;
  auto guard = InExpr();

  switch (this->opcode()) {
  case CastOps::Trunc: {
    os << "trunc ";
    break;
  }
  case CastOps::ZExt: {
    os << "zext ";
  }
  default: {
  }
  }
  DumpWithType(os, id_mgr, operand());
  os << " to ";
  DumpType(os, type());
}

void GlobalVariable::Dump(std::ostream &os, IdManager &id_mgr) const {
  os << "@" << _name;
  if (in_expr)
    return;
  auto &init_val                     = init();
  bool  dumped_with_trailing_newline = false;

  if (init_val) {
    auto init_type = init_val->type();
    /* Dump array */
    if (init_type->GetDerefedType() && init_type->GetDerefedType()->IsArray()) {
      init_val->Dump(os, id_mgr);
      dumped_with_trailing_newline = true;
    } else {
      /* Dump global variable */
      os << " = global ";
      DumpWithType(os, id_mgr, init_val);
    }
  } else {
    os << " = global ";
    DumpType(os, type()->GetDerefedType());
    os << " " << "zeroinitializer";
  }

  if (!dumped_with_trailing_newline) {
    os << std::endl;
  }
}

void AccessInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  if (PrintPrefix(os, id_mgr, this))
    return;
  auto guard = InExpr();
  os << "getelementptr inbounds ";
  auto ptr_ssa  = ptr();
  auto ptr_type = ptr_ssa->type();
  DumpType(os, ptr_type->GetDerefedType());
  os << ", ";
  DumpWithType(os, id_mgr, ptr());
  os << ", ";

  // dump index
  std::size_t index_len = this->size();
  for (std::size_t i = 1; i < index_len; i++) {
    DumpWithType(os, id_mgr, index(i));
    if (i != index_len - 1)
      os << ", ";
  }
}

void UnDefineValue::Dump(std::ostream &os, IdManager &id_mgr) const {
  os << "undef";
}

void PhiNode::Dump(std::ostream &os, IdManager &id_mgr) const {
  if (!in_expr)
    os << xIndent;
  os << "%phi" << id_mgr.GetId(this, IdType::_ID_PHI);
  if (!in_expr)
    os << " = ";
  if (in_expr)
    return;

  auto guard = InExpr();
  os << "phi ";
  DumpType(os, type());
  for (std::size_t i = 0; i < this->size(); i++) {
    os << " [ ";
    DumpValue(os, id_mgr, (*this)[i].value());
    os << ", %";
    getIncomingBlock(static_cast<unsigned>(i))->Dump(os, id_mgr);
    os << " ]";
    if (i != this->size() - 1)
      os << ",";
  }
}

} // namespace lava::mid
