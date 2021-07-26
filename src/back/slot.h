#ifndef LAVA_SLOT_H
#define LAVA_SLOT_H

#include "back/arch/arm/instdef.h"
#include <unordered_map>

namespace lava::back {

class SlotAllocator {
private:
  std::unordered_map<LLFunctionPtr, std::size_t> _slots;

public:

// void initialize

  // get a four-bytes slot
  LLOperandPtr AllocSlot(const LLFunctionPtr &func, const LLOperandPtr &operand);
};

}

#endif //LAVA_SLOT_H
