#ifndef LAVA_BACK_PASS_H
#define LAVA_BACK_PASS_H

#include "back/arch/arm/module.h"
#include "back/arch/arm/instdef.h"

namespace lava::back {

class PassBase {
protected:
  LLModule      &_module;

public:
  PassBase(LLModule &module) : _module(module) {}

  virtual ~PassBase() = default;

  virtual void Initialize() {}

  virtual void Reset() = 0;

  virtual void runOn(const LLFunctionPtr &func) = 0;
};

using PassPtr  = std::shared_ptr<PassBase>;
using PassList = std::list<PassPtr>;

// create a new pass pointer
template <typename T, typename... Args>
inline std::shared_ptr<T> MakePass(Args &&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

}

#endif //LAVA_BACK_PASS_H
