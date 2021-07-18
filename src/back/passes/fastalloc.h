#ifndef LAVA_FASTALLOC_H
#define LAVA_FASTALLOC_H

#include "pass.h"
#include "back/slot.h"
#include <queue>

namespace lava::back {

class FastAllocation : public PassBase {
private:
  SlotAllocator                                  _slot;
  std::queue<ArmReg>                             _reg;
  std::unordered_map<LLOperandPtr, LLOperandPtr> _allocated;

public:
  explicit FastAllocation(LLModule &module) : PassBase(module) {}

  void Reset() final;

  void runOn(const LLFunctionPtr &func) final;
};

}

#endif //LAVA_FASTALLOC_H
