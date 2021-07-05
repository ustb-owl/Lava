#include "module.h"
#include "common/casting.h"

namespace lava::back {

void LLModule::reset() {
  _virtual_max = 0;
  _functions.clear();
  _glob_decl.clear();
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
          arg = LLOperand::Register((ArmReg)i);
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

    // insert block into _block_map
    _block_map[block] = ll_block;
  }

  return ll_function;
}

LLBlockPtr LLModule::CreateBasicBlock(const mid::BlockPtr &block, const LLFunctionPtr& parent) {
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
    } else if (auto loadInst = dyn_cast<mid::LoadInst>(inst)){
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
    }
  }

  return ll_block;
}

}