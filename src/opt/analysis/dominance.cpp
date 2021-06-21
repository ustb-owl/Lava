#include "dominance.h"

int Dominance;

namespace lava::opt {

void DominanceInfo::SolveDominance(const FuncPtr &F) {
  auto &info = _dom_info[F.get()];
  auto entry = dyn_cast<BasicBlock>(F->entry());
  auto rpo = _blkWalker.RPOTraverse(entry.get());

  // init the dom set
  // set entry -> { entry }
  info.dom[entry.get()] = {entry.get()};

  // store all basic blocks except entry
  std::unordered_set<BasicBlock *> all;

  for (auto &it : rpo) {
    all.insert(it);
  }

  // set Dom(i) <- all
  auto it = rpo.begin();
  for (it++; it != rpo.end(); it++) {
    info.dom[*it] = all;
  }

  bool changed = true;
  while (true) {
    changed = false;

    // traverse all block except entry
    auto BB = rpo.begin();
    for (BB++; BB != rpo.end(); BB++) {
      auto block = *BB;
      // validate if the dominator is exist in all of its predecessors
      for (auto dominator = info.dom[block].begin(); dominator != info.dom[block].end();) {
        if (*dominator != block &&
            std::any_of((*BB)->begin(), (*BB)->end(), [&dominator, &info](Use &BB) {
              auto pred = dyn_cast<BasicBlock>(BB.get()).get();
              return info.dom[pred].find(*dominator) == info.dom[pred].end();
            })) {
          changed = true;
          dominator = info.dom[*BB].erase(dominator);
        } else {
          dominator++;
        }
      }
    }
    if (!changed) break;
  }


}


static PassRegisterFactory<DominanceInfoPassFactory> registry;

}