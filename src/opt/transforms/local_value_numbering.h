#ifndef LAVA_LOCAL_VALUE_NUMBERING_H
#define LAVA_LOCAL_VALUE_NUMBERING_H

#include <algorithm>

#include "common/casting.h"
#include "opt/analysis/funcanalysis.h"
#include "opt/blkwalker.h"
#include "opt/pass.h"
#include "opt/pass_manager.h"

namespace lava::opt {
// using ValueNumber = std::vector<std::pair<SSAPtr, SSAPtr>>;
using ValueNumber = std::unordered_map<SSAPtr, SSAPtr>;

class LocalValueNumbering : public FunctionPass {
private:
  bool        _changed;
  BlockWalker _blkWalker;
  ValueNumber _value_number;
  BasicBlock *_cur_block = nullptr;

  const FuncInfoMap &FunctionInfos() const;

  inline bool IsPureCall(const SSAPtr &value) {
    if (auto call_inst = dyn_cast<CallInst>(value)) {
      auto func = call_inst->Callee();
      auto it   = FunctionInfos().find(func.get());
      if (it != FunctionInfos().end() && it->second.IsPure()) {
        auto none_array_arg = std::none_of(
            call_inst->begin(), call_inst->end(),
            [](const Use &use) { return IsSSA<AccessInst>(use.value()); });
        return none_array_arg;
      }
    }
    return false;
  }

public:
  bool runOnFunction(const FuncPtr &F) final;

  void initialize() final;

  void finalize() final;

  void Replace(const InstPtr &inst, const SSAPtr &value);

  SSAPtr FindValue(const std::shared_ptr<BinaryOperator> &binary_inst);

  SSAPtr FindValue(const std::shared_ptr<AccessInst> &access_inst);

  SSAPtr FindValue(const std::shared_ptr<CallInst> &call_inst);

  SSAPtr FindValue(const std::shared_ptr<ICmpInst> &icmp_inst);

  SSAPtr ValueOf(const SSAPtr &value);

  void RunLocalValueNumbering(const FuncPtr &F);
};

class LocalValueNumberingFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass     = std::make_shared<LocalValueNumbering>();
    auto passinfo =
        std::make_shared<PassInfo>(pass, "LocalValueNumbering", false, 2,
                                   LOCAL_VALUE_NUMBERING);
    passinfo->Requires("FunctionInfoPass");
    return passinfo;
  }
};
} // namespace lava::opt

#endif // LAVA_LOCAL_VALUE_NUMBERING_H
