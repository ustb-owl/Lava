#include "codegen.h"
#include "common/casting.h"

namespace lava::back{


void CodeGenerator::CodeGene() {
  for (const auto &func : _module->Functions()) {
    auto ll_function = _ll_module.CreateFunction(func);
    DBG_ASSERT(ll_function != nullptr, "create low-level function failed");
    for (const auto &it : *func) {
      auto ll_block = _ll_module.CreateBasicBlock(dyn_cast<mid::BasicBlock>(it.value()), ll_function);
      DBG_ASSERT(ll_block != nullptr, "create low-level block failed");
    }
  }
}

}
