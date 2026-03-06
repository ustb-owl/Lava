#include "opt/transforms/loop_invariant_hoist.h"

#include "common/casting.h"
#include "opt/register.h"

int LoopInvariantHoistPass;

namespace {

void CollectLoopsPostOrder(const lava::opt::LoopPtr       &loop,
                           std::vector<lava::opt::Loop *> &loops) {
  for (const auto &sub_loop : loop->sub_loops()) {
    CollectLoopsPostOrder(sub_loop, loops);
  }
  loops.push_back(loop.get());
}

} // namespace

namespace lava::opt {

bool LoopInvariantHoist::IsPureCall(const SSAPtr &value) const {
  if (auto call_inst = dyn_cast<CallInst>(value)) {
    auto func = call_inst->Callee();
    if (_func_infos.at(func.get()).IsPure()) {
      auto none_array_arg = std::none_of(
          call_inst->begin(), call_inst->end(),
          [](const Use &use) { return IsSSA<AccessInst>(use.value()); });
      return none_array_arg;
    }
  }
  return false;
}

bool LoopInvariantHoist::IsHoistableInstruction(const InstPtr &inst) const {
  return IsSSA<BinaryOperator>(inst) || IsSSA<AccessInst>(inst) ||
         IsSSA<ICmpInst>(inst) || IsPureCall(inst);
}

bool LoopInvariantHoist::IsInLoop(BasicBlock *BB, const Loop *loop) const {
  auto &blocks = loop->blocks();
  return std::find(blocks.begin(), blocks.end(), BB) != blocks.end();
}

bool LoopInvariantHoist::DominatesPreheader(const InstPtr &inst,
                                            BasicBlock    *preheader) const {
  auto it = _dom_info.at(_cur_func).domBy.find(preheader);
  if (it == _dom_info.at(_cur_func).domBy.end())
    return false;
  return it->second.find(inst->getParent()) != it->second.end();
}

BasicBlock *LoopInvariantHoist::GetLoopPreheader(const Loop *loop) const {
  BasicBlock *preheader = nullptr;
  auto        header    = loop->header();
  for (const auto &pred : header->predecessors()) {
    auto block = pred.get();
    if (IsInLoop(block, loop))
      continue;
    if (preheader != nullptr)
      return nullptr;
    preheader = block;
  }
  return preheader;
}

bool LoopInvariantHoist::HoistInstructionIfInvariant(
    const InstPtr &inst, const Loop *loop, BasicBlock *preheader,
    std::unordered_set<Instruction *> &hoisted,
    std::unordered_set<Instruction *> &active) {
  auto *raw_inst = inst.get();
  if (!IsInLoop(inst->getParent(), loop))
    return DominatesPreheader(inst, preheader);
  if (hoisted.contains(raw_inst))
    return true;
  if (!IsHoistableInstruction(inst) || IsSSA<PhiNode>(inst) ||
      inst->isTerminator())
    return false;
  if (!active.insert(raw_inst).second)
    return false;

  for (unsigned i = 0; i < inst->size(); ++i) {
    auto op_inst = dyn_cast<Instruction>((*inst)[i].value());
    if (!op_inst)
      continue;
    if (IsInLoop(op_inst->getParent(), loop)) {
      if (!HoistInstructionIfInvariant(op_inst, loop, preheader, hoisted,
                                       active)) {
        active.erase(raw_inst);
        return false;
      }
    } else if (!DominatesPreheader(op_inst, preheader)) {
      active.erase(raw_inst);
      return false;
    }
  }

  active.erase(raw_inst);
  if (inst->getParent() != preheader) {
    inst->MoveBefore(preheader->terminator());
    _changed = true;
  }
  hoisted.insert(raw_inst);
  return true;
}

bool LoopInvariantHoist::runOnFunction(const FuncPtr &F) {
  _changed = false;
  if (F->is_decl())
    return false;

  PassManager::RunRequiredPasses(this);
  initialize();
  _cur_func = F.get();

  std::vector<Loop *> loops;
  for (const auto &loop : _loop_info.top_level()) {
    CollectLoopsPostOrder(loop, loops);
  }

  for (auto *loop : loops) {
    auto *preheader = GetLoopPreheader(loop);
    if (preheader == nullptr || preheader->terminator() == nullptr)
      continue;

    std::vector<InstPtr> candidates;
    for (auto *block : loop->blocks()) {
      for (const auto &inst : block->insts()) {
        auto instruction = dyn_cast<Instruction>(inst);
        if (instruction->isTerminator() || IsSSA<PhiNode>(instruction))
          continue;
        candidates.push_back(instruction);
      }
    }

    std::unordered_set<Instruction *> hoisted;
    std::unordered_set<Instruction *> active;
    for (const auto &inst : candidates) {
      if (!IsHoistableInstruction(inst))
        continue;
      HoistInstructionIfInvariant(inst, loop, preheader, hoisted, active);
    }
  }

  return _changed;
}

void LoopInvariantHoist::initialize() {
  _cur_func = nullptr;
  auto func_info =
      PassManager::GetAnalysis<FunctionInfoPass>("FunctionInfoPass");
  _func_infos    = func_info->GetFunctionInfo();
  auto dom_info  = PassManager::GetAnalysis<DominanceInfo>("DominanceInfo");
  _dom_info      = dom_info->GetDomInfo();
  auto loop_info = PassManager::GetAnalysis<LoopInfoPass>("LoopInfoPass");
  _loop_info     = loop_info->GetLoopInfo();
}

void LoopInvariantHoist::finalize() {
  _cur_func = nullptr;
  _func_infos.clear();
  _dom_info.clear();
  _loop_info.Clear();
}

void RegisterLoopInvariantHoistPass() {
  RegisterPassCliMetadata({
      "LoopInvariantHoist",
      "loop-invariant-hoist",
      {"licm-lite"},
      "hoist pure loop-invariant instructions",
  });
  static PassRegisterFactory<LoopInvariantHoistFactory> registry;
}

} // namespace lava::opt
