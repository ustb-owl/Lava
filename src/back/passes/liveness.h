#ifndef LAVA_LIVENESS_H
#define LAVA_LIVENESS_H

#include "pass.h"

namespace lava::back {

typedef std::unordered_set<LLOperandPtr> OprSet;

class BlockInfo {
public:
  BlockInfo() = default;
  BlockInfo(OprSet ue, OprSet kill, OprSet live) :
             ue_var(std::move(ue)), var_kill(std::move(kill)), live_out(std::move(live)) {}
  OprSet ue_var;
  OprSet var_kill;
  OprSet live_out;
};

class LivenessAnalysis : public PassBase {
private:
  std::list<LLBlockPtr>                       _rpo_blocks;
  std::unordered_set<LLBlockPtr>              _visited;
  std::unordered_map<LLBlockPtr, BlockInfo>   _blk_info;
  std::unordered_map<std::size_t, LLBlockPtr> _blk_map;

public:
  explicit LivenessAnalysis(LLModule &module) : PassBase(module) {}

  void Reset() final {
    _rpo_blocks.clear();
    _visited.clear();
    _blk_info.clear();
    _blk_map.clear();
  }

  // init gen and kill set
  void Init(const LLFunctionPtr &F);

  // traverse basic block in RPO
  void TraverseRPO(const LLBlockPtr &BB);

  // get successors of bb
  std::vector<LLBlockPtr> GetSuccessors(const LLBlockPtr &BB);

  // used for debug
  void DumpInitInfo();

  void SolveLiveness();

  void runOn(const LLFunctionPtr &func) final;
};

}

#endif //LAVA_LIVENESS_H
