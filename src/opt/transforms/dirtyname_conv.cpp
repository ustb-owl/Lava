#include <algorithm>

#include "opt/pass.h"
#include "lib/debug.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

int DirtyFunctionConvert;

namespace lava::opt {
/*
 * TODO: DIRTY_HACK
 * Convert starttime to _sysy_starttime, stoptime to _sysy_stoptime
 */

class DirtyFunctionNameConvert : public FunctionPass {
private:
  bool _changed = false;

public:
  bool runOnFunction(const FuncPtr &F) final {
    if (F->GetFunctionName() == "starttime") {
      F->SetName("_sysy_starttime");
    } else if (F->GetFunctionName() == "stoptime") {
      F->SetName("_sysy_stoptime");
    } else {
      return _changed;
    }
    return true;
  }
};

class DirtyFunctionNameConvertFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<DirtyFunctionNameConvert>();
    auto passinfo =
        std::make_shared<PassInfo>(pass, "DirtyFunctionNameConvert", false, false, DIRTY_FUNCTION_CONV);
    return passinfo;
  }
};

static PassRegisterFactory<DirtyFunctionNameConvertFactory> registry;
}