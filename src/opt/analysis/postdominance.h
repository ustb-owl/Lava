#ifndef LAVA_POSTDOMINANCE_H
#define LAVA_POSTDOMINANCE_H

#include "dominancebase.h"

namespace lava::opt {

/*
 Calculate the dominance information of basic blocks
 */
class PostDominanceInfo : public DominanceBase {
private:
  void SolveDominance(const FuncPtr &F) final;

  // solve the immediate dominator of each block
  void SolveImmediateDom() final;

  // solve the dominance frontier of each block
  void SolveDominanceFrontier() final;


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

class PostDominanceInfoPassFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<PostDominanceInfo>();
    auto passinfo =  std::make_shared<PassInfo>(pass, "PostDominanceInfo", true, false, POST_DOMINANCE_INFO);
    return passinfo;
  }
};

}

#endif //LAVA_POSTDOMINANCE_H
