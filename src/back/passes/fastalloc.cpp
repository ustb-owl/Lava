#include "fastalloc.h"
#include "common/casting.h"
#include "back/arch/arm/instdef.h"

namespace lava::back {

void FastAllocation::Reset() {
  while (!_reg.empty()) _reg.pop();
  for (int reg = (int) ArmReg::r4; reg < (int) ArmReg::r10; reg++) {
    _reg.push(static_cast<ArmReg>(reg));
  }
  _allocated.clear();
}

void FastAllocation::runOn(const LLFunctionPtr &func) {
  for (const auto &block : func->blocks()) {
    for (const auto &inst : block->insts()) {
      for (const auto &opr : inst->operands()) {
        if ((opr == nullptr) || (opr->state() != LLOperand::State::Virtual)) continue;
        if (opr->allocated() != nullptr) continue;
        auto slot = _slot.AllocSlot(func, opr);
        DBG_ASSERT(slot != nullptr, "operand spill to slot failed");
        _allocated[opr] = slot;
      }

      auto dest = inst->dest();
      if (dest && dest->IsVirtual()) {
        auto slot = _slot.AllocSlot(func, dest);
        DBG_ASSERT(slot != nullptr, "dst spill to slot failed");
        _allocated[dest] = slot;
      }

    }
  }
}

}