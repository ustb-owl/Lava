#include <define/ast.h>
#include "ssa.h"
#include "castssa.h"
#include "constant.h"
#include "idmanager.h"
#include "lib/guard.h"
#include "define/type.h"

namespace lava::mid {

Instruction::Instruction(unsigned opcode, unsigned operand_nums, const SSAPtr& insertBefore)
    : User(operand_nums), _opcode(opcode) {
  if (insertBefore != nullptr) {
    DBG_ASSERT(insertBefore->GetParent(), "InsertBefore do not contain a parent(BasicBlock)");
    insertBefore->GetParent()->AddInstBefore(insertBefore, SSAPtr(this));
  }
}

Instruction::Instruction(unsigned opcode, unsigned operand_nums,
   const Operands& operands, const SSAPtr& insertBefore)
   : User(operand_nums, operands), _opcode(opcode) {
  if (insertBefore != nullptr) {
    DBG_ASSERT(insertBefore->GetParent(), "InsertBefore do not contain a parent(BasicBlock)");
    auto insts = insertBefore->GetParent()->insts();
    auto it = std::find(insts.begin(), insts.end(), insertBefore);
    insts.insert(it, SSAPtr(this));
  }
}

Instruction::Instruction(unsigned opcode, unsigned operand_nums, const BlockPtr &insertAtEnd)
    : User(operand_nums), _opcode(opcode) {
  DBG_ASSERT(insertAtEnd != nullptr, "Basic block to append to may not be NULL");
  insertAtEnd->AddInstToEnd(SSAPtr(this));
}

Instruction::Instruction(unsigned opcode, unsigned operand_nums,
            const Operands& operands, const BlockPtr &insertAtEnd)
    : User(operand_nums, operands), _opcode(opcode) {
  DBG_ASSERT(insertAtEnd != nullptr, "Basic block to append to may not be NULL");
  insertAtEnd->AddInstToEnd(SSAPtr(this));
}


std::string Instruction::GetOpcodeAsString(unsigned int opcode) {
    switch (opcode) {
      // Terminators
      case Br:             return "br";
      case Ret:            return "ret";
      case Jmp:            return "br";

        // Standard binary operators...
      case Add:            return "add";
      case Sub:            return "sub";
      case Mul:            return "mul";
      case UDiv:           return "udiv";
      case SDiv:           return "sdiv";
//      case FDiv: return "fdiv";
      case URem:           return "urem";
      case SRem:           return "srem";
//      case FRem: return "frem";

        // Logical operators...
      case And:            return "and";
      case Or :            return "or";
      case Xor:            return "xor";

        // Memory instructions...
      case Malloc:         return "malloc";
      case Free:           return "free";
      case Alloca:         return "alloca";
      case Load:           return "load";
      case Store:          return "store";
//      case GetElementPtr: return "getelementptr";

        // Convert instructions...
      case Trunc:          return "trunc";
      case ZExt:           return "zext";
      case SExt:           return "sext";
#if 0
      case FPTrunc:   return "fptrunc";
      case FPExt:     return "fpext";
      case FPToUI:    return "fptoui";
      case FPToSI:    return "fptosi";
      case UIToFP:    return "uitofp";
      case SIToFP:    return "sitofp";
#endif
      case IntToPtr:       return "inttoptr";
      case PtrToInt:       return "ptrtoint";
      case BitCast:        return "bitcast";

        // Other instructions...
      case ICmp:           return "icmp";
//      case FCmp:           return "fcmp";
      case PHI:            return "phi";
      case Select:         return "select";
      case Call:           return "call";
      case Shl:            return "shl";
      case LShr:           return "lshr";
      case AShr:           return "ashr";
      case VAArg:          return "va_arg";
      case ExtractElement: return "extractelement";
      case InsertElement:  return "insertelement";
      case ShuffleVector:  return "shufflevector";

      default: return "<Invalid operator> ";
    }
  return "";
}

//===----------------------------------------------------------------------===//
//                           BinaryOperator Class
//===----------------------------------------------------------------------===//

BinaryOperator::BinaryOperator(Instruction::BinaryOps opcode, const SSAPtr &S1,
   const SSAPtr &S2, const define::TypePtr &type, const SSAPtr &IB)
   : Instruction(opcode, 2, IB) {
  AddValue(S1);
  AddValue(S2);
}

BinaryOperator::BinaryOperator(Instruction::BinaryOps opcode, const SSAPtr &S1,
   const SSAPtr &S2, const define::TypePtr &type, const BlockPtr &IAE)
   : Instruction(opcode, 2, IAE){
  AddValue(S1);
  AddValue(S2);
}

BinaryPtr
BinaryOperator::Create(Instruction::BinaryOps opcode, const SSAPtr &S1,
     const SSAPtr &S2, const SSAPtr &IB) {
  auto s1_type = S1->type();
  auto s2_type = S2->type();
  DBG_ASSERT(s1_type->IsPrime(), "S1 is not prime type");
  DBG_ASSERT(s2_type->IsPrime(), "S2 is not prime type");
  DBG_ASSERT(s1_type->IsInteger(), "binary operator can only being performed on int");

  // TODO: add necessary cast here
//  DBG_ASSERT(s1_type == s2_type, "S1 has different type with S2");
#if 0
  if (s1_type->IsNotShortThan(s2_type))
    return std::make_shared<BinaryOperator>(opcode, S1, S2, s1_type, IB);
  else
    return std::make_shared<BinaryOperator>(opcode, S1, S2, s2_type, IB);
#endif
  return nullptr;
}

BinaryPtr
BinaryOperator::Create(Instruction::BinaryOps opcode, const SSAPtr &S1,
     const SSAPtr &S2, const BlockPtr &IAE) {
  auto s1_type = S1->type();
  auto s2_type = S2->type();
  DBG_ASSERT(s1_type->IsPrime(), "S1 is not prime type");
  DBG_ASSERT(s2_type->IsPrime(), "S2 is not prime type");
  DBG_ASSERT(s1_type->IsInteger(), "binary operator can only being performed on int");

  DBG_ASSERT(s1_type == s2_type, "S1 has different type with S2");

#if 0
  if (s1_type->IsNotShortThan(s2_type))
    return std::make_shared<BinaryOperator>(opcode, S1, S2, s1_type, IAE);
  else
    return std::make_shared<BinaryOperator>(opcode, S1, S2, s2_type, IAE);
#endif
  return nullptr;
}

BinaryPtr BinaryOperator::createNeg(const SSAPtr &Op, const SSAPtr &InsertBefore) {
  auto typeInfo = Op->type();
  DBG_ASSERT(typeInfo->IsInteger(), "Neg operator is not integer");
  auto zero = GetZeroValue(typeInfo->GetType());
  return std::make_shared<BinaryOperator>(Instruction::Sub, zero, Op,
                                          typeInfo, InsertBefore);
}

BinaryPtr BinaryOperator::createNeg(const SSAPtr &Op, const BlockPtr &InsertAtEnd) {
  auto typeInfo = Op->type();
  DBG_ASSERT(typeInfo->IsInteger(), "Neg operator is not integer");
  auto zero = GetZeroValue(typeInfo->GetType());
  return std::make_shared<BinaryOperator>(Instruction::Sub, zero, Op,
                                          typeInfo, InsertAtEnd);
}

BinaryPtr BinaryOperator::createNot(const SSAPtr &Op, const SSAPtr &InsertBefore) {
  auto typeInfo = Op->type();
  DBG_ASSERT(typeInfo->IsInteger(), "Not operator is not integer");
  auto C = GetAllOneValue(typeInfo->GetType());
  return std::make_shared<BinaryOperator>(Instruction::Xor, Op, C,
                                          typeInfo, InsertBefore);
}

BinaryPtr BinaryOperator::createNot(const SSAPtr &Op, const BlockPtr &InsertAtEnd) {
  auto typeInfo = Op->type();
  DBG_ASSERT(typeInfo->IsInteger(), "Not operator is not integer");
  auto C = GetAllOneValue(typeInfo->GetType());
  return std::make_shared<BinaryOperator>(Instruction::Sub, Op, C,
                                          typeInfo, InsertAtEnd);
}

void BasicBlock::AddInstBefore(const SSAPtr &insertBefore, const SSAPtr &inst) {
  auto it = std::find(_insts.begin(), _insts.end(), insertBefore);
  DBG_ASSERT(it != _insts.end(), "Basic block don't has this instruction");
  _insts.insert(it, inst);
}

BranchInst::BranchInst(const SSAPtr &cond, const SSAPtr &true_block,
                       const SSAPtr &false_block, const SSAPtr &IB)
  : TerminatorInst(Instruction::TermOps::Br, 3, IB) {
  AddValue(cond);
  AddValue(true_block);
  AddValue(false_block);
}

CallInst::CallInst(const SSAPtr &callee, const std::vector<SSAPtr> &args, const SSAPtr &IB) :
    Instruction(Instruction::OtherOps::Call, args.size() + 1, IB) {
  AddValue(callee);
  for (const auto &it : args) AddValue(it);
}

ICmpInst::ICmpInst(Operator op, const SSAPtr &lhs, const SSAPtr &rhs, const SSAPtr &IB)
    : Instruction(Instruction::OtherOps::ICmp, 2, IB), _op(op) {
  AddValue(lhs);
  AddValue(rhs);
}

std::string ICmpInst::opStr() const {
  std::string  op;
  switch (_op) {

    case Operator::Equal:    op = "eq";  break;
    case Operator::NotEqual: op = "ne";  break;
    case Operator::SLess:    op = "slt"; break;
    case Operator::ULess:    op = "ult"; break;
    case Operator::SLessEq:  op = "sle"; break;
    case Operator::ULessEq:  op = "ule"; break;
    case Operator::SGreat:   op = "sgt"; break;
    case Operator::UGreat:   op = "ugt"; break;
    case Operator::SGreatEq: op = "sge"; break;
    case Operator::UGreatEq: op = "uge"; break;
    default: DBG_ASSERT(0, "compare op is error");
  }

  return op;
}

/* ---------------------------- Methods of Constant Value ------------------------------- */


SSAPtr GetZeroValue(define::Type type) {
  using define::Type;
  auto zero = std::make_shared<ConstantInt>(0);
  switch (type) {
    case Type::Void:
      zero->set_type(define::MakeVoid());
      break;
    case Type::Int8:
      zero->set_type(define::MakePrimType(Type::Int8, true));
      break;
    case Type::UInt8:
      zero->set_type(define::MakePrimType(Type::UInt8, true));
      break;
    case Type::Int32:
      zero->set_type(define::MakePrimType(Type::Int32, true));
      break;
    case Type::UInt32:
      zero->set_type(define::MakePrimType(Type::UInt32, true));
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
  if (ptr->isInstruction()) {
    auto inst = CastTo<Instruction>(ptr);
    if (inst->opcode() == Instruction::OtherOps::Call) {
      return true;
    }
  }
  return false;
}

bool IsBinaryOperator(const SSAPtr &ptr){
  if (ptr->isInstruction()) {
    auto inst = CastTo<Instruction>(ptr);
    if (inst->isBinaryOp() || inst->opcode() == Instruction::OtherOps::ICmp) {
      return true;
    }
  }
  return false;
}


/* ---------------------------- Methods of dumping IR ------------------------------- */

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
  os << type->GetTypeId();
}

void DumpValue(std::ostream &os, IdManager &id_mgr, const SSAPtr &value) {
  value->Dump(os, id_mgr);
}

void DumpValue(std::ostream &os, IdManager &id_mgr, const Use &operand) {
  DumpValue(os, id_mgr, operand.get());
}

template <typename It>
inline void DumpValue(std::ostream &os, IdManager &id_mgr, It begin, It end) {
  for (auto it = begin; it != end; ++it) {
    if (it != begin) os << ", ";
    DumpValue(os, id_mgr, *it);
  }
}

inline void DumpBlockName(std::ostream &os, IdManager &id_mgr, const BasicBlock *block) {
  auto &npos =  std::string::npos;
  auto name = block->name();
  if (name.find("if_cond") != npos) {
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
  } else if (name.find("block") != npos){
    os << name << id_mgr.GetId(block, IdType::_ID_BLOCK);
  } else {
    os << name;
  }
}

void PrintId(std::ostream &os, IdManager &id_mgr, const Value *value) {
  os << "%" << id_mgr.GetId(value);
}

void PrintId(std::ostream &os, IdManager &id_mgr, const std::string &name) {
  os << "%" << name;
}

inline void DumpWithType(std::ostream &os, IdManager &id_mgr, const SSAPtr &val) {
  DumpType(os, val->type());
  os << ' ';
  DumpValue(os, id_mgr, val);
}

// print indent, id and assign
// return true if in expression
inline bool PrintPrefix(std::ostream &os, IdManager &id_mgr, const Value *val) {
  if (!in_expr) os << xIndent;
  PrintId(os, id_mgr, val);
  if (!in_expr) os << " = ";
  return in_expr;
}

inline bool PrintPrefix(std::ostream &os, IdManager &id_mgr, const std::string &name) {
  if (!in_expr) os << xIndent;
  PrintId(os, id_mgr, name);
  if (!in_expr) os << " = ";
  return in_expr;
}

void BinaryOperator::Dump(std::ostream &os, IdManager &id_mgr) const {
  if (PrintPrefix(os, id_mgr, this)) return;

  auto guard = InExpr();

  os << this->GetOpcodeAsString() << " ";
  DumpType(os, type());
  os << " ";
  DumpValue(os, id_mgr, begin(), end());
  os << std::endl;
}

void BasicBlock::Dump(std::ostream &os, IdManager &id_mgr) const {
  if (!_name.empty()){
    if (in_branch) os << "%"; // add '%' if in branch instructions
    DumpBlockName(os, id_mgr, this);
  } else {
    PrintId(os, id_mgr, this);
  }
  if (in_expr) return;

  os << ":";

  // dump predecessors
  if (!empty()) {
    auto guard = InExpr();
    os << " ; preds: ";
    DumpValue(os, id_mgr, begin(), end());
  }
  os << std::endl;
  // dump each statements
  for (const auto &it : _insts) DumpValue(os, id_mgr, it);
}

void Function::Dump(std::ostream &os, IdManager &id_mgr) const {
  id_mgr.Reset();
  id_mgr.RecordName(this, _function_name);
  os << "define " ;

  // dump ret type
  auto func_type = type();
  DumpType(os, func_type->GetReturnType(func_type->GetArgsType().value()));

  // dump function name
  os << " @" << _function_name;

  // dump args
  os << "(";
  if (!_args.empty()) {
    auto args_type = func_type->GetArgsType();
    auto args_typevalues = args_type.value();
    for (std::size_t i = 0; i < _args.size(); i++) {
      DumpType(os, args_typevalues[i]);
      os << " ";  // span between type and name
      _args[i]->Dump(os, id_mgr);

      // separate each parameters
      if (i != _args.size() - 1) os << ", ";
    }
  }
  os << ") {\n";

  // dump content of blocks
  bool report_exit = false;
  for (std::size_t i = 0; i < this->size(); i++) {

    // dump function exit later
    if (auto block = CastTo<BasicBlock>((*this)[i].get())) {
      if (block->name() == "func_exit") {
        report_exit = true;
        continue;
      }
    }

    DumpValue(os, id_mgr, (*this)[i]);
    // end of block
    os << "\n";
  }

  if (report_exit) {
    DumpValue(os, id_mgr, (*this)[1]);
    os << std::endl;
  }

//   end of function
  os << "}\n" << std::endl;
}

void JumpInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  auto eguard = InExpr();
  auto bguard = InBranch();
  os << xIndent << "br label ";
  DumpValue(os, id_mgr, target());
  os << std::endl;
}

void ReturnInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  auto guard = InExpr();
  os << xIndent << "ret ";
  if (!RetVal()) os << "void";
  else DumpWithType(os, id_mgr, RetVal());
//  os << std::endl;
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
  os << std::endl;
}

void StoreInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  auto guard = InExpr();
  os << xIndent << "store ";
  DumpWithType(os, id_mgr, value());
  os << ", ";
  DumpWithType(os, id_mgr, pointer());
  os << std::endl;
}

void AllocaInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  bool ret;
  if (_name.empty()) {
    ret = PrintPrefix(os, id_mgr, this);
  } else {
    ret = PrintPrefix(os, id_mgr, _name);
  }

  if (ret) return;

  auto guard = InExpr();
  os << "alloca ";
  DumpType(os, type()->GetDerefedType());
  os << std::endl;
}

void LoadInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  if (PrintPrefix(os, id_mgr, this)) return;
  auto guard = InExpr();
  os << "load ";
  DumpType(os, type());
  os << ", ";
  DumpWithType(os, id_mgr, Pointer());
  os << std::endl;
}

void ArgRefSSA::Dump(std::ostream &os, IdManager &id_mgr) const {
  os << "%" << _arg_name;
}

void ConstantInt::Dump(std::ostream &os, IdManager &id_mgr) const {
  os << _value;
}


void ConstantString::Dump(std::ostream &os, IdManager &id_mgr) const {

}

void ConstantArray::Dump(std::ostream &os, IdManager &id_mgr) const {
}

void CallInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  if (PrintPrefix(os, id_mgr, this)) return;
  auto guard = InExpr();
  os << "call ";
  // dump return type
  DumpType(os, type());
  os << " @";

  // dump callee name
  auto callee = Callee();
  auto name = std::static_pointer_cast<Function>(callee)->GetFunctionName();
  os << name << "(";

  for (std::size_t i = 1; i < size(); i++) {
    auto arg = (*this)[i].get();
    DumpWithType(os, id_mgr, arg);
    if (i != size() - 1) os << ", ";
  }

  os << ")" << std::endl;
}

void ICmpInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  if (PrintPrefix(os, id_mgr, this)) return;
  auto guard = InExpr();
  os << "icmp " << opStr() << " ";
  DumpType(os, LHS()->type());
  os << " ";
  DumpValue(os, id_mgr, begin(), end());
  os << std::endl;
}

void CastInst::Dump(std::ostream &os, IdManager &id_mgr) const {
}

void GlobalVariable::Dump(std::ostream &os, IdManager &id_mgr) const {
}

}