#ifndef LAVA_FUNCANALYSIS_H
#define LAVA_FUNCANALYSIS_H

#include "opt/pass.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

namespace lava::opt {

class FunctionNode;
class FunctionInfo;
using FuncNodePtr = std::shared_ptr<FunctionNode>;
using FuncInfoMap = std::unordered_map<Function *, FunctionInfo>;

class FunctionInfo {
public:
  using RetInstPtr = std::shared_ptr<ReturnInst>;

private:
  FuncNodePtr F;
  RetInstPtr ret_inst;

public:
  bool is_leaf;
  bool load_global;
  bool store_global;
  bool load_global_array;
  bool has_size_effect;

  FunctionInfo()
    : F(nullptr), is_leaf(false),
      load_global(false), store_global(false),
      load_global_array(false), has_size_effect(false) {}

  bool IsPure() const { return !(load_global || has_size_effect); }

  bool IsRecursive() const;

  const FuncNodePtr &FuncNode() const {
    return F;
  }

  void SetFuncNode(const FuncNodePtr &f) {
    DBG_ASSERT(F == nullptr, "FunctionInfo already has a function pointer");
    F = f;
  }

  void SetRetInst(const RetInstPtr &ret) {
    ret_inst = ret;
  }

  const RetInstPtr RetInst() const {
    return ret_inst;
  }
};

class FunctionNode {
  Function *_func;
  std::unordered_set<FuncNodePtr> caller;
  std::unordered_set<FuncNodePtr> callee;
public:
  explicit FunctionNode(Function *F) : _func(F) {}

  Function *GetFunction() { return _func; }

  void SetFunction(Function *F) { _func = F; }

  void AddCaller(const FuncNodePtr &F) { caller.insert(F); }

  void AddCallee(const FuncNodePtr &F) { callee.insert(F); }

  std::unordered_set<FuncNodePtr> &Callers() { return caller; }
  std::unordered_set<FuncNodePtr> &Callees() { return callee; }
  const std::unordered_set<FuncNodePtr> &Callers() const { return caller; }
  const std::unordered_set<FuncNodePtr> &Callees() const { return callee; }

  void dump() const;
};


/*
 Calculate function information
 */
class FunctionInfoPass : public ModulePass {
private:
  FuncNodePtr                                  _main;
  FuncInfoMap                                  _func_infos;
  std::unordered_set<Function *>               _visited;
  std::unordered_map<Function *, FuncNodePtr>  _func_map;

public:
  void initialize() final {
    _main = nullptr;
    _visited.clear();
    _func_infos.clear();
    _func_map.clear();
  }

  void finalize() final {
    _visited.clear();
  }

  void CalculateCallGraph(Function *F);

  void CollectSideEffectInfo(const FuncNodePtr &FN);

  void DumpCallGraph();

  void dump();

  const FuncInfoMap &GetFunctionInfo() { return _func_infos; }

  bool runOnModule(Module &M) final;
};

class FunctionInfoPassFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<FunctionInfoPass>();
    auto passinfo =  std::make_shared<PassInfo>(pass, "FunctionInfoPass", true, 0, FUNCTION_INFO);
    return passinfo;
  }
};

}

#endif //LAVA_FUNCANALYSIS_H
