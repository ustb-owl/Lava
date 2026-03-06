#ifndef LAVA_LOOP_INVARIANT_HOIST_H
#define LAVA_LOOP_INVARIANT_HOIST_H

#include "opt/analysis/dominance.h"
#include "opt/analysis/funcanalysis.h"
#include "opt/analysis/loopinfo.h"
#include "opt/pass.h"
#include "opt/pass_manager.h"

namespace lava::opt {

class LoopInvariantHoist : public FunctionPass {
private:
  bool        _changed = false;
  FuncInfoMap _func_infos;
  DomInfo     _dom_info;
  LoopInfo    _loop_info;
  Function   *_cur_func = nullptr;

  bool IsPureCall(const SSAPtr &value) const;

  bool IsHoistableInstruction(const InstPtr &inst) const;

  bool IsInLoop(BasicBlock *BB, const Loop *loop) const;

  bool DominatesPreheader(const InstPtr &inst, BasicBlock *preheader) const;

  BasicBlock *GetLoopPreheader(const Loop *loop) const;

  bool HoistInstructionIfInvariant(const InstPtr &inst, const Loop *loop,
                                   BasicBlock                        *preheader,
                                   std::unordered_set<Instruction *> &hoisted,
                                   std::unordered_set<Instruction *> &active);

public:
  bool runOnFunction(const FuncPtr &F) final;

  void initialize() final;

  void finalize() final;
};

class LoopInvariantHoistFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass     = std::make_shared<LoopInvariantHoist>();
    auto passinfo = std::make_shared<PassInfo>(pass, "LoopInvariantHoist",
                                               false, 2, LOOP_INVARIANT_HOIST);
    passinfo->Requires("FunctionInfoPass");
    passinfo->Requires("LoopInfoPass");
    passinfo->Requires("DominanceInfo");
    return passinfo;
  }
};

} // namespace lava::opt

#endif // LAVA_LOOP_INVARIANT_HOIST_H
