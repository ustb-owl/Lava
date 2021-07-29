#ifndef LAVA_PEEPHOLE_H
#define LAVA_PEEPHOLE_H

#include "pass.h"

namespace lava::back {

/*
 * Run peephole optimization
 */
class PeepHole : public PassBase {
private:
  bool _is_final;

public:
  explicit PeepHole(LLModule &module, bool is_final = false)
    : PassBase(module), _is_final(is_final) {}

  void Reset() final {}

  void runOn(const LLFunctionPtr &func) final;

};

}

#endif //LAVA_PEEPHOLE_H
