#include <define/ast.h>
#include "ssa.h"
#include "common/casting.h"
#include "constant.h"
#include "common/idmanager.h"
#include "lib/guard.h"
#include "define/type.h"

namespace lava::mid {

Instruction::Instruction(unsigned opcode, unsigned operand_nums, ClassId classId)
    : User(classId, operand_nums), _opcode(opcode) {}

Instruction::Instruction(unsigned opcode, unsigned operand_nums,
                         const Operands &operands, ClassId classId)
    : User(classId, operand_nums, operands), _opcode(opcode) {}

bool Instruction::classof(Value *value) {
  switch (value->classId()) {
    case ClassId::PHINodeId:
    case ClassId::UndefId:
    case ClassId::FunctionId:
    case ClassId::BasicBlockId:
    case ClassId::ArgRefSSAId:
    case ClassId::GlobalVariableId:
    case ClassId::ConstantIntId:
    case ClassId::ConstantArrayId:
    case ClassId::ConstantStringId: return false;
    default: return true;
  }
}

bool Instruction::classof(const Value *value) {
  switch (value->classId()) {
    case ClassId::PHINodeId:
    case ClassId::UndefId:
    case ClassId::FunctionId:
    case ClassId::BasicBlockId:
    case ClassId::ArgRefSSAId:
    case ClassId::GlobalVariableId:
    case ClassId::ConstantIntId:
    case ClassId::ConstantArrayId:
    case ClassId::ConstantStringId: return false;
    default: return true;
  }
}

std::string Instruction::GetOpcodeAsString(unsigned int opcode) {
  switch (opcode) {
    // Terminators
    case Br:              return "br";
    case Ret:             return "ret";
    case Jmp:             return "br";

      // Standard binary operators...
    case Add:             return "add";
    case Sub:             return "sub";
    case Mul:             return "mul";
    case UDiv:            return "udiv";
    case SDiv:            return "sdiv";
//      case FDiv: return "fdiv";
    case URem:            return "urem";
    case SRem:            return "srem";
//      case FRem: return "frem";

      // Logical operators...
    case And:             return "and";
    case Or :             return "or";
    case Xor:             return "xor";

      // Memory instructions...
    case Malloc:          return "malloc";
    case Free:            return "free";
    case Alloca:          return "alloca";
    case Load:            return "load";
    case Store:           return "store";
//      case GetElementPtr: return "getelementptr";

      // Convert instructions...
    case Trunc:           return "trunc";
    case ZExt:            return "zext";
    case SExt:            return "sext";
#if 0
      case FPTrunc:   return "fptrunc";
      case FPExt:     return "fpext";
      case FPToUI:    return "fptoui";
      case FPToSI:    return "fptosi";
      case UIToFP:    return "uitofp";
      case SIToFP:    return "sitofp";
#endif
    case IntToPtr:        return "inttoptr";
    case PtrToInt:        return "ptrtoint";
    case BitCast:         return "bitcast";

      // Other instructions...
    case ICmp:            return "icmp";
//      case FCmp:           return "fcmp";
    case PHI:             return "phi";
    case Select:          return "select";
    case Call:            return "call";
    case Shl:             return "shl";
    case LShr:            return "lshr";
    case AShr:            return "ashr";
    case VAArg:           return "va_arg";
    case ExtractElement:  return "extractelement";
    case InsertElement:   return "insertelement";
    case ShuffleVector:   return "shufflevector";

    default:              return "<Invalid operator> ";
  }
  return "";
}

bool TerminatorInst::classof(Value *value) {
  switch (value->classId()) {
    case ClassId::ReturnInstId:
    case ClassId::BranchInstId:
    case ClassId::JumpInstId: return true;
    default: return false;
  }
}

bool TerminatorInst::classof(const Value *value) {
  switch (value->classId()) {
    case ClassId::ReturnInstId:
    case ClassId::BranchInstId:
    case ClassId::JumpInstId: return true;
    default: return false;
  }
}

//===----------------------------------------------------------------------===//
//                           BinaryOperator Class
//===----------------------------------------------------------------------===//

BinaryOperator::BinaryOperator(Instruction::BinaryOps opcode, const SSAPtr &S1,
                               const SSAPtr &S2, const define::TypePtr &type)
    : Instruction(opcode, 2, ClassId::BinaryOperatorId) {
  AddValue(S1);
  AddValue(S2);
}

BinaryPtr
BinaryOperator::Create(Instruction::BinaryOps opcode, const SSAPtr &S1, const SSAPtr &S2) {
  auto s1_type = S1->type();
  auto s2_type = S2->type();
  DBG_ASSERT(s1_type->IsPrime(), "S1 is not prime type");
  DBG_ASSERT(s1_type->IsInteger(), "binary operator can only being performed on int");
  DBG_ASSERT(s1_type->GetSize() == s2_type->GetSize(), "S1 has different type with S2");

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
  if (value->classId() == ClassId::BinaryOperatorId) return true;
  return false;
}

bool BinaryOperator::classof(const Value *value) {
  if (value->classId() == ClassId::BinaryOperatorId) return true;
  return false;
}

std::vector<BasicBlock *> BasicBlock::successors() {
  std::vector<BasicBlock *> succes;
  auto inst = insts().back();
  if (auto jumpInst = dyn_cast<JumpInst>(inst)) {
    succes.push_back(dyn_cast<BasicBlock>(jumpInst->target()).get());
  } else if (auto branchInst = dyn_cast<BranchInst>(inst)) {
    succes.push_back(dyn_cast<BasicBlock>(branchInst->true_block()).get());
    succes.push_back(dyn_cast<BasicBlock>(branchInst->false_block()).get());
  } else {
    // do nothing, maybe function exit
  }
  return succes;
}

bool BasicBlock::classof(Value *value) {
  switch (value->classId()) {
    case ClassId::BasicBlockId: return true;
    default: return false;
  }
}

bool BasicBlock::classof(const Value *value) {
  switch (value->classId()) {
    case ClassId::BasicBlockId: return true;
    default: return false;
  }
}

void BasicBlock::ClearInst() {
  for (const auto &i : _insts) {
    // remove all of its uses
    dyn_cast<Instruction>(i)->Clear();
  }

  // clear instruction list
  _insts.clear();
}

void BasicBlock::DeleteSelf() {
  // remove all use of predecessors
  this->Clear();

  // remove all of its instructions
  this->ClearInst();
}

BranchInst::BranchInst(const SSAPtr &cond, const SSAPtr &true_block, const SSAPtr &false_block)
    : TerminatorInst(Instruction::TermOps::Br, 3) {
  AddValue(cond);
  AddValue(true_block);
  AddValue(false_block);
}

CallInst::CallInst(const SSAPtr &callee, const std::vector<SSAPtr> &args) :
    Instruction(Instruction::OtherOps::Call, args.size() + 1, ClassId::CallInstId) {
  AddValue(callee);
  for (const auto &it : args) AddValue(it);
}

ICmpInst::ICmpInst(Operator op, const SSAPtr &lhs, const SSAPtr &rhs)
    : Instruction(Instruction::OtherOps::ICmp, 2, ClassId::ICmpInstId), _op(op) {
  AddValue(lhs);
  AddValue(rhs);
}

AccessInst::AccessInst(AccessType acc_type, const SSAPtr &ptr, const SSAPtrList &indexs)
    : Instruction(Instruction::MemoryOps::Access, 0, ClassId::AccessInstId), _acc_type(acc_type) {
  AddValue(ptr);
  for (const auto &it : indexs) {
    AddValue(it);
  }
}

std::string ICmpInst::opStr() const {
  std::string op;
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

bool Instruction::NeedLoad() const {
  bool res = true;
  if (this->isBinaryOp()) {
    res &= false;
  }
  if (this->isCast()) {
    res &= false;
  }
  if (this->opcode() == Instruction::Call) {
    res &= false;
  }
  return res;
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
  DumpValue(os, id_mgr, operand.value());
}

template<typename It>
inline void DumpValue(std::ostream &os, IdManager &id_mgr, It begin, It end) {
  for (auto it = begin; it != end; ++it) {
    if (it != begin) os << ", ";
    DumpValue(os, id_mgr, *it);
  }
}

inline void DumpBlockName(std::ostream &os, IdManager &id_mgr, const BasicBlock *block) {
  auto &npos = std::string::npos;
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
  } else if (name.find("lhs.true") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_LHS_TRUE);
  } else if (name.find("lhs.false") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_LHS_FALSE);
  } else if (name.find("land.end") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_LAND_END);
  } else if (name.find("lor.end") != npos) {
    os << name << id_mgr.GetId(block, IdType::_ID_LOR_END);
  }
  else if (name.find("block") != npos) {
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
  if (!_name.empty()) {
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
        auto tmpType = define::MakePointer(define::MakePrimType(define::Type::Int32, true));
        DumpType(os, tmpType);
      } else {
        DumpType(os, args_type[i]);
      }

      if (!_is_decl) {
        os << " ";  // span between type and name
        _args[i]->Dump(os, id_mgr);
      }

      // separate each parameters
      if (i != args_type.size() - 1) os << ", ";
    }
  }
  os << ")";

  if (_is_decl) {
    os << "\n" << std::endl;
    return;
  }

  os << " {\n";

  // dump content of blocks
  bool report_exit = false;
  for (std::size_t i = 0; i < this->size(); i++) {

    // dump function exit later
    if (auto block = dyn_cast<BasicBlock>((*this)[i].value())) {
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
  DumpWithType(os, id_mgr, data());
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
  bool ret;
  if (_name.empty()) {
    ret = PrintPrefix(os, id_mgr, this);
  } else {
    ret = PrintPrefix(os, id_mgr, _name);
  }

  if (ret) return;

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
  if (!_name.empty()) {
    os << _name;
    if (in_expr) return;
  }

  os << " = global ";
  DumpType(os, type()->GetDerefedType());
  os << " [";
  for (std::size_t i = 0; i < this->size(); i++) {
    DumpWithType(os, id_mgr, (*this)[i].value());
    if (i != this->size() - 1) os << ", ";
  }
  os << "]" << std::endl;
}

void CallInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  bool is_void = type()->IsVoid();

  // do not print name if its return type is void
  if (!is_void && PrintPrefix(os, id_mgr, this)) return;

  auto guard = InExpr();

  if (is_void) os << xIndent; // print indent if return void

  os << "call ";
  // dump return type
  DumpType(os, type());
  os << " @";

  // dump callee name
  auto callee = Callee();
  auto name = std::static_pointer_cast<Function>(callee)->GetFunctionName();
  os << name << "(";

  for (std::size_t i = 1; i < size(); i++) {
    auto arg = (*this)[i].value();
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
  if (PrintPrefix(os, id_mgr, this)) return;
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
  os << std::endl;
}

void GlobalVariable::Dump(std::ostream &os, IdManager &id_mgr) const {
  os << "@" << _name;
  if (in_expr) return;
  auto &init_val = init();

  if (init_val) {
    auto init_type = init_val->type();
    /* Dump array */
    if (init_type->GetDerefedType() && init_type->GetDerefedType()->IsArray()) {
      init_val->Dump(os, id_mgr);
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
  os << std::endl;
}

void AccessInst::Dump(std::ostream &os, IdManager &id_mgr) const {
  if (PrintPrefix(os, id_mgr, this)) return;
  auto guard = InExpr();
  os << "getelementptr inbounds ";
  auto ptr_ssa = ptr();
  auto ptr_type = ptr_ssa->type();
  DumpType(os, ptr_type->GetDerefedType());
  os << ", ";
  DumpWithType(os, id_mgr, ptr());
  os << ", ";

  // dump index
  std::size_t index_len = this->size();
  for (std::size_t i = 1; i < index_len; i++) {
    DumpWithType(os, id_mgr, index(i));
    if (i != index_len - 1) os << ", ";
  }
  os << std::endl;
}

}