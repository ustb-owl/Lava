#include "builder_context.h"

#include <memory>
#include <string>

#include "common/casting.h"
#include "constant.h"

using namespace lava::define;

namespace lava::mid {

void IRBuilderContext::reset() {
  _array_id     = 0;
  _return_val   = nullptr;
  _func_entry   = nullptr;
  _func_exit    = nullptr;
  _insert_point = nullptr;
  _value_symtab.reset();
  _insert_pos = InstList::iterator();
  _loggers    = {};
  _break_cont = {};
  _array_lens.clear();
  _origin_array.clear();
}

xstl::Guard IRBuilderContext::NewEnv() {
  _value_symtab = lib::MakeNestedMap(_value_symtab);
  return xstl::Guard([this] { _value_symtab = _value_symtab->outer(); });
}

xstl::Guard IRBuilderContext::SetContext(const front::Logger &logger) {
  return SetContext(std::make_shared<front::Logger>(logger));
}

xstl::Guard IRBuilderContext::SetContext(const front::LoggerPtr &logger) {
  _loggers.push(logger);
  return xstl::Guard([this] { _loggers.pop(); });
}

BlockPtr IRBuilderContext::CreateBlock(const FuncPtr &parent) {
  return CreateBlock(parent, "block");
}

BlockPtr IRBuilderContext::CreateBlock(const FuncPtr     &parent,
                                       const std::string &name) {
  return CreateBlock(parent.get(), name);
}

BlockPtr IRBuilderContext::CreateBlock(Function *parent) {
  return CreateBlock(parent, "block");
}

BlockPtr IRBuilderContext::CreateBlock(Function          *parent,
                                       const std::string &name) {
  auto block = module().CreateBlock(parent, name);
  ApplyLogger(block);
  return block;
}

SSAPtr IRBuilderContext::CreateJump(const BlockPtr &target) {
  auto jump = AddInst<JumpInst>(target);
  jump->set_type(nullptr);
  target->AddPredecessor(_insert_point);
  return jump;
}

SSAPtr IRBuilderContext::CreateStore(const SSAPtr &V, const SSAPtr &P) {
  auto val       = V;
  auto target_ty = P->type()->GetDerefedType();
  if (!V->type()->IsIdentical(target_ty)) {
    val = CreateCastInst(V, target_ty);
  }
  auto store = AddInst<StoreInst>(val, P);
  store->set_type(nullptr);
  return store;
}

SSAPtr IRBuilderContext::CreateArgRef(const SSAPtr &func, std::size_t index,
                                      const std::string &arg_name) {
  auto arg_ref = module().CreateArgRef(func, index, arg_name);
  ApplyLogger(arg_ref);
  return arg_ref;
}

SSAPtr IRBuilderContext::CreateAlloca(const TypePtr &type) {
  auto insert_point = _insert_point;
  auto last_pos     = _func_entry->inst_end();
  if (!_func_entry->empty())
    --last_pos;
  SetInsertPoint(_func_entry, last_pos);

  DBG_ASSERT(!type->IsVoid(), "alloc type can't be void");
  auto alloca   = AddInst<AllocaInst>();
  auto ptr_type = MakePointer(type);
  alloca->set_type(ptr_type);

  SetInsertPoint(insert_point);
  return alloca;
}

SSAPtr IRBuilderContext::CreateReturn(const SSAPtr &value) {
  auto ret = AddInst<ReturnInst>(value);
  ret->set_type(nullptr);
  return ret;
}

SSAPtr IRBuilderContext::CreateLoad(const SSAPtr &ptr) {
  auto type = ptr->type();
  DBG_ASSERT(type->IsPointer(), "loading from non-pointer type is forbidden");
  auto load = AddInst<LoadInst>(ptr);

  auto load_type = ptr->type()->GetDerefedType();
  DBG_ASSERT(load_type != nullptr, "load type is nullptr");
  load->set_type(load_type);
  return load;
}

SSAPtr IRBuilderContext::CreateBranch(const SSAPtr   &cond,
                                      const BlockPtr &true_block,
                                      const BlockPtr &false_block) {
  DBG_ASSERT(cond != nullptr, "branch condition is nullptr");
  DBG_ASSERT(cond->type()->IsInteger(), "cond type should be integer");
  auto br = AddInst<BranchInst>(cond, true_block, false_block);
  br->set_type(nullptr);
  true_block->AddPredecessor(_insert_point);
  false_block->AddPredecessor(_insert_point);
  return br;
}

static unsigned OpToOpcode(front::Operator op) {
  using CastOps   = Instruction::CastOps;
  using OtherOps  = Instruction::OtherOps;
  using BinaryOps = Instruction::BinaryOps;
  using AssignOps = Instruction::AssignOps;
  using MemoryOps = Instruction::MemoryOps;
  switch (op) {
  case front::Operator::Not:
    return BinaryOps::Xor;
  case front::Operator::Neg:
    return BinaryOps::Sub;
  case front::Operator::LNot:
    return BinaryOps::And;
  case front::Operator::Add:
    return BinaryOps::Add;
  case front::Operator::Sub:
    return BinaryOps::Sub;
  case front::Operator::Mul:
    return BinaryOps::Mul;
  case front::Operator::SDiv:
    return BinaryOps::SDiv;
  case front::Operator::UDiv:
    return BinaryOps::UDiv;
  case front::Operator::SRem:
    return BinaryOps::SRem;
  case front::Operator::URem:
    return BinaryOps::URem;
  case front::Operator::And:
    return BinaryOps::And;
  case front::Operator::Or:
    return BinaryOps::Or;
  case front::Operator::Xor:
    return BinaryOps::Xor;
  case front::Operator::Shl:
    return BinaryOps::Shl;
  case front::Operator::AShr:
    return BinaryOps::AShr;
  case front::Operator::LShr:
    return BinaryOps::LShr;
  case front::Operator::LAnd:
    return BinaryOps::And;
  case front::Operator::LOr:
    return BinaryOps::Or;
  case front::Operator::Equal:
  case front::Operator::NotEqual:
  case front::Operator::SLess:
  case front::Operator::ULess:
  case front::Operator::SGreat:
  case front::Operator::UGreat:
  case front::Operator::SLessEq:
  case front::Operator::ULessEq:
  case front::Operator::SGreatEq:
  case front::Operator::UGreatEq:
    return OtherOps::ICmp;
  case front::Operator::Assign:
    return AssignOps::Assign;
  case front::Operator::AssAdd:
    return AssignOps::AssAdd;
  case front::Operator::AssSub:
    return AssignOps::AssSub;
  case front::Operator::AssMul:
    return AssignOps::AssMul;
  case front::Operator::AssSDiv:
    return AssignOps::AssSDiv;
  case front::Operator::AssUDiv:
    return AssignOps::AssUDiv;
  case front::Operator::AssSRem:
    return AssignOps::AssSRem;
  case front::Operator::AssURem:
    return AssignOps::AssURem;
  case front::Operator::AssAnd:
    return AssignOps::AssAnd;
  case front::Operator::AssOr:
    return AssignOps::AssOr;
  case front::Operator::AssXor:
    return AssignOps::AssXor;
  case front::Operator::AssShl:
    return AssignOps::AssShl;
  case front::Operator::AssAShr:
    return AssignOps::AssAShr;
  case front::Operator::AssLShr:
    return AssignOps::AssLShr;
  case front::Operator::Deref:
    return MemoryOps::Load;
  case front::Operator::Addr:
    return CastOps::PtrToInt;
  case front::Operator::Access:
  case front::Operator::Arrow:
  case front::Operator::Pos:
  case front::Operator::SizeOf:
    break;
  }
  return OtherOps::Undef;
}

SSAPtr IRBuilderContext::CreateAssign(const SSAPtr &S1, const SSAPtr &S2) {
  auto store_inst = CreateStore(S2, S1);
  DBG_ASSERT(store_inst != nullptr, "emit store inst failed");
  return store_inst;
}

SSAPtr IRBuilderContext::CreatePureBinaryInst(Instruction::BinaryOps opcode,
                                              const SSAPtr          &S1,
                                              const SSAPtr          &S2) {
  DBG_ASSERT(opcode >= Instruction::BinaryOps::Add,
             "opcode is not pure binary operator");
  auto bin_inst = AddInst<BinaryOperator>(opcode, S1, S2, S1->type());
  DBG_ASSERT(bin_inst != nullptr, "emit binary instruction failed");
  bin_inst->set_type(S1->type());
  return bin_inst;
}

SSAPtr IRBuilderContext::CreateBinaryOperator(BinaryStmt::Operator op,
                                              const SSAPtr        &S1,
                                              const SSAPtr        &S2) {
  using OtherOps  = Instruction::OtherOps;
  using BinaryOps = Instruction::BinaryOps;
  using AssignOps = Instruction::AssignOps;
  auto opcode     = OpToOpcode(op);

  if (opcode == AssignOps::Assign) {
    return CreateAssign(S1, S2);
  }
  if (opcode >= AssignOps::AssAdd && opcode <= AssignOps::AssignOpsEnd) {
    auto bin_inst = CreatePureBinaryInst(
        static_cast<BinaryOps>(opcode - Instruction::AssignSpain), S1, S2);
    return CreateAssign(S1, bin_inst);
  }
  if (opcode == OtherOps::ICmp) {
    return CreateICmpInst(op, S1, S2);
  }
  if (opcode >= BinaryOps::Add && opcode <= BinaryOps::BinaryOpsEnd) {
    return CreatePureBinaryInst(static_cast<BinaryOps>(opcode), S1, S2);
  }

  return nullptr;
}

SSAPtr IRBuilderContext::CreateConstInt(unsigned int value, Type type) {
  auto const_int = module().CreateConstInt(value, type);
  ApplyLogger(const_int);
  return const_int;
}

SSAPtr IRBuilderContext::CreateCallInst(const SSAPtr              &callee,
                                        const std::vector<SSAPtr> &args) {
  auto callee_func = dyn_cast<Function>(callee);
  DBG_ASSERT(callee_func != nullptr, "callee is not a direct function");
  DBG_ASSERT(callee_func->type()->IsFunction(), "callee is not function type");
  auto args_type = *callee_func->type()->GetArgsType();
  DBG_ASSERT(args_type.size() == args.size(), "arguments size not fit");
  auto call_inst = AddInst<CallInst>(callee_func, args);
  DBG_ASSERT(call_inst != nullptr, "emit call inst failed");
  auto callee_type = callee_func->type();
  call_inst->set_type(
      callee_type->GetReturnType(callee_type->GetArgsType().value()));
  return call_inst;
}

SSAPtr IRBuilderContext::CreateICmpInst(BinaryStmt::Operator opcode,
                                        const SSAPtr &lhs, const SSAPtr &rhs) {
  DBG_ASSERT(lhs != nullptr, "lhs SSA is null ptr");
  DBG_ASSERT(rhs != nullptr, "rhs SSA is null ptr");

  auto icmp_inst = AddInst<ICmpInst>(opcode, lhs, rhs);
  DBG_ASSERT(icmp_inst != nullptr, "emit ICmp instruction failed");
  icmp_inst->set_type(MakePrimType(Type::Bool, true));
  return icmp_inst;
}

SSAPtr IRBuilderContext::CreateCastInst(const SSAPtr  &operand,
                                        const TypePtr &type) {
  const auto &operand_type = operand->type();
  auto        target       = type->GetTrivialType();
  DBG_ASSERT(operand_type->IsIdentical(target) ||
                 operand_type->CanCastTo(target),
             "can't cast this two type");

  if (operand_type->IsIdentical(target))
    return operand;
  if (operand_type->GetSize() == target->GetSize())
    return operand;

  Instruction::CastOps op          = Instruction::CastOps::CastOpsEnd;
  auto                 operand_tmp = operand;
  if (operand_type->IsArray()) {
    operand_tmp = operand->GetAddr();
    DBG_ASSERT(operand_tmp != nullptr, "can't fetch array address");
  }

  if ((target->IsArray() || target->IsPointer()) && operand_type->IsInteger()) {
    op = Instruction::CastOps::PtrToInt;
  } else if ((operand_type->IsPointer() || operand_type->IsArray()) &&
             target->IsInteger()) {
    op = Instruction::CastOps::IntToPtr;
  } else if (operand_type->GetSize() > target->GetSize()) {
    op = Instruction::CastOps::Trunc;
  } else {
    op = Instruction::CastOps::ZExt;
  }
  DBG_ASSERT(op != Instruction::CastOps::CastOpsEnd,
             "get cast operator failed");

  auto cast = AddInst<CastInst>(op, operand_tmp);
  DBG_ASSERT(cast != nullptr, "emit cast instruction failed");
  cast->set_type(target);
  return cast;
}

SSAPtr IRBuilderContext::CreateElemAccess(const SSAPtr     &ptr,
                                          const SSAPtrList &index) {
  auto pointer  = ptr;
  auto acc_type = AccessInst::AccessType::Element;
  auto access   = AddInst<AccessInst>(acc_type, pointer, index);
  DBG_ASSERT(access != nullptr, "emit access instruction failed");

  TypePtr type = ptr->type();
  for (std::size_t i = 0; i < index.size(); i++) {
    type = type->GetDerefedType();
    DBG_ASSERT(type != nullptr, "type can't be dereferenced");
  }
  access->set_type(
      MakePointer(MakePrimType(Type::Int32, type->IsRightValue())));
  return access;
}

ArrayPtr IRBuilderContext::CreateArray(const SSAPtrList  &elems,
                                       const TypePtr     &type,
                                       const std::string &name) {
  auto array = module().CreateArray(elems, type, name);
  ApplyLogger(array);
  return array;
}

GlobalVarPtr IRBuilderContext::CreateGlobalVar(bool               is_var,
                                               const std::string &name,
                                               const TypePtr     &type) {
  return CreateGlobalVar(is_var, name, type, nullptr);
}

GlobalVarPtr IRBuilderContext::CreateGlobalVar(bool               is_var,
                                               const std::string &name,
                                               const TypePtr     &type,
                                               const SSAPtr      &init) {
  auto global = module().CreateGlobalVar(is_var, name, type, init);
  ApplyLogger(global);
  return global;
}

FuncPtr IRBuilderContext::CreateFunction(const std::string &name,
                                         const TypePtr &type, bool is_decl) {
  auto func = module().CreateFunction(name, type, is_decl);
  ApplyLogger(func);
  return func;
}

SSAPtr IRBuilderContext::GetZeroValue(Type type) {
  return module().GetZeroValue(type);
}

FuncPtr IRBuilderContext::GetFunction(const std::string &func_name) {
  return module().GetFunction(func_name);
}

SSAPtr IRBuilderContext::GetValues(const std::string &var_name) {
  return _value_symtab->GetItem(var_name);
}

std::string IRBuilderContext::GetArrayName() {
  if (_value_symtab->is_root())
    return "";
  auto func_name = "@__const." + _insert_point->getParent()->GetFunctionName();
  return func_name + "." + std::to_string(_array_id++);
}

bool IRBuilderContext::IsGlobalVariable(const SSAPtr &var) const {
  return module().IsGlobalVariable(var);
}

} // namespace lava::mid
