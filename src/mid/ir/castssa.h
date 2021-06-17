#ifndef XY_LANG_CASTSSA_H
#define XY_LANG_CASTSSA_H

#include "mid/ir/ssa.h"

using namespace lava::mid;

namespace lava {
template <typename SSA>
std::shared_ptr<SSA> dyn_cast(const SSAPtr &ssa) {
  if (SSA::classof(ssa.get())) {
    auto ptr = std::static_pointer_cast<SSA>(ssa);
    DBG_ASSERT(ptr != nullptr, "cast SSA failed");
    return ptr;
  } else {
    DBG_ASSERT(0, "cast SSA failed");
    return nullptr;
  }
}

template <typename SSA>
std::shared_ptr<SSA> dyn_cast(SSAPtr &ssa) {
  if (SSA::classof(ssa.get())) {
    auto ptr = std::static_pointer_cast<SSA>(ssa);
    DBG_ASSERT(ptr != nullptr, "cast SSA failed");
    return ptr;
  } else {
    DBG_ASSERT(0, "cast SSA failed");
    return nullptr;
  }
}

}

#endif //XY_LANG_CASTSSA_H
