#include "opt/transforms/sanitize_ir.h"

#include <unordered_set>
#include <vector>

#include "common/casting.h"
#include "opt/register.h"

int SanitizeIRPass;

namespace {

using namespace lava::mid;

bool SanitizeFunctionCFG(const FuncPtr &F) {
  if (!F || F->is_decl()) return false;

  bool changed = false;

  std::unordered_set<BasicBlock *> in_function;
  for (const auto &bb_use : *F) {
    auto block = lava::dyn_cast<BasicBlock>(bb_use.value());
    if (block) in_function.insert(block.get());
  }

  auto entry = lava::dyn_cast<BasicBlock>(F->entry());
  if (!entry) return false;

  std::unordered_set<BasicBlock *> reachable;
  std::vector<BlockPtr> worklist{entry};
  while (!worklist.empty()) {
    auto block = lava::dyn_cast<BasicBlock>(worklist.back());
    worklist.pop_back();
    if (!block || !reachable.insert(block.get()).second) continue;

    auto term = lava::dyn_cast<TerminatorInst>(block->terminator());
    if (!term) continue;
    for (unsigned i = 0; i < term->GetSuccessorNum(); ++i) {
      auto succ = lava::dyn_cast<BasicBlock>(term->GetSuccessor(i));
      if (succ && in_function.count(succ.get())) {
        worklist.push_back(succ);
      }
    }
  }

  for (auto &bb_use : *F) {
    auto block = lava::dyn_cast<BasicBlock>(bb_use.value());
    if (block && !reachable.count(block.get())) {
      bb_use.set(nullptr);
      changed = true;
    }
  }
  F->RemoveValue(nullptr);

  for (const auto &bb_use : *F) {
    auto block = lava::dyn_cast<BasicBlock>(bb_use.value());
    if (!block) continue;

    std::vector<unsigned> dead_pred_indices;
    unsigned idx = 0;
    for (const auto &pred_use : *block) {
      auto pred = lava::dyn_cast<BasicBlock>(pred_use.value());
      if (!pred || !reachable.count(pred.get())) {
        dead_pred_indices.push_back(idx);
      }
      idx++;
    }
    if (dead_pred_indices.empty()) continue;

    for (auto inst_it = block->inst_begin(); inst_it != block->inst_end();) {
      auto phi = lava::dyn_cast<PhiNode>(*inst_it);
      if (!phi) break;

      for (auto it = dead_pred_indices.rbegin(); it != dead_pred_indices.rend(); ++it) {
        if (*it < phi->size()) phi->RemoveValue(*it);
      }

      if (phi->size() == 1) {
        phi->ReplaceBy(phi->begin()->value());
        inst_it = phi->EraseFromParent();
        changed = true;
      } else {
        ++inst_it;
      }
    }

    for (auto it = dead_pred_indices.rbegin(); it != dead_pred_indices.rend(); ++it) {
      if (*it < block->size()) {
        block->RemoveValue(*it);
        changed = true;
      }
    }
  }

  return changed;
}

}

namespace lava::opt {

bool SanitizeIR::runOnModule(Module &M) {
  bool changed = false;
  for (const auto &F : M.Functions()) {
    changed |= SanitizeFunctionCFG(F);
  }
  return changed;
}

void RegisterSanitizeIRPass() {
  RegisterPassCliMetadata({
      "SanitizeIR",
      "sanitize-ir",
      {},
      "repair CFG and phi edges after transforms",
  });
  static PassRegisterFactory<SanitizeIRFactory> registry;
}

}
