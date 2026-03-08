#include "dominancebase.h"

namespace lava::opt {

bool DominanceBase::IsStrictlyDom(BasicBlock *dominator, BasicBlock *BB) {
  /* A node d strictly dominates a node n if d dominates n and d does not equal
   * n. */
  const auto &dom_info = GetDomInfo();
  if (dominator != BB) {
    return dom_info.at(_cur_func).domBy.at(BB).find(dominator) !=
           dom_info.at(_cur_func).domBy.at(BB).end();
  }
  return false;
}

std::unordered_set<mid::BasicBlock *> DominanceBase::GetSDoms(BasicBlock *BB) {
  auto doms = GetDomInfo().at(_cur_func).domBy.at(BB);
  doms.erase(BB);
  return doms;
}

} // namespace lava::opt
