#ifndef LAVA_DOMINANCEBASE_H
#define LAVA_DOMINANCEBASE_H

#include "opt/pass.h"
#include "opt/blkwalker.h"
#include "opt/pass_manager.h"
#include "lib/bitvec.h"
#include "common/casting.h"

namespace lava::opt {

// dominance information of each function
struct DominanceResult {
  // dominators set of basic blocks
  std::unordered_map<mid::BasicBlock *, std::unordered_set<mid::BasicBlock * >> domBy;

  // immediate dominator
  std::unordered_map<mid::BasicBlock *, mid::BasicBlock *> idom;

  // dominatees set of basic blocks
  std::unordered_map<mid::BasicBlock *, std::unordered_set<mid::BasicBlock * >> doms;

  std::unordered_map<mid::BasicBlock *, std::unordered_set<mid::BasicBlock * >> DF;

  std::unordered_map<mid::BasicBlock *, uint32_t> depth;
};

typedef std::unordered_map<mid::User *, DominanceResult> DomInfo;

class DominanceBase : public FunctionPass {
protected:
  bool _changed;
  Function *_cur_func;
  BlockWalker _blkWalker;

  virtual void SolveDominance(const FuncPtr &F) = 0;

  // check if dominator strictly dominates BB
  bool IsStrictlyDom(BasicBlock *dominator, BasicBlock *BB);

  // get strictly dominators of a block
  std::unordered_set<mid::BasicBlock *> GetSDoms(BasicBlock *BB);

  // solve the immediate dominator of each block
  virtual void SolveImmediateDom() = 0;

  // solve the dominance frontier of each block
  virtual void SolveDominanceFrontier() = 0;

  // dominance information for each function
  DomInfo _dom_info;

public:
  const DomInfo &
  GetDomInfo() const {
    return _dom_info;
  }

};

}
#endif //LAVA_DOMINANCEBASE_H
