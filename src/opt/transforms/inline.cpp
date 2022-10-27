#include <algorithm>

#include "opt/pass.h"
#include "lib/debug.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

int Inlining;

namespace lava::opt {

/*
 * Function Inlining
 * 1. copy a function
 * 2. insert the new function to its caller
 */
class FunctionInlining : public FunctionPass {
public:
  bool runOnFunction(const FuncPtr &F) final {
    bool changed = false;
    if (F->is_decl()) return changed;
    if (!Inlinable(F)) return changed;

//    CopyFunction(F);

    return changed;
  }

  // check if this function is inlinable
  bool Inlinable(const FuncPtr &F) const;

  // get the total instruction count of this function
  std::size_t GetInstCount(const FuncPtr &F) const;

  // copy the callee function
  FuncPtr CopyFunction(const FuncPtr &F) const;
};

std::size_t FunctionInlining::GetInstCount(const FuncPtr &F) const {
  std::size_t cnt = 0;
  for (const auto &it : *F) {
    auto blk = dyn_cast<BasicBlock>(it.value());
    cnt += blk->insts().size();
  }
  return cnt;
}

bool FunctionInlining::Inlinable(const FuncPtr &F) const {
  bool res = true;
  if (GetInstCount(F) > 50) res = false;

  return res;
}

FuncPtr FunctionInlining::CopyFunction(const FuncPtr &F) const {
  Module *module = F->getParent();
  std::string callee_name = F->GetFunctionName() + "_cpy";
  FuncPtr callee = module->CreateFunction(callee_name, F->type());
  callee->set_logger(F->logger());
  return callee;
}

class FunctionInliningFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<FunctionInlining>();
    auto passinfo =
        std::make_shared<PassInfo>(pass, "FunctionInlining", false, 2, FUNCTION_INLINING);
    return passinfo;
  }
};

static PassRegisterFactory<FunctionInliningFactory> registry;

}