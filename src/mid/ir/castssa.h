#ifndef XY_LANG_CASTSSA_H
#define XY_LANG_CASTSSA_H

#include "mid/ir/ssa.h"

using namespace lava::mid;

namespace lava {
template <typename SSA>
std::shared_ptr<SSA> dyn_cast(const SSAPtr &ssa) {
  if (ssa && SSA::classof(ssa.get())) {
    auto ptr = std::static_pointer_cast<SSA>(ssa);
    DBG_ASSERT(ptr != nullptr, "cast SSA failed");
    return ptr;
  } else {
    return nullptr;
  }
}

template <typename SSA>
std::shared_ptr<SSA> dyn_cast(SSAPtr &ssa) {
  if (ssa && SSA::classof(ssa.get())) {
    auto ptr = std::static_pointer_cast<SSA>(ssa);
    DBG_ASSERT(ptr != nullptr, "cast SSA failed");
    return ptr;
  } else {
    return nullptr;
  }
}

template <typename SSA>
bool IsSSA(const SSAPtr &ssa) {
  DBG_ASSERT(ssa != nullptr, "SSA is nullptr");
  if (SSA::classof(ssa.get())) {
    return true;
  } else {
    return false;
  }
}

template <typename SSA>
bool IsSSA(SSAPtr &ssa) {
  DBG_ASSERT(ssa != nullptr, "SSA is nullptr");
  if (SSA::classof(ssa.get())) {
    return true;
  } else {
    return false;
  }
}

}

#endif //XY_LANG_CASTSSA_H
