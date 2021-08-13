#ifndef LAVA_TAILRECURSION_H
#define LAVA_TAILRECURSION_H

#include "pass.h"

namespace lava::back {
class TailRecursionTransform : public PassBase {
public:
  explicit TailRecursionTransform(LLModule &module) : PassBase(module) {}

  void Reset() final {}

  void runOn(const LLFunctionPtr &func) final;
};
}

#endif //LAVA_TAILRECURSION_H
