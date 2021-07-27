#ifndef LAVA_PEEPHOLE_H
#define LAVA_PEEPHOLE_H

#include "pass.h"

namespace lava::back {

/*
 * Run peephole optimization
 */
class PeepHole : public PassBase {

public:
  explicit PeepHole(LLModule &module) : PassBase(module) {}

  void Reset() final {}

  void runOn(const LLFunctionPtr &func) final;

};

}

#endif //LAVA_PEEPHOLE_H
