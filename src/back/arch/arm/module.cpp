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
    if (auto jumpInst = dyn_cast<mid::JumpInst>(inst)) {
      auto target = _block_map[dyn_cast<mid::BasicBlock>(jumpInst->target())];
      DBG_ASSERT(target != nullptr, "can't find target block in JumpInst");

      auto ll_jumpInst = AddInst<LLJump>(target);
      DBG_ASSERT(ll_jumpInst != nullptr, "create LLJump failed");
    } else if (auto loadInst = dyn_cast<mid::LoadInst>(inst)){

      // solve operand
      auto src = CreateOperand(inst->GetAddr());

      auto dst = CreateOperand(loadInst);
      auto ll_loadInst = AddInst<LLLoad>(dst, src, nullptr);
      DBG_ASSERT(ll_loadInst != nullptr, "create LLLoad failed");
    }
  }

  return ll_block;
}

}