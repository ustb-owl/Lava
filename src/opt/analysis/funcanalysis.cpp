#include <iostream>
#include "funcanalysis.h"

int FunctionInfo;

namespace lava::opt {


bool FunctionInfoPass::runOnModule(Module &M) {
  for (const auto &F : M.Functions()) {
    if (F->GetFunctionName() == "main") {
      _main = std::make_shared<FunctionNode>(F.get());
    }
    _func_infos.insert({F.get(), FunctionInfo()});
    if (F->is_decl()) {
      _func_infos[F.get()].has_size_effect = true;
      TRACE("%s %d\n", F->GetFunctionName().c_str(), _func_infos[F.get()].IsPure());
    }
  }

  CalculateCallGraph(_main->GetFunction());

  std::vector<Function *> worklist;
  for (const auto &[k, v] : _func_infos) {
    if (v.has_size_effect) worklist.push_back(k);
  }

  while (!worklist.empty()) {
    auto f = worklist.back();
    worklist.pop_back();
    if (_func_map.find(f) == _func_map.end()) continue;
    for (auto &caller : _func_map[f]->Callers()) {
      if (!_func_infos[caller->GetFunction()].has_size_effect) {
        _func_infos[caller->GetFunction()].has_size_effect = true;
        worklist.push_back(caller->GetFunction());
      }
    }
  }

  for (auto &[k, v] : _func_infos) {
    if (_func_map.find(k) == _func_map.end()) continue;
    if (_func_map[k]->Callees().empty()) v.is_leaf = true;
  }

  for (auto it = _func_infos.begin(); it != _func_infos.end();) {
    if (_func_map.find(it->first) == _func_map.end()) {
      it = _func_infos.erase(it);
    } else {
      it++;
    }
  }

//  DumpCallGraph();
//  DumpFunctionInfo();

  return false;
}

void FunctionInfoPass::CalculateCallGraph(Function *F) {
  if (_visited.find(F) != _visited.end()) return;
  _visited.insert(F);

  // get caller
  FuncNodePtr caller = nullptr;
  auto it = _func_map.find(F);
  if (it != _func_map.end()) caller = it->second;
  else {
    caller = std::make_shared<FunctionNode>(F);
    _func_map.insert({F, caller});
  }
  auto &info = _func_infos[F];

  for (const auto &use : *F) {
    auto BB = dyn_cast<BasicBlock>(use.value());
    for (const auto &inst : BB->insts()) {
      if (auto call_inst = dyn_cast<CallInst>(inst)) {
        auto callee = dyn_cast<Function>(call_inst->Callee());
        // get callee
        FuncNodePtr callee_node = nullptr;
        auto res = _func_map.find(callee.get());
        if (res != _func_map.end()) callee_node = res->second;
        else {
          callee_node = std::make_shared<FunctionNode>(callee.get());
          _func_map.insert({callee.get(), callee_node});
        }

        caller->AddCallee(callee_node);
        callee_node->AddCaller(caller);
        CalculateCallGraph(callee.get());
      } else if (auto load_inst = dyn_cast<LoadInst>(inst)) {
        if (IsSSA<GlobalVariable>(load_inst->Pointer())) {
          info.load_global = true;
        }
      } else if (auto store_inst = dyn_cast<StoreInst>(inst)) {
        auto dst = store_inst->pointer();
        if (IsSSA<GlobalVariable>(dst)) {
          info.store_global = true;
          info.has_size_effect = true;
        }
      } else if (auto access_inst = dyn_cast<AccessInst>(inst)) {
        auto array = access_inst->ptr();
        if (IsSSA<GlobalVariable>(array)) {
          info.load_global_array = true;
          info.has_size_effect = true;
        }
      }
    }
  }
  for (const auto &arg : F->args()) {
    if (arg->type()->IsPointer() || arg->type()->IsArray()) {
      info.has_size_effect = true;
      break;
    }
  }
}

void FunctionInfoPass::CollectSideEffectInfo(const FuncNodePtr &FN) {
  if (_func_infos[FN->GetFunction()].has_size_effect) return;

}

void FunctionInfoPass::DumpCallGraph() {
  for (const auto &[k, v] : _func_map) {
    std::cout << k->GetFunctionName() << ":" << std::endl;
    std::cout << "callers: ";
    for (const auto &callers : v->Callers()) {
      std::cout << callers->GetFunction()->GetFunctionName() << " ";
    }
    std::cout << "\ncallees: ";
    for (const auto &callee : v->Callees()) {
      std::cout << callee->GetFunction()->GetFunctionName() << " ";
    }
    std::cout << std::endl;
    std::cout << "---------\n" << std::endl;
  }
}

void FunctionInfoPass::DumpFunctionInfo() {
  for (const auto &[k, v] : _func_infos) {
    std::cout << k->GetFunctionName() << ":" << std::endl;
    std::cout << "is leaf: " << v.is_leaf << "\tload global: " << v.load_global
              << "\tstore global: " << v.store_global << "\tload global array" << v.load_global_array
              << "\thas side effect: " << v.has_size_effect << "\tIsPure: " << v.IsPure()
              << std::endl;
  }
}

static PassRegisterFactory<FunctionInfoPassFactory> registry;


}