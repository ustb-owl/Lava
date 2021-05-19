#include <memory>
#include "module.h"
#include "constant.h"
#include "idmanager.h"

using namespace lava::define;

namespace lava::mid {

void Module::reset() {
  _global_vars.clear();
  _value_symtab.reset();
  _functions.clear();
}

xstl::Guard Module::NewEnv() {
  _value_symtab = lib::MakeNestedMap(_value_symtab);
  return xstl::Guard([this] { _value_symtab = _value_symtab->outer(); });
}

void Module::Dump(std::ostream &os) {
  IdManager id_mgr;

  // dump global value
  for (const auto &it : _global_vars) {
    it->Dump(os, id_mgr);
  }

  os << std::endl;

  // dump functions
  for (const auto &it : _functions) {
    it->Dump(os, id_mgr);
  }
}

xstl::Guard Module::SetContext(const front::Logger &logger) {
  return SetContext(std::make_shared<front::Logger>(logger));
}

xstl::Guard Module::SetContext(const front::LoggerPtr &logger) {
  _loggers.push(logger);
  return xstl::Guard([this] { _loggers.pop(); });
}

FuncPtr Module::CreateFunction(const std::string &name, const define::TypePtr &type) {
  DBG_ASSERT(type->IsFunction(), "not function type");
  auto func = MakeSSA<Function>(name);
  func->set_type(type);
  _functions.push_back(func);
  return func;
}

FuncPtr Module::GetFunction(const std::string &func_name) {
  for (const auto &it : _functions) {
    it->GetFunctionName();
    if (it->GetFunctionName() == func_name) {
      return it;
    }
  }
  return nullptr;
}

SSAPtr Module::GetValues(const std::string &var_name) {
  return _value_symtab->GetItem(var_name);
}

BlockPtr Module::CreateBlock(const UserPtr &parent) {
  return CreateBlock(parent, "block");
}

BlockPtr Module::CreateBlock(const UserPtr &parent, const std::string &name) {
  DBG_ASSERT((parent != nullptr) && parent->type()->IsFunction(),
             "block's parent should be function type");
  auto block = MakeSSA<BasicBlock>(parent, name);
  block->set_type(nullptr);

  // update parent function use-def info
  parent->AddValue(block);
  return block;
}

SSAPtr Module::CreateJump(const BlockPtr &target) {
  auto jump = AddInst<JumpInst>(target);
  jump->set_type(nullptr);

  // add the use to predecessor
  target->AddValue(_insert_point);
  return jump;
}

// TODO: add necessary cast
SSAPtr Module::CreateReturn(const SSAPtr &value) {
  auto ret = AddInst<ReturnInst>(value);
  ret->set_type(nullptr);
  return ret;
}

SSAPtr Module::CreateBranch(const SSAPtr &cond, const BlockPtr &true_block,
                            const BlockPtr &false_block) {
  // check condition type
  DBG_ASSERT(cond->type()->IsInteger(), "cond type should be integer");

  // create branch instruction
  auto br = AddInst<BranchInst>(cond, true_block, false_block);
  br->set_type(nullptr);

  // update predecessor
  true_block->AddValue(_insert_point);
  false_block->AddValue(_insert_point);
  return br;
}


// TODO: add necessary cast before store
SSAPtr Module::CreateStore(const SSAPtr &V, const SSAPtr &P) {
  auto store = AddInst<StoreInst>(V, P);
  store->set_type(nullptr);
  return store;
}

SSAPtr Module::CreateArgRef(const SSAPtr &func, std::size_t index, const std::string &arg_name) {
  // checking
  auto args_type = *func->type()->GetArgsType();
  DBG_ASSERT(index < args_type.size(), "index out of range");

  // set arg type
  auto arg_ref = MakeSSA<ArgRefSSA>(func, index, arg_name);
  arg_ref->set_type(args_type[index]);

  // update function
  std::dynamic_pointer_cast<Function>(func)->set_arg(index, arg_ref);
  return arg_ref;
}

SSAPtr Module::CreateAlloca(const define::TypePtr &type) {
  // set insert point to entry block
  auto insert_point = _insert_point;
  auto last_pos = _func_entry->inst_end();
  SetInsertPoint(_func_entry, --last_pos);

  // create alloca and insert it to entry
  DBG_ASSERT(!type->IsVoid(), "alloc type can't be void");
  auto alloca = AddInst<AllocaInst>();
  auto ptr_type = MakePointer(type);
  alloca->set_type(ptr_type);

  // recover insert point to previous block
  SetInsertPoint(insert_point);
  return alloca;
}

SSAPtr Module::CreateLoad(const SSAPtr &ptr) {
  auto type = ptr->type();
  DBG_ASSERT(type->IsPointer(), "loading from non-pointer type is forbidden");
  auto load = AddInst<LoadInst>(ptr);

  // set load type
  auto load_type = ptr->type()->GetDerefedType();
  DBG_ASSERT(load_type != nullptr, "load type is nullptr");
  load->set_type(load_type);
  return load;
}

static unsigned OpToOpcode(front::Operator op) {
  using CastOps   = Instruction::CastOps;
  using OtherOps  = Instruction::OtherOps;
  using BinaryOps = Instruction::BinaryOps;
  using AssignOps = Instruction::AssignOps;
  using MemoryOps = Instruction::MemoryOps;
  switch (op) {
    /* ----------- unary operator ------------ */
    case front::Operator::Not:      return BinaryOps::Xor;
    case front::Operator::Neg:      return BinaryOps::Sub;
    case front::Operator::LNot:     return BinaryOps::And;  // operand && 1

    /* ----------- binary operator ------------ */
    case front::Operator::Add:      return BinaryOps::Add;
    case front::Operator::Sub:      return BinaryOps::Sub;
    case front::Operator::Mul:      return BinaryOps::Mul;
    case front::Operator::SDiv:     return BinaryOps::SDiv;
    case front::Operator::UDiv:     return BinaryOps::UDiv;
    case front::Operator::SRem:     return BinaryOps::SRem;
    case front::Operator::URem:     return BinaryOps::URem;
    case front::Operator::And:      return BinaryOps::And;
    case front::Operator::Or:       return BinaryOps::Or;
    case front::Operator::Xor:      return BinaryOps::Xor;
    case front::Operator::Shl:      return BinaryOps::Shl;
    case front::Operator::AShr:     return BinaryOps::AShr;
    case front::Operator::LShr:     return BinaryOps::LShr;
    case front::Operator::LAnd:     return BinaryOps::And;
    case front::Operator::LOr:      return BinaryOps::Or;

    /* ----------- compare operator ------------ */
    case front::Operator::Equal:    return OtherOps::ICmp;
    case front::Operator::NotEqual: return OtherOps::ICmp;
    case front::Operator::SLess:    return OtherOps::ICmp;
    case front::Operator::ULess:    return OtherOps::ICmp;
    case front::Operator::SGreat:   return OtherOps::ICmp;
    case front::Operator::UGreat:   return OtherOps::ICmp;
    case front::Operator::SLessEq:  return OtherOps::ICmp;
    case front::Operator::ULessEq:  return OtherOps::ICmp;
    case front::Operator::SGreatEq: return OtherOps::ICmp;
    case front::Operator::UGreatEq: return OtherOps::ICmp;

    /* ----------- assign operator ------------ */
    case front::Operator::Assign:   return AssignOps::Assign;
    case front::Operator::AssAdd:   return AssignOps::AssAdd;
    case front::Operator::AssSub:   return AssignOps::AssSub;
    case front::Operator::AssMul:   return AssignOps::AssMul;
    case front::Operator::AssSDiv:  return AssignOps::AssSDiv;
    case front::Operator::AssUDiv:  return AssignOps::AssUDiv;
    case front::Operator::AssSRem:  return AssignOps::AssSRem;
    case front::Operator::AssURem:  return AssignOps::AssURem;
    case front::Operator::AssAnd:   return AssignOps::AssAnd;
    case front::Operator::AssOr:    return AssignOps::AssOr;
    case front::Operator::AssXor:   return AssignOps::AssXor;
    case front::Operator::AssShl:   return AssignOps::AssShl;
    case front::Operator::AssAShr:  return AssignOps::AssAShr;
    case front::Operator::AssLShr:  return AssignOps::AssLShr;
    case front::Operator::Access:
      break;
    case front::Operator::Arrow:
      break;
    case front::Operator::Pos:
      break;
    case front::Operator::Deref:   return MemoryOps::Load;
    case front::Operator::Addr:    return CastOps::PtrToInt;
    case front::Operator::SizeOf:
      break;
  }
  return OtherOps::Undef;
}

// S1 = S2;
SSAPtr Module::CreateAssign(const SSAPtr &S1, const SSAPtr &S2) {
  if (S2->type()->IsConst() || IsBinaryOperator(S2) || IsCallInst(S2)) {
    // S1 = C ---> store C, s1
    auto store_inst = AddInst<StoreInst>(S2, S1);
    DBG_ASSERT(store_inst != nullptr, "emit store inst failed");
    return store_inst;
  } else {
    // S1 = S2 ---> %0 = load s2; store %0, i32* s1
    auto load_inst = CreateLoad(S2);

    // TODO: add necessary cast here
    auto store_inst = AddInst<StoreInst>(load_inst, S1);
    DBG_ASSERT(store_inst != nullptr, "emit store inst failed");
    return store_inst;
  }
}

SSAPtr Module::CreatePureBinaryInst(Instruction::BinaryOps opcode,
                                    const SSAPtr &S1, const SSAPtr &S2) {
  DBG_ASSERT(opcode >= Instruction::BinaryOps::Add, "opcode is not pure binary operator");
  SSAPtr load_s1 = nullptr;
  SSAPtr load_s2 = nullptr;
  if (!S1->type()->IsConst() && !IsBinaryOperator(S1) && !IsCallInst(S1)) {
    load_s1 = CreateLoad(S1);
    DBG_ASSERT(load_s1 != nullptr, "emit load S1 failed");
  }

  if (!S2->type()->IsConst() && !IsBinaryOperator(S2) && !IsCallInst(S2)) {
    load_s2 = CreateLoad(S2);
    DBG_ASSERT(load_s2 != nullptr, "emit load S2 failed");
  }
  auto bin_inst = BinaryOperator::Create(opcode,
                                         ((load_s1 != nullptr) ? load_s1 : S1),
                                         ((load_s2 != nullptr) ? load_s2 : S2));

  DBG_ASSERT(bin_inst != nullptr, "emit binary instruction failed");

  // set binary instruction type
  auto s1_type = S1->type();
  if (s1_type->IsPointer()) {
    bin_inst->set_type(s1_type->GetDerefedType());
  } else {
    bin_inst->set_type(S1->type());
  }

  // add inst into basic block
  _insert_point->AddInstToEnd(bin_inst);
  return bin_inst;
}


SSAPtr Module::CreateBinaryOperator(define::BinaryStmt::Operator op,
                                    const SSAPtr &S1, const SSAPtr &S2) {
  using OtherOps  = Instruction::OtherOps;
  using BinaryOps = Instruction::BinaryOps;
  using AssignOps = Instruction::AssignOps;
  auto opcode = OpToOpcode(op);

  // create assign operator
  if (opcode == AssignOps::Assign) {
    return CreateAssign(S1, S2);
  } else if (opcode >= AssignOps::AssAdd && opcode <= AssignOps::AssignOpsEnd) {
    auto bin_inst = CreatePureBinaryInst(static_cast<BinaryOps>(opcode - Instruction::AssignSpain), S1, S2);
    auto assign_inst = CreateAssign(S1, bin_inst);
    return assign_inst;
  } else if (opcode == OtherOps::ICmp) {
    return CreateICmpInst(op, S1, S2);
  } else if (opcode >= BinaryOps::Add && opcode <= BinaryOps::BinaryOpsEnd) {
    return CreatePureBinaryInst(static_cast<BinaryOps>(opcode), S1, S2);
  }

  return nullptr;
}

SSAPtr Module::CreateConstInt(unsigned int value) {
  auto const_int =  MakeSSA<ConstantInt>(value);
  const_int->set_type(MakeConst(Type::UInt32));
  DBG_ASSERT(const_int != nullptr, "emit const int value failed");
  return const_int;
}

SSAPtr Module::CreateCallInst(const SSAPtr &callee, const std::vector<SSAPtr>& args) {
  std::vector<SSAPtr> new_args;
  for (const auto &it : args) {

    if (it->type()->IsConst() || IsBinaryOperator(it) || IsCallInst(it)) {
      new_args.push_back(it);
    } else {
      auto load_inst = CreateLoad(it);
      DBG_ASSERT(load_inst != nullptr, "emit load inst before call inst failed");
      new_args.push_back(load_inst);
    }
  }

  auto call_inst = AddInst<CallInst>(callee, new_args);
  DBG_ASSERT(call_inst != nullptr, "emit call inst failed");
  auto callee_type = callee->type();
  call_inst->set_type(callee_type->GetReturnType(callee_type->GetArgsType().value()));
  return call_inst;
}

SSAPtr Module::CreateICmpInst(define::BinaryStmt::Operator opcode, const SSAPtr &lhs, const SSAPtr &rhs) {
  DBG_ASSERT(lhs != nullptr, "lhs SSA is null ptr");
  DBG_ASSERT(rhs != nullptr, "rhs SSA is null ptr");

  SSAPtr icmp_inst, lhs_ssa, rhs_ssa;
  bool is_lhs_bin = false, is_rhs_bin = false;

  if (lhs->isInstruction()) {
    auto lhs_inst = std::static_pointer_cast<Instruction>(lhs);
    if (lhs_inst->isBinaryOp()) {
      is_lhs_bin = true;
    }
  }

  if (rhs->isInstruction()) {
    auto rhs_inst = std::static_pointer_cast<Instruction>(rhs);
    if (rhs_inst->isBinaryOp()) {
      is_rhs_bin = true;
    }
  }

  if (lhs->type()->IsConst() || is_lhs_bin) {
    lhs_ssa = lhs;
  } else {
    lhs_ssa = CreateLoad(lhs);
  }

  if (rhs->type()->IsConst() || is_rhs_bin) {
    rhs_ssa = rhs;
  } else {
    rhs_ssa = CreateLoad(rhs);
  }

  icmp_inst = AddInst<ICmpInst>(opcode, lhs_ssa, rhs_ssa);
  DBG_ASSERT(icmp_inst != nullptr, "emit ICmp instruction failed");
  icmp_inst->set_type(MakePrimType(Type::Bool, true));

  return icmp_inst;
}

SSAPtr Module::CreateCastInst(const SSAPtr &operand, const TypePtr &type) {
  // type checking
  const auto &operand_type = operand->type();
  auto target = type->GetTrivialType();
  DBG_ASSERT(operand_type->IsIdentical(target) ||
             operand_type->CanCastTo(target), "can't cast this two type");

  // return is redundant type cast
  if (operand_type->IsIdentical(target)) return operand;
  if (operand_type->GetSize() == target->GetSize()) return operand;

  Instruction::CastOps op = Instruction::CastOps::CastOpsEnd;

  // get address of array
  auto operand_tmp = operand;
  if (operand_type->IsArray()) {
    operand_tmp = operand->GetAddr();
    DBG_ASSERT(operand_tmp != nullptr, "can't fetch array address");
  }

  if ((target->IsArray() || target->IsPointer()) && operand_type->IsInteger()) {
    op = Instruction::CastOps::PtrToInt;
  } else if ((operand_type->IsPointer() || operand_type->IsArray()) && target->IsInteger()) {
    op = Instruction::CastOps::IntToPtr;
  } else {
    // 1. trunc to small size
    // 2. extend to large size (sign or unsigend)
    if (operand_type->GetSize() > target->GetSize()) {
      op = Instruction::CastOps::Trunc;
    } else {
      if (target->IsUnsigned()) {
        op = Instruction::CastOps::ZExt;
      } else {
        op = Instruction::CastOps::SExt;
      }
    }
  }
  DBG_ASSERT(op != Instruction::CastOps::CastOpsEnd, "get cast operator failed");

  // TODO: maybe need distinguish const with non-const
  // create type casting
  SSAPtr cast;
//  if (operand_tmp->IsConst()) {
    // create a constant type casting, do not insert as an instruction
//    cast = MakeSSA<CastInst>(op, operand_tmp);
//  } else {
    // create a non-constant type casting
    cast = AddInst<CastInst>(op, operand_tmp);
//  }
  DBG_ASSERT(cast != nullptr, "emit cast instruction failed");

  cast->set_type(target);
  return cast;
}

GlobalVarPtr Module::CreateGlobalVar(bool is_var, const std::string &name, const lava::define::TypePtr &type) {
  return CreateGlobalVar(is_var, name, type, nullptr);
}

GlobalVarPtr Module::CreateGlobalVar(bool is_var, const std::string &name, const TypePtr &type, const SSAPtr &init) {
  DBG_ASSERT(!type->IsVoid(), "global variable shouldn't be void type");
  auto var_type = type->GetTrivialType();
  DBG_ASSERT(!init || var_type->IsIdentical(init->type()), "init value type is not allow for global variable");
  DBG_ASSERT(!init || init->IsConst(), "init value of global variable should be const");

  auto global = MakeSSA<GlobalVariable>(is_var, name, init);
  global->set_type(MakePointer(var_type, false));

  // add to global variables
  _global_vars.push_back(global);
  return global;
}


}