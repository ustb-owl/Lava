#ifndef LAVA_CODEGEN_H
#define LAVA_CODEGEN_H

#include "arch/arm/module.h"
#include "mid/ir/module.h"

namespace lava::back {

enum class TargetArch {
  ARM = 0
};

// consume IR and generate
class CodeGenerator {
private:
  TargetArch   _target_arch;
  mid::Module*  _module;
  LLModule     _ll_module;

public:
  CodeGenerator() : _target_arch(TargetArch::ARM), _module(nullptr) {}

  CodeGenerator(TargetArch targetArch) : _target_arch(targetArch), _module(nullptr) {}

  // set IR module
  void SetModule(mid::Module *module) { _module = module; }

  // generate LLIR
  void CodeGene();

  TargetArch targetArch() const { return _target_arch; }
};

}

#endif //LAVA_CODEGEN_H
