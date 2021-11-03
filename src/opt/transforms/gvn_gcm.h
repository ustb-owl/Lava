#ifndef LAVA_GVN_GCM_H
#define LAVA_GVN_GCM_H

#include <algorithm>
#include "opt/pass.h"
#include "opt/blkwalker.h"
#include "common/casting.h"
#include "opt/pass_manager.h"
#include "opt/transforms/dce.h"
#include "opt/analysis/identiy.h"
#include "opt/analysis/loopinfo.h"
#include "opt/analysis/dominance.h"
#include "opt/analysis/funcanalysis.h"

namespace lava::opt {
using ValueNumber = std::vector<std::pair<SSAPtr, SSAPtr>>;

/*
 * Global value numbering and global code motion
 * TODO: swapOperand or FoldBinary would cause int_literal WA
 */
class GlobalValueNumberingGlobalCodeMotion : public FunctionPass {
private:
//  int                 _cnt = 0;
  bool               _changed;
  BlockWalker        _blkWalker;
  ValueNumber        _value_number;
  FuncInfoMap        _func_infos;
  DomInfo            _dom_info;
  LoopInfo           _loop_info;
  Function          *_cur_func;

  std::unordered_set<Instruction *> _visited;
  std::unordered_map<Instruction *, InstPtr> _user_map;
  std::unordered_map<Instruction *, BasicBlock *> _inst_block_map;

  inline bool IsPure(const SSAPtr &value) {
    if (auto call_inst = dyn_cast<CallInst>(value)) {
      if (auto func = dyn_cast<Function>(call_inst->Callee())) {
        return _func_infos[func.get()].IsPure();
      }
    }
    return false;
  }

public:

  bool runOnFunction(const FuncPtr &F) final;

  void initialize() final;

  void finalize() final;

  void Replace(const InstPtr &inst, const SSAPtr &value, BasicBlock *block, SSAPtrList::iterator it);

  SSAPtr FindValue(const std::shared_ptr<BinaryOperator> &binary_inst);

  SSAPtr FindValue(const std::shared_ptr<AccessInst> &access_inst);

  SSAPtr FindValue(const std::shared_ptr<CallInst> &call_inst);

  SSAPtr FindValue(const std::shared_ptr<ICmpInst> &icmp_inst);

  SSAPtr ValueOf(const SSAPtr &value);

  void GlobalValueNumbering(const FuncPtr &F);

  void CollectInstBlockMap(const FuncPtr &F);

  void TransferInst(const InstPtr &inst, BasicBlock *new_block);

  void ScheduleOp(BasicBlock *entry, const InstPtr &I, const SSAPtr &operand);

  void ScheduleEarly(BasicBlock *entry, const InstPtr &inst);

  BasicBlock *FindLCA(BasicBlock *a, BasicBlock *b);

  void ScheduleLate(const InstPtr &inst);

};

class GlobalValueNumberingGlobalCodeMotionFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<GlobalValueNumberingGlobalCodeMotion>();
    auto passinfo = std::make_shared<PassInfo>(pass, "GlobalValueNumberingGlobalCodeMotion", false, 2, GVN_GCM);
//    passinfo->Requires("FunctionInfoPass");
    passinfo->Requires("LoopInfoPass");
    return passinfo;
  }
};
}

#endif //LAVA_GVN_GCM_H
