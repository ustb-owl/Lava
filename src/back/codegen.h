#ifndef LAVA_CODEGEN_H
#define LAVA_CODEGEN_H

#include "passes/pass.h"
#include "mid/ir/module.h"

namespace lava::back {

enum class TargetArch {
  ARM = 0
};

// consume IR and generate
class CodeGenerator {
private:
  TargetArch            _target_arch;
  mid::Module*          _module;
  LLModule              _ll_module;
  std::vector<PassPtr>  _passes;

public:
  CodeGenerator() : _target_arch(TargetArch::ARM), _module(nullptr) {}

  CodeGenerator(TargetArch targetArch) : _target_arch(targetArch), _module(nullptr) {}

  // set IR module
  void SetModule(mid::Module *module) { _module = module; }

  LLModule &GetLLModule() { return _ll_module; }

  // generate LLIR
  void CodeGene();

  // create passes
  template<typename TYPE, typename... Args>
  std::shared_ptr<TYPE> CREATE_PASS(Args &&... args) {
    auto pass = std::make_shared<TYPE>(std::forward<Args>(args)...);
    DBG_ASSERT(pass != nullptr, "create pass failed");
    return pass;
  }

  // register passes
  void RegisterPasses();

  // run passes on each function
  void RunPasses();

  // dump asm
  void DumpASM(std::ostream &os) const { _ll_module.DumpASM(os); }

  TargetArch targetArch() const { return _target_arch; }
};

}

#endif //LAVA_CODEGEN_H
