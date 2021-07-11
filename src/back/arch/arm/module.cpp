#include "module.h"
#include "lib/guard.h"
#include "common/casting.h"
#include "common/idmanager.h"

#define TAB          "\t"
#define SPACE        " "
#define TWO_SPACE    "  "
#define INDENT       "\t\t"
#define TYPE_LABEL   ".type"
#define GLOBAL_LABEL ".global"
#define FUNC_TYPE    "%function"

namespace lava::back {

IdManager id_mgr;

void LLModule::reset() {
  _virtual_max = 0;
  _functions.clear();
  _glob_decl.clear();
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
    const auto &args = _insert_function->function()->args();
    LLOperandPtr arg = nullptr;
    for (std::size_t i = 0; i < args.size(); i++) {
      if (args[i] == value) {
        // copy from register
        if (i < 4) {
          // r0-r3
          arg = LLOperand::Register((ArmReg) i);
        } else {
          /* read from sp + (i-4)*4 in entry block */

          // mov rm, (i - 4) * 4
          auto offset = LLOperand::Virtual(_virtual_max);
          auto src = LLOperand::Immediate((i - 4) * 4);
          auto moveInst = AddInst<LLMove>(offset, src);

          // load rn, [sp, rm]
          auto vreg = LLOperand::Virtual(_virtual_max);
          auto addr = LLOperand::Register(ArmReg::sp);
          auto load_arg = AddInst<LLLoad>(vreg, addr, offset);
        }
        break;
      }
    }

    DBG_ASSERT(arg != nullptr, "create argument failed");
    return arg;
  } else if (auto constValue = dyn_cast<mid::ConstantInt>(value)) {
    return CreateImmediate(constValue->value());
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
      auto data = CreateOperand(storeInst->data());
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

      auto multiplier = dyn_cast<mid::ConstantInt>(gepInst->multiplier());
      DBG_ASSERT(multiplier != nullptr, "create multiplier in getelementptr instruction failed");

      // try to get const index
      auto constIdx = dyn_cast<mid::ConstantInt>(gepInst->index());

      AddInst<LLComment>("begin getelementptr");
      // 1. getelementptr ptr, 0, x || getelementptr ptr, x, 0
      // dst <- ptr
      if (multiplier->IsZero() || (constIdx && constIdx->IsZero())) {
        auto move_inst = AddInst<LLMove>(dst, ptr);
        DBG_ASSERT(move_inst != nullptr, "create LLMove failed in getelementptr instruction");
      } else if (constIdx) {
        // dst <- ptr + index * multiplier
        auto offset = constIdx->value() * multiplier->value();
        auto imm_operand = CreateImmediate(offset);

        auto add_inst = AddInst<LLBinaryInst>(LLInst::Opcode::Add, dst, ptr, imm_operand);
        DBG_ASSERT(add_inst != nullptr, "create add instruction failed in getelementptr instruction");
      } else if ((multiplier->value() & (multiplier->value() - 1)) == 0) {
        /* multiplier is 2^n */
        // dst <- ptr + (index << log(multiplier))
        auto index = CreateOperand(gepInst->index());
        auto add_inst = AddInst<LLBinaryInst>(LLInst::Opcode::Add, dst, ptr, index);
        DBG_ASSERT(add_inst != nullptr, "create add instruction failed in getelementptr instruction");
        ArmShift shift(__builtin_ctz(multiplier->value()), ArmShift::ShiftType::Lsl);
        add_inst->setShift(shift);
      } else {
        // dst <- ptr
        auto index = CreateOperand(gepInst->index());
        auto move_inst = AddInst<LLMove>(dst, ptr);

        // multiplier <- multiplier (imm)
        auto move_mult = AddInst<LLMove>(LLOperand::Virtual(_virtual_max), LLOperand::Immediate(multiplier->value()));

        // dst <- index * multiplier + dst
        auto mla = AddInst<LLMLA>(dst, index, move_mult->dst(), dst);
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

    } else if (auto branchInst = dyn_cast<mid::BranchInst>(inst)) {
      auto cond = CreateOperand(branchInst->cond());

      ArmCond armCond = ArmCond::Eq;
      DBG_ASSERT(_block_map.find(dyn_cast<mid::BasicBlock>(branchInst->true_block())) != _block_map.end(),
                 "can't find true block");
      auto true_block = _block_map[dyn_cast<mid::BasicBlock>(branchInst->true_block())];
      DBG_ASSERT(_block_map.find(dyn_cast<mid::BasicBlock>(branchInst->false_block())) != _block_map.end(),
                 "can't find false block");
      auto false_block = _block_map[dyn_cast<mid::BasicBlock>(branchInst->false_block())];

      auto ll_branch = AddInst<LLBranch>(armCond, cond, true_block, false_block);
    } else if (auto callInst = dyn_cast<mid::CallInst>(inst)) {
      std::vector<LLOperandPtr> params;

      // create parameters
      std::size_t param_size = callInst->param_size();
      for (int i = 0; i < param_size; i++) {
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

          // addr
          auto addr = LLOperand::Register(ArmReg::sp);

          // offset
          auto offset = LLOperand::Immediate(-((param_size - i) * 4));
          auto st_inst = AddInst<LLStore>(data, addr, offset);
          DBG_ASSERT(st_inst != nullptr, "store parameter(%d) failed in calling %s",
                     i, dyn_cast<mid::Function>(callInst->Callee())->GetFunctionName().c_str());
        }
      }

      // create stack for callee
      if (param_size > 4) {
        // sub sp, sp, (n - 4) * 4
        auto dst = LLOperand::Register(ArmReg::sp);
        auto lhs = LLOperand::Register(ArmReg::sp);
        auto rhs = LLOperand::Immediate(4 * (param_size - 4));
        auto sub_inst = AddInst<LLBinaryInst>(LLInst::Opcode::Sub, dst, lhs, rhs);
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
        auto rhs = LLOperand::Immediate(4 * (param_size - 4));
        auto sub_inst = AddInst<LLBinaryInst>(LLInst::Opcode::Add, dst, lhs, rhs);
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
      if (allocaInst->type()->IsArray()) {
        size = allocaInst->type()->GetSize();
      }

      // get address on stack
      auto dst = CreateOperand(inst);
      auto offset = CreateImmediate(_insert_function->stack_size());
      auto sp = LLOperand::Register(ArmReg::sp);
      auto add_inst = AddInst<LLBinaryInst>(LLInst::Opcode::Add, dst, sp, offset);

      // allocate size on sp
      _insert_function->SetStackSize(_insert_function->stack_size() + size);

    } else {
      // TODO:
    }
  }

