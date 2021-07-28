#ifndef LAVA_BLKWALKER_H
#define LAVA_BLKWALKER_H

#include <list>
#include <unordered_set>

#include "mid/ir/ssa.h"
#include "common/casting.h"

using namespace lava::mid;

namespace lava::opt {

class BlockWalker {
private:
  std::list<BasicBlock *>          _rpo;
  std::list<BasicBlock *>          _po;
  std::unordered_set<BasicBlock *> _visited;

  // reverse post order
  void TraverseRPO(BasicBlock *BB);

  // post order
  void TraversePO(BasicBlock *BB);
public:
  BlockWalker() = default;

  void init() {
    _rpo.clear();
    _po.clear();
    _visited.clear();
  }

  BasicBlock *GetExitBlock(const std::list<BasicBlock *>& bb_list);

  std::list<BasicBlock *> RPOTraverse(BasicBlock *entry) {
    init();
    TraverseRPO(entry);
    return _rpo;
  }

  std::list<BasicBlock *> POTraverse(BasicBlock *entry) {
    init();
    TraversePO(entry);
    return _po;
  }


};

}

#endif //LAVA_BLKWALKER_H
