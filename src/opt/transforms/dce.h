#ifndef LAVA_DCE_H
#define LAVA_DCE_H

#include "opt/pass.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

/*
 aggressive dead code elimination

 use mark and sweep algorithm
 */

namespace lava::opt {
class DeadCodeElimination : public FunctionPass {
private:
  bool _changed;
  std::vector<User *> _work_list;
  std::unordered_set<Value *> _critical_list;

public:
  bool runOnFunction(const FuncPtr &F) final;

  void initialize() final {}

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

    return passinfo;
  }
};

}

#endif //LAVA_DCE_H
