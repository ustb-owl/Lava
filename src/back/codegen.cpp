#include "codegen.h"
#include "common/casting.h"
#include "back/passes/dme.h"
#include "back/passes/spill.h"
#include "back/passes/funcfix.h"
#include "back/passes/peephole.h"
#include "back/passes/liveness.h"
#include "back/passes/linearscan.h"
#include "back/passes/fastalloc.h"
#include "back/passes/tailrecursion.h"

#include <iostream>

namespace lava::back {

void CodeGenerator::CodeGene() {
  _ll_module.SetFile(_module->File());

  // init global variable
  std::vector<mid::GlobalVariable *> glob_decl;
  for (const auto &it : _module->GlobalVars()) {
    if (auto glob = dyn_cast<mid::GlobalVariable>(it)) {
      glob_decl.push_back(glob.get());
    }
  }
  _ll_module.SetGlobalVariables(glob_decl);

  for (const auto &func : _module->Functions()) {
    auto ll_function = _ll_module.CreateFunction(func);
    DBG_ASSERT(ll_function != nullptr, "create low-level function failed");
    for (const auto &it : *func) {
      auto ll_block = _ll_module.CreateBasicBlock(dyn_cast<mid::BasicBlock>(it.value()), ll_function);
      DBG_ASSERT(ll_block != nullptr, "create low-level block failed");
    }

    // handle phi-node
    _ll_module.HandlePhiNode(func);

    // update vreg number
    ll_function->SetVirtualMax(_ll_module.VirtualMax());
    _ll_module.ClearVirtualMax();

    // create global map
    _ll_module.ClearGlobalMap();
  }
}

void CodeGenerator::RunPasses() {
//  DumpASM(std::cout);
  for (const auto &pass : _passes) {
    for (const auto &function : _ll_module.Functions()) {
      if (function->is_decl()) continue;
      pass->Initialize();
      pass->runOn(function);
      pass->Reset();
    }
  }
}

void CodeGenerator::RegisterPasses() {
  auto tail_recur    = CREATE_PASS<TailRecursionTransform>(_ll_module);
  auto dme           = CREATE_PASS<DeadMoveElimination>(_ll_module);
  auto spill         = CREATE_PASS<Spill>(_ll_module);
  auto pre_peephole  = CREATE_PASS<PeepHole>(_ll_module);
  auto liveness      = CREATE_PASS<LivenessAnalysis>(_ll_module);
  auto linear_scan   = CREATE_PASS<LinearScanRegisterAllocation>(_ll_module, liveness);

  auto fast_alloc    = CREATE_PASS<FastAllocation>(_ll_module);
  auto post_peephole = CREATE_PASS<PeepHole>(_ll_module, true);
  auto func_fix      = CREATE_PASS<FunctionFix>(_ll_module);

  _passes.push_back(tail_recur);
  _passes.push_back(dme);
  _passes.push_back(pre_peephole);
  _passes.push_back(dme);

//  _passes.push_back(liveness);
  if (!no_ra()) {
    // _passes.push_back(fast_alloc);

    _passes.push_back(linear_scan);
    _passes.push_back(spill);
  }

  _passes.push_back(post_peephole);
  _passes.push_back(func_fix);
}

}
