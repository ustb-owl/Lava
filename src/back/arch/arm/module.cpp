#include "module.h"
#include "lib/guard.h"
#include "common/casting.h"
#include "common/idmanager.h"

#define TAB          "\t"
#define SPACE        " "
#define TWO_SPACE    "  "
#define INDENT       "\t"
#define TYPE_LABEL   ".type"
#define GLOBAL_LABEL ".global"
#define FUNC_TYPE    "%function"

namespace lava::back {

IdManager id_mgr;

void LLModule::reset() {
  _virtual_max = 0;
  _functions.clear();
  _glob_decl.clear();
  _block_map.clear();
  _value_map.clear();
  _cond_map.clear();
  _glob_map.clear();
  _param_map.clear();
}

LLOperandPtr LLModule::CreateNoImmOperand(const mid::SSAPtr &value) {
  if (auto constValue = dyn_cast<mid::ConstantInt>(value)) {
    // store the immediate to a register
    auto dst = LLOperand::Virtual(_virtual_max);
    auto imm = LLOperand::Immediate(constValue->value());
    auto mv_inst = AddInst<LLMove>(dst, imm);
    return dst;
  } else {
    return CreateOperand(value);
  }
}

LLOperandPtr LLModule::CreateOperand(const mid::SSAPtr &value) {
  if (auto param = dyn_cast<mid::ArgRefSSA>(value)) {
    auto it = _param_map.find(value);
    if (it != _param_map.end()) {
      return it->second;
    }
    const auto &args = _insert_function->function()->args();
    LLOperandPtr arg = nullptr;
    for (std::size_t i = 0; i < args.size(); i++) {
      if (args[i] == value) {
        // copy from register
        if (i < 4) {
          // r0-r3
          // mov v1, r0
          auto dst = LLOperand::Virtual(_virtual_max);
          auto src = LLOperand::Register((ArmReg) i);

          // insert this instruction at entry
          auto mv_inst = std::make_shared<LLMove>(dst, src);
          auto entry = _insert_function->entry();
          entry->insts().insert(entry->insts().begin(), mv_inst);

          arg = dst;
        } else {
          /* read from sp + (i-4)*4 in entry block */

          // mov rm, (i - 4) * 4

          auto offset = LLOperand::Virtual(_virtual_max);
          auto src = LLOperand::Immediate((i - 4) * 4);
          auto moveInst = AddInst<LLMove>(offset, src);
          moveInst->SetIsArg(true);


          // load rn, [sp, rm]
          auto vreg = LLOperand::Virtual(_virtual_max);
          auto addr = LLOperand::Register(ArmReg::sp);
          auto load_arg = AddInst<LLLoad>(vreg, addr, offset);

          // set load is argument
          load_arg->SetIsArg(true);

          arg = vreg;
        }
        break;
      }
    }

    DBG_ASSERT(arg != nullptr, "create argument failed");
    _param_map[value] = arg;
    return arg;
  } else if (auto constValue = dyn_cast<mid::ConstantInt>(value)) {
    return CreateImmediate(constValue->value());
  } else if (auto globalValue = dyn_cast<mid::GlobalVariable>(value)) {
    auto it = _glob_map.find(globalValue);
    if (it == _glob_map.end()) {
      // save insert_point
      auto insert_point = _insert_point;

      // allocate virtual reg
      auto res = LLOperand::Virtual(_virtual_max);
      _glob_map[globalValue] = res;

      // insert at entry
      SetInsertPoint(_insert_function->entry(), _insert_function->entry()->insts().begin());
      auto load_global_addr = AddInst<LLGlobal>(res, globalValue.get());
      DBG_ASSERT(load_global_addr != nullptr, "load global variable address failed");

      SetInsertPoint(insert_point);
      return res;
    } else {
      return it->second;
    }

  } else if (auto undef = dyn_cast<mid::UnDefineValue>(value)) {
    auto res = CreateImmediate(0);
    return res;
  } else {
    auto it = _value_map.find(value);
    if (it == _value_map.end()) {
      // allocate virtual reg
      auto res = LLOperand::Virtual(_virtual_max);
      _value_map[value] = res;
      return res;
    } else {
      return it->second;
    }
  }
}

LLOperandPtr LLModule::CreateImmediate(int value) {
  auto operand = LLOperand::Immediate(value);
  if (can_encode_imm(value)) {
    return operand;
  } else {
    AddInst<LLComment>("split immediate number + " + std::to_string(value));
    auto dst = LLOperand::Virtual(_virtual_max);
    // ldr dst, =value
    auto mv_inst = AddInst<LLMove>(dst, operand);
    return dst;
  }
}

LLFunctionPtr LLModule::CreateFunction(const mid::FuncPtr &function) {
  auto ll_function = std::make_shared<LLFunction>(function);
  _functions.push_back(ll_function);
  _insert_function = ll_function;

  // create block map
  for (const auto &it : *function) {
    auto block = dyn_cast<mid::BasicBlock>(it.value());
    auto ll_block = std::make_shared<LLBasicBlock>(block->name(), block, ll_function);

    // insert blocks into function
    ll_function->AddBlock(ll_block);

    // insert block into _block_map
    _block_map[block] = ll_block;
  }

  return ll_function;
}

LLBlockPtr LLModule::CreateBasicBlock(const mid::BlockPtr &block, const LLFunctionPtr &parent) {
  auto ll_block = _block_map[block];
  SetInsertPoint(ll_block);
  for (const auto &inst : block->insts()) {

    /* Create each instruction */
    if (auto jumpInst = dyn_cast<mid::JumpInst>(inst)) {
      /* JUMP */
      auto target = _block_map[dyn_cast<mid::BasicBlock>(jumpInst->target())];
      DBG_ASSERT(target != nullptr, "can't find target block in JumpInst");

      auto ll_jumpInst = AddInst<LLJump>(target);
      DBG_ASSERT(ll_jumpInst != nullptr, "create LLJump failed");
    } else if (auto loadInst = dyn_cast<mid::LoadInst>(inst)) {
      /* LOAD */
      // solve operand
      auto src = CreateOperand(inst->GetAddr());

      auto dst = CreateOperand(loadInst);
      auto ll_loadInst = AddInst<LLLoad>(dst, src, nullptr);
      DBG_ASSERT(ll_loadInst != nullptr, "create LLLoad failed");
    } else if (auto storeInst = dyn_cast<mid::StoreInst>(inst)) {
      /* STORE */
      // create operands of store
      // store data, pointer
      auto data = CreateNoImmOperand(storeInst->data());
      DBG_ASSERT(data != nullptr, "create data failed in StoreInst");
      auto pointer = CreateOperand(storeInst->pointer());
      DBG_ASSERT(pointer != nullptr, "create pointer failed in StoreInst");

      auto ll_storeInst = AddInst<LLStore>(data, pointer, nullptr);
      DBG_ASSERT(ll_storeInst != nullptr, "create StoreInst failed");
    } else if (auto gepInst = dyn_cast<mid::AccessInst>(inst)) {
      /* GETELEMENTPTR */
      // getelementptr ptr, index, multiplier
      auto dst = CreateOperand(gepInst);
      DBG_ASSERT(dst != nullptr, "create dst in getelementptr instruction failed");

      auto ptr = CreateOperand(gepInst->ptr());
      DBG_ASSERT(ptr != nullptr, "create pointer in getelementptr instruction failed");

      std::shared_ptr<ConstantInt> multiplier = nullptr;
      if (gepInst->has_multiplier()){
        multiplier = dyn_cast<mid::ConstantInt>(gepInst->multiplier());
      } else {
        multiplier = std::make_shared<mid::ConstantInt>(1);
      }
      DBG_ASSERT(multiplier, "create multiplier failed in getelementptr instruction failed");

      // try to get const index
      auto constIdx = dyn_cast<mid::ConstantInt>(gepInst->index());

      AddInst<LLComment>("begin getelementptr");
      // 1. getelementptr ptr, 0, x || getelementptr ptr, x, 0
      // dst <- ptr
      if (multiplier->IsZero() || (constIdx && constIdx->IsZero())) {
        auto move_inst = AddInst<LLMove>(dst, ptr);
        DBG_ASSERT(move_inst != nullptr, "create LLMove failed in getelementptr instruction");
      } else if (constIdx) {
        // dst <- ptr + (index * multiplier * size)
        auto offset = constIdx->value() * multiplier->value() * 4;
        auto imm_operand = CreateImmediate(offset);

        auto add_inst = AddInst<LLBinaryInst>(LLInst::Opcode::Add, dst, ptr, imm_operand);
        DBG_ASSERT(add_inst != nullptr, "create add instruction failed in getelementptr instruction");
      } else if ((multiplier->value() & (multiplier->value() - 1)) == 0) {
        /* multiplier is 2^n */
        // dst <- ptr + (index << log(multiplier))
        auto index = CreateOperand(gepInst->index());
        auto add_inst = AddInst<LLBinaryInst>(LLInst::Opcode::Add, dst, ptr, index);
        DBG_ASSERT(add_inst != nullptr, "create add instruction failed in getelementptr instruction");
        ArmShift shift(__builtin_ctz(multiplier->value() * 4), ArmShift::ShiftType::Lsl);
        if (shift.shift() != 0) add_inst->setShift(shift);
      } else {
        // dst <- ptr
        auto move_inst = AddInst<LLMove>(dst, ptr);

        // offset <- index << 2
        auto index = CreateOperand(gepInst->index());
        auto size = CreateImmediate(2);

        auto offset = LLOperand::Virtual(_virtual_max);
        auto lsl_inst = AddInst<LLBinaryInst>(LLInst::Opcode::LShr, offset, index, size);

        // multiplier <- multiplier (imm)
        auto move_mult = AddInst<LLMove>(LLOperand::Virtual(_virtual_max), LLOperand::Immediate(multiplier->value()));

        // dst <- index * multiplier + dst
        auto mla = AddInst<LLMLA>(dst, offset, move_mult->dst(), dst);
        DBG_ASSERT(mla != nullptr, "create mla instruction failed in getelementptr instruction");
      }
      AddInst<LLComment>("create getelementptr end");
    } else if (auto returnInst = dyn_cast<mid::ReturnInst>(inst)) {
      if (returnInst->RetVal() != nullptr) {
        auto ret_value = CreateOperand(returnInst->RetVal());

        // move return value to R0
        auto r0 = LLOperand::Register(ArmReg::r0);
        auto move_inst = AddInst<LLMove>(r0, ret_value);
        DBG_ASSERT(move_inst != nullptr, "create move instruction failed in return instruction");

        // create return instruction
        AddInst<LLReturn>();
      } else {
        AddInst<LLReturn>();
      }
    } else if (auto binaryInst = dyn_cast<mid::BinaryOperator>(inst)) {
      AddInst<LLComment>("--- binary here ---");


      // TODO: generate binary instruction
      using LLOpcode  = LLInst::Opcode;
      using BinaryOps = BinaryOperator::BinaryOps;

      BinaryOps opcode = binaryInst->opcode();
      auto lhs = CreateNoImmOperand(binaryInst->LHS());
      DBG_ASSERT(lhs != nullptr, "create lhs failed in binary instruction");
      LLOperandPtr rhs = nullptr;
      if (binaryInst->opcode() == BinaryOps::Mul  ||
          binaryInst->opcode() == BinaryOps::SDiv ||
          binaryInst->opcode() == BinaryOps::SRem) {
        rhs = CreateNoImmOperand(binaryInst->RHS());
      } else {
        rhs = CreateOperand(binaryInst->RHS());
      }
      DBG_ASSERT(rhs != nullptr, "create rhs failed in binary instruction");

      LLOpcode bin_op;
      switch (opcode) {
        case BinaryOps::Add:  bin_op = LLOpcode::Add;  break;
        case BinaryOps::Sub:  bin_op = LLOpcode::Sub;  break;
        case BinaryOps::Mul:  bin_op = LLOpcode::Mul;  break;
        case BinaryOps::SDiv: bin_op = LLOpcode::SDiv; break;
        case BinaryOps::And:  bin_op = LLOpcode::And;  break;
        case BinaryOps::Or:   bin_op = LLOpcode::Or;   break;
        case BinaryOps::Xor:  bin_op = LLOpcode::Xor;  break;
        case BinaryOps::Shl:  bin_op = LLOpcode::Shl;  break;
        case BinaryOps::AShr: bin_op = LLOpcode::AShr; break;
        case BinaryOps::LShr: bin_op = LLOpcode::LShr; break;
        case BinaryOps::SRem: bin_op = LLOpcode::SRem; break;
        default: ERROR("should not reach here");
      }


      // convert mod to sdiv and sub
      // c = b % a
      // sdiv r0, r1, r2
      // mls r0, r0, r2, r1
      // r1 - ((r1 / r2) * r2)
      if (bin_op == LLOpcode::SRem) {
        auto dst = CreateOperand(inst);
        auto sdiv = AddInst<LLBinaryInst>(LLOpcode::SDiv, dst, lhs, rhs);

//        auto mul_res = LLOperand::Virtual(_virtual_max);
        auto mul = AddInst<LLBinaryInst>(LLOpcode::Mul, dst, dst, rhs);
        auto sub = AddInst<LLBinaryInst>(LLOpcode::Sub, dst, lhs, dst);
//        auto mls = AddInst<LLMLS>(dst, dst, rhs, lhs);
        DBG_ASSERT(sub != nullptr, "create mod instruction failed");
      } else {
        auto dst = CreateOperand(inst);
        auto bin_inst = AddInst<LLBinaryInst>(bin_op, dst, lhs, rhs);
        DBG_ASSERT(bin_inst != nullptr, "create binary instruction failed");
      }

    } else if (auto branchInst = dyn_cast<mid::BranchInst>(inst)) {
      ArmCond armCond = ArmCond::Eq;
      if (auto it = _cond_map.find(branchInst->cond()); it != _cond_map.end()) {
        armCond = it->second.second;
      }

      auto cond = CreateOperand(branchInst->cond());

      DBG_ASSERT(_block_map.find(dyn_cast<mid::BasicBlock>(branchInst->true_block())) != _block_map.end(),
                 "can't find true block");
      auto true_block = _block_map[dyn_cast<mid::BasicBlock>(branchInst->true_block())];
      DBG_ASSERT(_block_map.find(dyn_cast<mid::BasicBlock>(branchInst->false_block())) != _block_map.end(),
                 "can't find false block");
      auto false_block = _block_map[dyn_cast<mid::BasicBlock>(branchInst->false_block())];

      auto ll_branch = AddInst<LLBranch>(armCond, cond, true_block, false_block);

    } else if (auto callInst = dyn_cast<mid::CallInst>(inst)) {
      // add lr into callee's saved registers
      parent->SetHasCallInst(true);

      std::vector<LLOperandPtr> params;

      // create parameters
      std::size_t param_size = callInst->param_size();
      for (std::size_t i = 0; i < param_size; i++) {
        if (i < 4) {
          // move args to r0-r3
          auto rhs = CreateOperand(callInst->Param(i));
          auto dst = LLOperand::Register((ArmReg) i);
          auto mv_inst = AddInst<LLMove>(dst, rhs);
          DBG_ASSERT(mv_inst != nullptr, "move param to register failed");
        } else {
          // store to [sp - (n - i) * 4]

          // data
          auto data = CreateNoImmOperand(callInst->Param(i));
          data->set_not_allowed_to_tmp(false);

          // addr
          auto addr = LLOperand::Register(ArmReg::sp);

          // offset
          auto offset = CreateImmediate(-((param_size - i) * 4));
          offset->set_not_allowed_to_tmp(false);
          auto st_inst = AddInst<LLStore>(data, addr, offset);
          DBG_ASSERT(st_inst != nullptr, "store parameter(%lu) failed in calling %s",
                     i, dyn_cast<mid::Function>(callInst->Callee())->GetFunctionName().c_str());
        }
      }

      // create stack for callee
      if (param_size > 4) {
        // sub sp, sp, (n - 4) * 4
        auto dst = LLOperand::Register(ArmReg::sp);
        auto lhs = LLOperand::Register(ArmReg::sp);
        auto rhs = CreateImmediate(4 * (param_size - 4));
        auto sub_inst = AddInst<LLBinaryInst>(LLInst::Opcode::Sub, dst, lhs, rhs);
        rhs->set_not_allowed_to_tmp(false);
      }

      // create call instruction
      auto callee = dyn_cast<mid::Function>(callInst->Callee());
      auto ll_call = AddInst<LLCall>(callee);
      DBG_ASSERT(ll_call != nullptr, "create call instruction failed");

      // recover stack
      if (param_size > 4) {
        // add sp, sp, (n - 4) * 4
        auto dst = LLOperand::Register(ArmReg::sp);
        auto lhs = LLOperand::Register(ArmReg::sp);
        auto rhs = CreateImmediate(4 * (param_size - 4));
        auto sub_inst = AddInst<LLBinaryInst>(LLInst::Opcode::Add, dst, lhs, rhs);
        rhs->set_not_allowed_to_tmp(false);
      }

      // set return value
      if (!callee->type()->GetReturnType(callee->type()->GetArgsType().value())->IsVoid()) {
        // move r0 to dst
        auto dst = CreateOperand(inst);
        auto r0 = LLOperand::Register(ArmReg::r0);
        auto mv_inst = AddInst<LLMove>(dst, r0);
      }
    } else if (auto allocaInst = dyn_cast<mid::AllocaInst>(inst)) {
      // get size
      int size = 4;
      if (allocaInst->type()->GetDerefedType()->IsArray()) {
        size *= allocaInst->type()->GetDerefedType()->GetLength();
      }

      // get address on stack
      auto dst = CreateOperand(inst);
      auto offset = CreateImmediate(_insert_function->stack_size());
      auto sp = LLOperand::Register(ArmReg::sp);
      auto add_inst = AddInst<LLBinaryInst>(LLInst::Opcode::Add, dst, sp, offset);

      // allocate size on sp
      _insert_function->SetStackSize(_insert_function->stack_size() + size);

    } else if (auto icmp_inst = dyn_cast<mid::ICmpInst>(inst)) {
      ArmCond cond = ArmCond::Any, opposite = ArmCond::Any;
      auto lhs = CreateNoImmOperand(icmp_inst->LHS());
      auto rhs = CreateOperand(icmp_inst->RHS());

      switch (icmp_inst->op()) {
        case front::Operator::Equal:    cond = ArmCond::Eq; break;
        case front::Operator::SLess:
        case front::Operator::ULess:    cond = ArmCond::Lt; break;
        case front::Operator::SGreat:
        case front::Operator::UGreat:   cond = ArmCond::Gt; break;
        case front::Operator::SLessEq:
        case front::Operator::ULessEq:  cond = ArmCond::Le; break;
        case front::Operator::SGreatEq:
        case front::Operator::UGreatEq: cond = ArmCond::Ge; break;
        case front::Operator::NotEqual: cond = ArmCond::Ne; break;
        default:
          ERROR("should not reach here");
      }
      opposite = opposite_cond(cond);

      auto ll_cmp = AddInst<LLCompare>(cond, lhs, rhs);
      if (icmp_inst->uses().size() == 1 &&
          (*icmp_inst->uses().begin())->getUser()->classId() == ClassId::BranchInstId) {
        auto pair = std::make_pair(ll_cmp, cond);
        _cond_map.insert({inst, pair});
      } else {
        // TODO: load compare value
        auto dst = CreateOperand(inst);
        auto src1 = CreateImmediate(1);
        auto mv1_inst = AddInst<LLMove>(dst, src1, cond);

        auto src2 = CreateImmediate(0);
        auto mv2_inst = AddInst<LLMove>(dst, src2, opposite);
      }
    } else if (auto cast_inst = dyn_cast<mid::CastInst>(inst)) {
      if (cast_inst->opcode() == mid::Instruction::CastOps::Trunc) {
        auto dst = CreateOperand(inst);
        auto imm = CreateImmediate(1);
        auto src = CreateNoImmOperand(cast_inst->operand());
        AddInst<LLBinaryInst>(LLInst::Opcode::And, dst, src, imm);
      } else if (cast_inst->opcode() == mid::Instruction::CastOps::ZExt) {
        auto dst = CreateOperand(inst);
        auto src = CreateOperand(cast_inst->operand());
        AddInst<LLMove>(dst, src);
      }
    } else {
      // TODO:
    }
  }

  return ll_block;
}

void LLModule::HandlePhiNode(const mid::FuncPtr &function) {
  using PhiAssign = std::vector<std::pair<LLOperandPtr , LLOperandPtr>>;
  for (const auto &BB : *function) {
    auto block = dyn_cast<BasicBlock>(BB.value());
    DBG_ASSERT(_block_map.find(block) != _block_map.end(), "can't find the block in current _block_map");
    auto ll_block = _block_map[block];

    PhiAssign phi_dst;

    std::unordered_map<BlockPtr, PhiAssign> move;
    for (const auto &inst : block->insts()) {
      if (auto phi_inst = dyn_cast<PhiNode>(inst)) {
        // for each phi:
        // 1. create a virtual register for each instruction
        // 2. add mov for each node
        // 2. add mov in each predecessor
        auto vreg = LLOperand::Virtual(_virtual_max);
        auto dst = CreateOperand(inst);
        phi_dst.emplace_back(dst, vreg);
        for (auto i = 0; i < phi_inst->size(); i++) {
          auto pred = dyn_cast<BasicBlock>(block->GetOperand(i));
          DBG_ASSERT(pred != nullptr, "get predecessor of current block failed");
          auto val = CreateOperand((*phi_inst)[i].value());
          move[pred].emplace_back(vreg, val);
        }
      } else {
        break;
      }
    }

    auto begin = ll_block->inst_begin();
    SetInsertPoint(ll_block, begin);
    for (auto &[lhs, rhs] : phi_dst) {
      auto mov_inst = AddInst<LLMove>(lhs, rhs);
      DBG_ASSERT(mov_inst != nullptr, "create move instruction for phi-node failed");
    }

    // insert move instructions in each predecessor
    for (auto &[bb, movs] : move) {
      auto ll_bb = _block_map[bb];

      // find terminate instruction
      auto it = ll_bb->inst_begin();
      for (; it != ll_bb->insts().end(); it++) {
        if ((*it)->classId() == ClassId::LLJumpId)
          break;
        else if ((*it)->classId() == ClassId::LLBranchId) {
          it = std::prev(it);
          break;
        }
      }
      auto pos = it;

      SetInsertPoint(ll_bb, pos);
      for (auto &[lhs, rhs] : movs) {
        auto mov_inst = AddInst<LLMove>(lhs, rhs);
        DBG_ASSERT(mov_inst != nullptr, "create move instruction for phi-node failed");
      }
    }

  }
}


/*----  Dump ASM  ----*/

// indicate if is in instruction
int in_instruction = 0;

xstl::Guard InInst() {
  ++in_instruction;
  return xstl::Guard([] { --in_instruction; });
}

void LLModule::DumpASM(std::ostream &os) const {
  // TODO: dump head
  os << ".arch" << SPACE << "armv7ve" << std::endl;
  os << ".section" << SPACE << ".text" << std::endl;
  os << std::endl;

  // dump function
  for (const auto &function : _functions) {
    if (function->is_decl()) continue;
    os << function << std::endl;
//    id_mgr.Reset();
  }

  if (_glob_decl.empty()) return;
  // dump global variables
  os << ".section" << SPACE << ".data" << std::endl;
  os << ".align 4" << std::endl;
  for (const auto &it : _glob_decl) {
    os << ".global" << SPACE << it->name() << std::endl;
    os << INDENT << ".type" << TAB << it->name() << "," << SPACE << "%object" << std::endl;

    os << it->name() << ":" << std::endl;

    // print zero if has no init value
    const auto &type = it->type()->GetDerefedType();
    if (it->init() == nullptr) {
      if (type->IsArray()) {
        int size = type->GetLength() * 4;
        os << INDENT << ".zero" << TAB << size << std::endl;
      } else {
        os << INDENT << ".zero" << TAB << 4 << std::endl;
      }
    } else {
      if (type->IsArray()) {
        auto array = dyn_cast<mid::ConstantArray>(it->init());
        for (auto &init : *array) {
          os << INDENT << ".long" << TAB << dyn_cast<mid::ConstantInt>(init.value())->value() << std::endl;
        }
      } else {
        os << INDENT << ".long" << TAB << dyn_cast<mid::ConstantInt>(it->init())->value() << std::endl;
      }
    }
    os << std::endl;
  }
}

std::ostream &operator<<(std::ostream &os, const LLFunctionPtr &function) {
  auto func_name = function->function()->GetFunctionName();
  // 1. dump function name
  os << GLOBAL_LABEL << SPACE << func_name << std::endl;

  // 2. dump type
  os << INDENT << TYPE_LABEL << TWO_SPACE << func_name << "," << SPACE << FUNC_TYPE << std::endl;

  // 3. print label
  os << func_name << ":" << std::endl;
  for (const auto &block : function->blocks()) {
    os << block << std::endl;
  }

  // 4. insert literal pool
//  os << INDENT << ".pool" << std::endl;
  return os;
}

std::ostream &operator<<(std::ostream &os, const LLBlockPtr &block) {
  auto &npos = std::string::npos;
  auto name = block->name();

  if (name != "entry") {
    os << ".";
    if (name.find("if_cond") != npos) {
      os << name << id_mgr.GetId(block, IdType::_ID_LL_IF_COND);
    } else if (name.find("if.then") != npos) {
      os << "if_then" << id_mgr.GetId(block, IdType::_ID_LL_THEN);
    } else if (name.find("if.else") != npos) {
      os << "if_else" << id_mgr.GetId(block, IdType::_ID_LL_ELSE);
    } else if (name.find("if.end") != npos) {
      os << "if_end" << id_mgr.GetId(block, IdType::_ID_LL_IF_END);
    } else if (name.find("while.cond") != npos) {
      os << "while_cond" << id_mgr.GetId(block, IdType::_ID_LL_WHILE_COND);
    } else if (name.find("loop.body") != npos) {
      os << "loop_body" << id_mgr.GetId(block, IdType::_ID_LL_LOOP_BODY);
    } else if (name.find("while.end") != npos) {
      os << "while_end" << id_mgr.GetId(block, IdType::_ID_LL_WHILE_END);
    } else if (name.find("lhs.true") != npos) {
      os << "lhs_true" << id_mgr.GetId(block, IdType::_ID_LL_LHS_TRUE);
    } else if (name.find("lhs.false") != npos) {
      os << "lhs_false" << id_mgr.GetId(block, IdType::_ID_LL_LHS_FALSE);
    } else if (name.find("land.end") != npos) {
      os << "land_end" << id_mgr.GetId(block, IdType::_ID_LL_LAND_END);
    } else if (name.find("lor.end") != npos) {
      os << "lor_end" << id_mgr.GetId(block, IdType::_ID_LL_LOR_END);
    } else if (name.find("block") != npos) {
      os << "block" << id_mgr.GetId(block, IdType::_ID_LL_BLOCK);
    } else if (name.find("func_exit") != npos) {
      auto func_name = block->parent()->function()->GetFunctionName();
      os << func_name << "_exit";
    } else {
      os << name;
    }

    if (in_instruction) return os;
    os << ":" << std::endl;
  }

  for (const auto &inst : block->insts()) {
    os << inst << std::endl;
  }

  return os;
}

std::ostream &operator<<(std::ostream &os, const LLInstPtr &inst) {
  auto guard = InInst();
  if (auto mv_inst = dyn_cast<LLMove>(inst)) {
    // limit of ARM immediate number
    // https://stackoverflow.com/questions/10261300/invalid-constant-after-fixup
    if ((mv_inst->src()->state() == LLOperand::State::Immediate) &&
        !LLModule::can_encode_imm(mv_inst->src()->imm_num())) {
      auto imm = mv_inst->src()->imm_num();
      if ((uint32_t)imm >> 16u == 0) {
        os << INDENT << "movw" << TAB
           << mv_inst->dst() << ","<< SPACE <<"#" << (uint16_t)imm;
      } else {
        // wider than 16 bits
#if 0
        os << INDENT << "ldr" << TAB
           << mv_inst->dst() << "," << SPACE <<"=" << imm;
#endif
        os << INDENT << "mov" << TAB
           << mv_inst->dst() << "," << SPACE << "#" << (imm & 0xffff) << std::endl;
        os << INDENT << "movt" << TAB
           << mv_inst->dst() << "," << SPACE << "#" << (uint16_t (imm >> 16));
      }
    } else {
      os << INDENT << "mov" << mv_inst->cond() << TAB
         << mv_inst->dst() << "," << SPACE << mv_inst->src()
         << mv_inst->shift();
    }
  } else if (auto load_inst = dyn_cast<LLLoad>(inst)) {
    os << INDENT << "ldr" << TAB
       << load_inst->dst() << "," << SPACE
       << "[" << load_inst->addr();

    if (load_inst->offset() != nullptr) {
      os << "," << SPACE << load_inst->offset();
    }

    os << "]";
  } else if (auto jump_inst = dyn_cast<LLJump>(inst)) {
    os << INDENT << "b" << TAB
       << jump_inst->target() << std::endl;

    // TODO: insert literal pool
//    os << INDENT << ".pool";
  } else if (auto ret_inst = dyn_cast<LLReturn>(inst)) {
    os << INDENT << "bx" << TAB << ArmReg::lr;
    // TODO:
  } else if (auto store_inst = dyn_cast<LLStore>(inst)) {
    os << INDENT << "str" << TAB
       << store_inst->data() << "," << SPACE
       << "[" << store_inst->addr();

    if (store_inst->offset() != nullptr) {
      os << "," << SPACE << store_inst->offset();
    }

    os << "]";
  } else if (auto branch_inst = dyn_cast<LLBranch>(inst)) {
    os << INDENT << "b" << branch_inst->arm_cond() << TAB << branch_inst->true_block() << std::endl;
    if (branch_inst->need_out_false()) {
      os << INDENT << "b" << TAB << branch_inst->false_block() << std::endl;
    }

    // output literal pool
//    os << INDENT << ".pool";
  } else if (auto call_inst = dyn_cast<LLCall>(inst)) {
    os << INDENT << "bl" << TAB << call_inst->function()->GetFunctionName();
  } else if (auto binary_inst = dyn_cast<LLBinaryInst>(inst)) {
    std::string op = "unknown";
    auto opcode = binary_inst->opcode();
    switch (opcode) {
      case LLInst::Opcode::Add:  op = "add";  break;
      case LLInst::Opcode::Sub:  op = "sub";  break;
      case LLInst::Opcode::Mul:  op = "mul";  break;
      case LLInst::Opcode::SDiv: op = "sdiv"; break;
      case LLInst::Opcode::And:  op = "and";  break;
      case LLInst::Opcode::Or:   op = "orr";  break;
      case LLInst::Opcode::AShr: op = "asr";  break;
      case LLInst::Opcode::LShr: op = "lsr";  break;
      case LLInst::Opcode::Shl:  op = "lsl";  break;
      case LLInst::Opcode::Xor:  op = "eor";  break;
      default:
        ERROR("should not reach here");
    }

    os << INDENT << op << TAB << binary_inst->dst()
       << "," << SPACE << binary_inst->lhs()
       << "," << SPACE << binary_inst->rhs();

    if (!binary_inst->shift().is_none()) {
      os << "," << SPACE << binary_inst->shift();
    }
  } else if (auto cmp_inst = dyn_cast<LLCompare>(inst)) {
    os << INDENT << "cmp" << TAB;
    os << cmp_inst->lhs() << "," << SPACE << cmp_inst->rhs();
  } else if (auto mla_inst = dyn_cast<LLMLA>(inst)) {
    os << INDENT << "mla" << TAB << mla_inst->dst()
       << "," << SPACE << mla_inst->lhs()
       << "," << SPACE << mla_inst->rhs()
       << "," << SPACE << mla_inst->acc();
  } else if (auto mls_inst = dyn_cast<LLMLS>(inst)) {
    os << INDENT << "mls" << TAB << mls_inst->dst()
       << "," << SPACE << mls_inst->lhs()
       << "," << SPACE << mls_inst->rhs()
       << "," << SPACE << mls_inst->acc();
  } else if (auto push_inst = dyn_cast<LLPush>(inst)) {
    os << INDENT << "push" << TAB << "{ ";
    auto regs = push_inst->reg_list();
    for (std::size_t i = 0; i < regs.size(); i++) {
      os << regs[i];
      if (i != regs.size() - 1) os << SPACE << ",";
    }
    os << " }";
  } else if (auto pop_inst = dyn_cast<LLPop>(inst)) {
    os << INDENT << "pop" << TAB << "{ ";
    auto regs = pop_inst->reg_list();
    for (std::size_t i = 0; i < regs.size(); i++) {
      os << regs[i];
      if (i != regs.size() - 1) os << SPACE << ",";
    }
    os << " }";
  } else if (auto global_inst = dyn_cast<LLGlobal>(inst)) {
    os << INDENT << "movw" << TAB
       << global_inst->dst() << ","
       << SPACE << "#:lower16:" << global_inst->global_variable()->name();
    os << std::endl;
    os << INDENT << "movt" << TAB
       << global_inst->dst() << ","
       << SPACE << "#:upper16:" << global_inst->global_variable()->name();
  } else if (auto load_pseudo = dyn_cast<LLLoadPseudo>(inst)) {

    auto imm = load_pseudo->imm();
    if ((uint32_t)imm >> 16u == 0) {
      os << INDENT << "movw" << TAB
         << load_pseudo->dst() << ","<< SPACE <<"#" << (uint16_t)imm;
    } else {
      // wider than 16 bits
      os << INDENT << "mov" << TAB
         << load_pseudo->dst() << "," << SPACE << "#" << (imm & 0xffff) << std::endl;
      os << INDENT << "movt" << TAB
         << load_pseudo->dst() << "," << SPACE << "#" << (uint16_t (imm >> 16));
    }
  } else if (auto comment = dyn_cast<LLComment>(inst)) {
    os << '@' << SPACE << comment->comment();
  }

  return os;
}

std::ostream &operator<<(std::ostream &os, const LLOperandPtr &operand) {
  switch (operand->state()) {
    case LLOperand::State::RealReg: {
      os << operand->reg();
      break;
    }
    case LLOperand::State::Virtual: {
      os << "v" << operand->virtual_num();
      break;
    }
    case LLOperand::State::Immediate:
      os << "#" << operand->imm_num();
      break;
    default: break;
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const ArmReg armReg) {
  switch (armReg) {
    case ArmReg::r0:  os << "r0";  break;
    case ArmReg::r1:  os << "r1";  break;
    case ArmReg::r2:  os << "r2";  break;
    case ArmReg::r3:  os << "r3";  break;
    case ArmReg::r4:  os << "r4";  break;
    case ArmReg::r5:  os << "r5";  break;
    case ArmReg::r6:  os << "r6";  break;
    case ArmReg::r7:  os << "r7";  break;
    case ArmReg::r8:  os << "r8";  break;
    case ArmReg::r9:  os << "r9";  break;
    case ArmReg::r10: os << "r10"; break;
    case ArmReg::r11: os << "r11"; break;
    case ArmReg::r12: os << "r12"; break;
    case ArmReg::r13: os << "sp";  break;
    case ArmReg::r14: os << "lr";  break;
    case ArmReg::r15: os << "pc";  break;
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const ArmShift &shift) {
  switch (shift.type()) {
    case ArmShift::ShiftType::None: /* nil */ return os;
    case ArmShift::ShiftType::Asr: os << "asr"; break;
    case ArmShift::ShiftType::Lsl: os << "lsl"; break;
    case ArmShift::ShiftType::Lsr: os << "lsr"; break;
    case ArmShift::ShiftType::Ror: os << "ror"; break;
    case ArmShift::ShiftType::Rrx: os << "rrx"; break;
  }
  os << SPACE << "#" << shift.shift();
  return os;
}

std::ostream &operator<<(std::ostream &os, const ArmCond &cond) {
  switch (cond) {
    case ArmCond::Any: /* nil */  break;
    case ArmCond::Eq: os << "eq"; break;
    case ArmCond::Ne: os << "ne"; break;
    case ArmCond::Ge: os << "ge"; break;
    case ArmCond::Gt: os << "gt"; break;
    case ArmCond::Le: os << "le"; break;
    case ArmCond::Lt: os << "lt"; break;
  }
  return os;
}


}