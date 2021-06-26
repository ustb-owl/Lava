#include "dominancebase.h"

namespace lava::opt {

bool DominanceBase::IsStrictlyDom(BasicBlock *dominator, BasicBlock *BB) {
  /* A node d strictly dominates a node n if d dominates n and d does not equal n. */
  if (dominator != BB) {
    return _dom_info[_cur_func].domBy[BB].find(dominator) != _dom_info[_cur_func].domBy[BB].end();
  }
  return false;
}

std::unordered_set<mid::BasicBlock *> DominanceBase::GetSDoms(BasicBlock *BB) {
  auto doms = _dom_info[_cur_func].domBy[BB];
  doms.erase(BB);
  return doms;
}

}