  return ll_block;
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

  std::string indent = "\t";
  // dump function
  for (const auto &function : _functions) {
    os << function << std::endl;
    id_mgr.Reset();
  }
}

std::ostream &operator<<(std::ostream &os, const LLFunctionPtr &function) {
  auto func_name = function->function()->GetFunctionName();
  // 1. dump function name
  os << GLOBAL_LABEL << SPACE << func_name << std::endl;

  // 2. dump type
  os << INDENT << TYPE_LABEL << TWO_SPACE << func_name << "," << SPACE << FUNC_TYPE << std::endl;

  for (const auto &block : function->blocks()) {
    os << block << std::endl;
  }

  return os;
}

std::ostream &operator<<(std::ostream &os, const LLBlockPtr &block) {
  auto &npos = std::string::npos;
  auto name = block->name();

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
  } else {
    os << name;
  }

  if (in_instruction) return os;
  os << ":" << std::endl;

  for (const auto &inst : block->insts()) {
    os << inst << std::endl;
  }

  return os;
}

std::ostream &operator<<(std::ostream &os, const LLInstPtr &inst) {
  auto guard = InInst();
  if (auto mv_inst = dyn_cast<LLMove>(inst)) {
    os << INDENT << "mov" << TAB
       << mv_inst->dst() << "," << SPACE << mv_inst->src()
       << mv_inst->shift();
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
       << jump_inst->target();
  } else if (auto ret_inst = dyn_cast<LLReturn>(inst)) {
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
    os << INDENT << "beq" << TAB << branch_inst->true_block() << std::endl;
    os << INDENT << "b" << TAB << branch_inst->false_block();
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
      default:
        ERROR("should not reach here");
    }

    os << INDENT << op << TAB << binary_inst->dst()
       << "," << SPACE << binary_inst->lhs()
       << "," << SPACE << binary_inst->rhs();

    if (!binary_inst->shift().is_none()) {
      os << "," << SPACE << binary_inst->shift();
    }
  }

  return os;
}

std::ostream &operator<<(std::ostream &os, const LLOperandPtr &operand) {
  switch (operand->state()) {
    case LLOperand::State::RealReg: {
      os << operand->reg();
      break;
    }
    case LLOperand::State::Allocated: {
      break;
    }
    case LLOperand::State::Virtual: {
      os << "v" << operand->virtual_num();
      break;
    }
    case LLOperand::State::Immediate:
      os << "#" << operand->imm_num();
      break;
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
  return os;
}


}