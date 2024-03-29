#ifndef LAVA_DOMINANCE_H
#define LAVA_DOMINANCE_H

#include "dominancebase.h"

namespace lava::opt {

/*
 Calculate the dominance information of basic blocks
 */
class DominanceInfo : public DominanceBase {
private:
  void SolveDominance(const FuncPtr &F) final;

  // solve the immediate dominator of each block
  void SolveImmediateDom() final;

  // solve the dominance frontier of each block
  void SolveDominanceFrontier() final;

  void SolveDepth(BasicBlock *BB, uint32_t depth);

public:
  bool runOnFunction(const FuncPtr &F) final {
    _changed = false;
    if (F->is_decl()) return _changed;

    SolveDominance(F);

    return _changed;
  }

  void initialize() final {
    _cur_func = nullptr;
    _dom_info.clear();
  }

};

class DominanceInfoPassFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<DominanceInfo>();
    auto passinfo =  std::make_shared<PassInfo>(pass, "DominanceInfo", true, 0, DOMINANCE_INFO);
    return passinfo;
  }
};

}

#endif //LAVA_DOMINANCE_H
