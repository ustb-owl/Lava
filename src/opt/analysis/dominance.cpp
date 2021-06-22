#include "dominance.h"

int Dominance;

namespace lava::opt {

bool DominanceInfo::IsStrictlyDom(BasicBlock *dominator, BasicBlock *BB) {
  /* A node d strictly dominates a node n if d dominates n and d does not equal n. */
  if (dominator != BB) {
    return _dom_info[_cur_func].domBy[BB].find(dominator) != _dom_info[_cur_func].domBy[BB].end();
  }
  return false;
}

std::unordered_set<mid::BasicBlock *> DominanceInfo::GetSDoms(BasicBlock *BB) {
  auto doms = _dom_info[_cur_func].domBy[BB];
  doms.erase(BB);
  return doms;
}

void DominanceInfo::SolveDominance(const FuncPtr &F) {
  _cur_func = F.get();
  auto &info = _dom_info[_cur_func];
  auto entry = dyn_cast<BasicBlock>(F->entry());
  auto rpo = _blkWalker.RPOTraverse(entry.get());

  // init the dom set
  // set entry -> { entry }
  info.domBy[entry.get()] = {entry.get()};

  // store all basic blocks except entry
  std::unordered_set<BasicBlock *> all;

  for (auto &it : rpo) {
    all.insert(it);
  }

  // set Dom(i) <- all
  auto it = rpo.begin();
  for (it++; it != rpo.end(); it++) {
    info.domBy[*it] = all;
  }

  // calculate the dominators of each basic block
  bool changed = true;
  while (true) {
    changed = false;

    // traverse all block except entry
    auto BB = rpo.begin();
    for (BB++; BB != rpo.end(); BB++) {
      auto block = *BB;
      // validate if the dominator is exist in all of its predecessors
      for (auto dominator = info.domBy[block].begin(); dominator != info.domBy[block].end();) {
        if (*dominator != block &&
            std::any_of((*BB)->begin(), (*BB)->end(), [&dominator, &info](Use &BB) {
              auto pred = dyn_cast<BasicBlock>(BB.get()).get();
              return info.domBy[pred].find(*dominator) == info.domBy[pred].end();
            })) {
          changed = true;
          dominator = info.domBy[*BB].erase(dominator);
        } else {
          dominator++;
        }
      }
    }
    if (!changed) break;
  }

  SolveImmediateDom();
}

/*
 * The immediate dominator or idom of a node n is the unique node that strictly
 * dominates n but does not strictly dominate any other node that strictly dominates n.
 * Every node, except the entry node, has an immediate dominator.
 */
void DominanceInfo::SolveImmediateDom() {
  auto &info = _dom_info[_cur_func];
  auto entry = dyn_cast<BasicBlock>(_cur_func->entry()).get();
  auto rpo = _blkWalker.RPOTraverse(entry);

  // insert entry itself into its dominatee set
  info.doms[entry].insert(entry);

  auto it = rpo.begin();
  for (it++; it != rpo.end(); it++) {
    BasicBlock *BB = *it;

    // insert this BB into entry block's dominatees set
    info.doms[entry].insert(BB);

    // traverse BB's dominators
    for (BasicBlock *dominator : info.domBy[BB]) {
      // insert BB into its dominators dominatees set
      info.doms[dominator].insert(BB);

      // 1. check if dominator is strictly dominate BB
      if (dominator == BB) continue;

      // 2. check if dominator does not strictly dominate any other node
      // that strictly dominates BB
      auto sdoms = GetSDoms(BB);
      if (std::any_of(sdoms.begin(), sdoms.end(), [dominator, this] (BasicBlock *sdominator) {
        return IsStrictlyDom(dominator, sdominator);
      })) { continue; }

      // set dominator as BB's immediate dominator
      DBG_ASSERT(info.idom.find(BB) == info.idom.end(), "block already has a idom");
      info.idom[BB] = dominator;
    }
  }
}


static PassRegisterFactory<DominanceInfoPassFactory> registry;

}