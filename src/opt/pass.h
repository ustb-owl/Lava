#ifndef XY_LANG_PASS_H
#define XY_LANG_PASS_H

#include <string>

#include "order.h"
#include "mid/ir/ssa.h"
#include "mid/ir/module.h"

using namespace lava::mid;

namespace lava::opt {
class Pass {
private:
  std::string _pass_name;

public:
  // return true if is module pass
  virtual bool IsModulePass() const = 0;

  // return true if is function pass
  virtual bool IsFunctionPass() const = 0;

  // return true if is block pass
  virtual bool IsBlockPass() const = 0;

  // overridden by subclass, run on module
  virtual bool runOnModule(Module &M) = 0;

  // overridden by subclass, run on function
  virtual bool runOnFunction(const FuncPtr &F) = 0;

  // initialize before run pass
  virtual void initialize() = 0;

  // finalize after run pass
  virtual void finalize() = 0;

  // print - Print out the internal state of the pass.  This is called by
  // Analyze to print out the contents of an analysis.  Otherwise it is not
  // necessary to implement this method.  Beware that the module pointer MAY be
  // null.  This automatically forwards to a virtual function that does not
  // provide the Module* in case the analysis doesn't need it it can just be
  // ignored.
  virtual void print(std::ostream &O, const Module *M) const;

  void SetName(const std::string &name) { _pass_name = name; }
  const std::string &name() const { return _pass_name; }
};

//std::ostream &operator<<(std::ostream &OS, const Pass &P) {
//  P.print(OS, nullptr); return OS;
//}

//===----------------------------------------------------------------------===//
// ModulePass class - This class is used to implement unstructured
// interprocedural optimizations and analyses.  ModulePasses may do anything
// they want to the program.
class ModulePass : public Pass {
public:
  bool IsBlockPass()    const final { return false; }
  bool IsModulePass()   const final { return true;  }
  bool IsFunctionPass() const final { return false; }

  bool runOnFunction(const FuncPtr &F) final { return false; };

  // subclass can override this method to perform initialization
  void initialize() override {};

  // subclass can override this method to perform finalization
  void finalize() override {};

  ModulePass() = default;
  // Force out-of-line virtual method.
  virtual ~ModulePass() = default;
};

//===----------------------------------------------------------------------===//
// FunctionPass class - This class is used to implement most global
// optimizations.  Optimizations should subclass this class if they meet the
// following constraints:
//
//  1. Optimizations are organized globally, i.e., a function at a time
//  2. Optimizing a function does not cause the addition or removal of any
//     functions in the module
class FunctionPass : public Pass {
public:
  FunctionPass() = default;
  virtual ~FunctionPass() = default;

  bool IsBlockPass()    const final { return false; }
  bool IsModulePass()   const final { return false; }
  bool IsFunctionPass() const final { return true;  }

  // doInitialization - Virtual method overridden by subclasses to do
  // any necessary per-module initialization.
  virtual bool doInitialization(Module &M) { return false; }

  // not implement
  bool runOnModule(Module &M) final { return false; }

  // subclass can override this method to perform initialization
  void initialize() override {};

  // subclass can override this method to perform finalization
  void finalize() override {};
};

using PassPtr = std::shared_ptr<Pass>;

}

#endif //XY_LANG_PASS_H
