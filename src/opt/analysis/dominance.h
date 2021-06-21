#ifndef LAVA_DOMINANCE_H
#define LAVA_DOMINANCE_H

#include "opt/pass.h"
#include "opt/blkwalker.h"
#include "opt/pass_manager.h"
#include "lib/bitvec.h"
#include "mid/ir/castssa.h"

namespace lava::opt {

/*
 Calculate the dominance information of basic blocks
 */
class DominanceInfo : public FunctionPass {
private:
  bool _changed;
  BlockWalker _blkWalker;

  void SolveDominance(const FuncPtr &F);

  // dominance information of each function
  struct DominanceResult {
    // dominance set of basic blocks
    std::unordered_map<mid::BasicBlock *, std::unordered_set<mid::BasicBlock *>> dom;
  };

  std::unordered_map<mid::User *, DominanceResult> _dom_info;

public:
  bool runOnFunction(const FuncPtr &F) final {
    _changed = false;
    if (F->is_decl()) return _changed;

    SolveDominance(F);

    return _changed;
  }

  void initialize() final {
    _dom_info.clear();
  }

};

class DominanceInfoPassFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<DominanceInfo>();
    auto passinfo =  std::make_shared<PassInfo>(pass, "DominanceInfo", true, false, DOMINANCE_INFO);
    return passinfo;
  }
};

}

#endif //LAVA_DOMINANCE_H
