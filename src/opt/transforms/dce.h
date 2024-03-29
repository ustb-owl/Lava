#ifndef LAVA_DCE_H
#define LAVA_DCE_H

#include "opt/pass.h"
#include "common/casting.h"
#include "opt/pass_manager.h"
#include "opt/analysis/funcanalysis.h"

/*
 aggressive dead code elimination

 use mark and sweep algorithm
 */

namespace lava::opt {
class DeadCodeElimination : public FunctionPass {
private:
  bool _changed;
  FuncInfoMap                 _func_infos;
  std::vector<User *>         _work_list;
  std::unordered_set<Value *> _critical_list;

  inline bool IsPureCall(const SSAPtr &value) {
    if (auto call_inst = dyn_cast<CallInst>(value)) {
      if (auto func = dyn_cast<Function>(call_inst->Callee())) {
        if (_func_infos[func.get()].IsPure()) {
          auto none_array_arg = std::none_of(call_inst->begin(), call_inst->end(),[](const Use &use) {
            return IsSSA<AccessInst>(use.value());
          });
          return none_array_arg;
        }
      }
    }
    return false;
  }

public:
  bool runOnFunction(const FuncPtr &F) final;

  void initialize() final {
    auto func_info = PassManager::GetAnalysis<FunctionInfoPass>("FunctionInfoPass");
    _func_infos = func_info->GetFunctionInfo();
  }

  void finalize() final {
    DBG_ASSERT(_work_list.empty(), "work list is not empty");
    _critical_list.clear();
  }

private:
  /*
   An operation is critical if:
   1. It sets return values for the function
   2. It is an input/output statement
   3. It affects the value which may be accessible from outside the current function
   */
  bool IsCriticalInstruction(const SSAPtr &ptr);

  /* Mark critical instructions */
  void Mark(const FuncPtr &F);

  void Sweep(const FuncPtr &F);
};


class DeadCodeEliminationFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<DeadCodeElimination>();
    auto passinfo =
        std::make_shared<PassInfo>(pass, "DeadCodeElimination", false, 0, DEAD_CODE_ELIMINATION);
    passinfo->Requires("FunctionInfoPass");
    return passinfo;
  }
};

}

#endif //LAVA_DCE_H
