#include "opt/pass.h"
#include "mid/ir/castssa.h"
#include "opt/pass_manager.h"

#include "opt/analysis/postdominance.h"

int DeadCodeElimination;

namespace lava::opt {

/*
 aggressive dead code elimination

 use mark and sweep algorithm
 */
class DeadCodeElimination : public FunctionPass {
private:
  bool _changed;

public:
  bool runOnFunction(const FuncPtr &F) final {
    _changed = false;

    auto AA = PassManager::GetAnalysis<PostDominanceInfo>("PostDominanceInfo");

    auto domInfo = AA->GetDomInfo();
    return _changed;
  }

private:

  /*
   An operation is critical if:
   1. It sets return values for the function
   2. It is an input/output statement
   3. It affects the value which may be accessible from outside the current function
   */
  bool IsCriticalInstruction(const SSAPtr &ptr) {
    if (auto inst = dyn_cast<Instruction>(ptr)) {
      if (IsSSA<JumpInst>(inst) || IsSSA<ReturnInst>(inst) ||
          IsSSA<BranchInst>(inst) || IsSSA<CallInst>(inst)) {
        return true;
      } else if (IsSSA<StoreInst>(inst)) {
        // TODO: check if is global
        // TODO: how to check array
        return true;
      }
    }
    return false;
  }
};

class DeadCodeEliminationFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<DeadCodeElimination>();
    auto passinfo =
        std::make_shared<PassInfo>(pass, "DeadCodeElimination", false, false, DEAD_CODE_ELIMINATION);

    // add requires pass
//    passinfo->Requires("PostDominanceInfo");
    return passinfo;
  }
};

static PassRegisterFactory<DeadCodeEliminationFactory> registry;

}