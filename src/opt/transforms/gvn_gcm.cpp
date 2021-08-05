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

  void Replace(const InstPtr &inst, const SSAPtr &value, const BlockPtr &block, SSAPtrList::iterator it) {
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

  void GlobalValueNumbering(const FuncPtr &F) {
    auto entry = dyn_cast<BasicBlock>(F->entry());
    auto rpo = _blkWalker.RPOTraverse(entry.get());
  }

};

class GlobalValueNumberingGlobalCodeMotionFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<GlobalValueNumberingGlobalCodeMotion>();
    auto passinfo = std::make_shared<PassInfo>(pass, "GlobalValueNumberingGlobalCodeMotion", false, 0, GVN_GCM);
    return passinfo;
  }
};

static PassRegisterFactory<GlobalValueNumberingGlobalCodeMotionFactory> registry;

}