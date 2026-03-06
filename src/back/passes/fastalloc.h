#ifndef LAVA_FASTALLOC_H
#define LAVA_FASTALLOC_H

#include <queue>

#include "back/slot.h"
#include "pass.h"

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

} // namespace lava::back

#endif // LAVA_FASTALLOC_H
