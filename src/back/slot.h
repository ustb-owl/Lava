#ifndef LAVA_SLOT_H
#define LAVA_SLOT_H

#include <unordered_map>

#include "back/arch/arm/instdef.h"

namespace lava::back {

class SlotAllocator {
private:
  std::unordered_map<LLFunctionPtr, std::size_t> _slots;

public:
  // void initialize

  // get a four-bytes slot
  LLOperandPtr AllocSlot(const LLFunctionPtr &func,
                         const LLOperandPtr  &operand);
};

} // namespace lava::back

#endif // LAVA_SLOT_H
