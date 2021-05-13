#ifndef XY_LANG_CASTSSA_H
#define XY_LANG_CASTSSA_H

#include "mid/ir/ssa.h"

using namespace lava::mid;

namespace lava {
template <typename SSA>
auto CastTo(const SSAPtr &ssa) {
  auto ptr = std::static_pointer_cast<SSA>(ssa);
  DBG_ASSERT(ptr != nullptr, "Cast SSA failed");
  return ptr;
}

}

#endif //XY_LANG_CASTSSA_H
