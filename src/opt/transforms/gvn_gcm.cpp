#include <algorithm>

#include "opt/pass.h"
#include "opt/blkwalker.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

int GlobalValueNumbering;

namespace lava::opt {

using ValueNumber = std::vector<std::pair<Value *, Value *>>;

/*
 * Global value numbering and global code motion
 * TODO: swapOperand or FoldBinary would cause int_literal WA
 */
class GlobalValueNumberingGlobalCodeMotion : public FunctionPass {
private:
  bool        _changed;
  BlockWalker _blkWalker;
  ValueNumber _value_number;


public:

  bool runOnFunction(const FuncPtr &F) final {
    _changed = false;
    if (F->is_decl()) return _changed;

    GlobalValueNumbering(F);

    return _changed;
  }

  void initialize() final {}

  void finalize() final {
    _value_number.clear();
  }

  void Replace(const InstPtr &inst, const SSAPtr &value, BasicBlock *block, SSAPtrList::iterator it) {
    if (inst != value) {
      inst->ReplaceBy(value);

      auto res = std::find_if(_value_number.begin(), _value_number.end(),
          [inst](std::pair<Value *, Value *>kv) {
            return kv.first == inst.get();
      });
      if (res != _value_number.end()) {
        std::swap(*res, _value_number.back());
        _value_number.pop_back();
      }

      DBG_ASSERT(*it == inst, "iterator is not current instruction");
      block->insts().erase(it);
    }
  }

  Value *ValueOf(Value *value) {
    auto it = std::find_if(_value_number.begin(), _value_number.end(), [value](std::pair<Value *, Value *> kv) {
      return kv.first == value;
    });
    if (it != _value_number.end()) return it->second;

    uint32_t idx = _value_number.size();
    _value_number.emplace_back(value, value);

    // find any way

    return _value_number[idx].second;
  }

  void GlobalValueNumbering(const FuncPtr &F) {
    auto entry = dyn_cast<BasicBlock>(F->entry());
    auto rpo = _blkWalker.RPOTraverse(entry.get());

    for (const auto &BB : rpo) {
      for (auto it = BB->insts().begin(); it != BB->inst_end(); ) {
        auto next = std::next(it);
        if (auto binary_inst = dyn_cast<BinaryOperator>(*it)) {
          // always move const value to rhs
          if (binary_inst->LHS()->classId() == ClassId::ConstantIntId) {
            binary_inst->swapOperand();
          }

          // try to get lhs and rhs as constant value
          auto lhs_const = dyn_cast<ConstantInt>(binary_inst->LHS());
          auto rhs_const = dyn_cast<ConstantInt>(binary_inst->RHS());

          if ((lhs_const != nullptr) && (rhs_const != nullptr)) {
            auto const_inst = binary_inst->EvalArithOnConst();
            Replace(binary_inst, const_inst, BB, it);
          } else {
            binary_inst->TryToFold();
            if (auto simp_val = binary_inst->OptimizedValue()) {
              Replace(binary_inst, simp_val, BB, it);
            } else {

            }
          }
        }

        it = next;
      }
    }
  }

};

class GlobalValueNumberingGlobalCodeMotionFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<GlobalValueNumberingGlobalCodeMotion>();
    auto passinfo = std::make_shared<PassInfo>(pass, "GlobalValueNumberingGlobalCodeMotion", false, 1, GVN_GCM);
    return passinfo;
  }
};

static PassRegisterFactory<GlobalValueNumberingGlobalCodeMotionFactory> registry;

}