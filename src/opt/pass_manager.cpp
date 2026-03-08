#include "pass_manager.h"

#include "lib/debug.h"

namespace lava::opt {

namespace {

class ActiveValidGuard {
private:
  PassNameSet *&_slot;
  PassNameSet  *_saved;

public:
  ActiveValidGuard(PassNameSet *&slot, PassNameSet &next)
      : _slot(slot), _saved(slot) {
    _slot = &next;
  }

  ~ActiveValidGuard() { _slot = _saved; }
};

PassInfoPtr LookupPassInfo(const std::string &name) {
  const auto &passes = PassManager::GetPasses();
  auto        it     = passes.find(name);
  if (it == passes.end()) {
    ERROR("pass %s not found", name.c_str());
  }
  return it->second;
}

} // namespace

PassManager *PassManager::_instance = nullptr;

PassInfo &PassInfo::Requires(const std::string &pass_name) {
  _required_passes.push_back(pass_name);
  PassManager::RequiredBy(pass_name, _pass_name);
  return *this;
}

PassInfo &PassInfo::Invalidates(const std::string &pass_name) {
  _invalidated_passes.push_back(pass_name);
  return *this;
}

void PassManager::RequiredBy(const std::string &slave,
                             const std::string &master) {
  GetRequiredBy()[slave].insert(master);
}

bool PassManager::RunPass(PassNameSet &valid, const PassInfoPtr &info) {
  if (valid.contains(info->name())) {
    return false;
  }

  ActiveValidGuard guard(GetPassManager()->_active_valid, valid);
  RunRequiredPasses(valid, info);

  if (info->is_analysis()) {
    ClearAnalysisResult(info->name());
  }

  auto changed = RunPass(info->pass());
  valid.insert(info->name());

  if (!info->is_analysis() && changed) {
    InvalidateAnalyses(valid);
    for (const auto &name : info->invalidated_passes()) {
      InvalidatePass(valid, name);
    }
  }

  return changed;
}

bool PassManager::RunPass(const PassPtr &pass) {
  bool changed = false;

  // perform initialization
  pass->initialize();
  if (pass->IsModulePass()) {
    changed = pass->runOnModule(module());
    // perform finalization
    pass->finalize();
  } else {
    DBG_ASSERT(pass->IsFunctionPass(), "unknown pass class");
    auto &functions = module().Functions();
    for (std::size_t i = 0; i < functions.size(); i++) {
      auto func = std::static_pointer_cast<Function>(functions[i]);
      if (func->is_copied())
        continue;
      //      TRACE("%s\n", pass->name().c_str());
      changed |= pass->runOnFunction(func);
      // perform finalization
      pass->finalize();
    }
  }

  return changed;
}

void PassManager::RunPasses(const PassPtrList &passes) {
  PassNameSet valid;
  // run all passes
  for (const auto &it : passes) {
    if (it->is_analysis())
      continue;
    RunPass(valid, it);
  }
}

bool PassManager::RunRequiredPasses(PassNameSet       &valid,
                                    const PassInfoPtr &info) {
  for (const auto &name : info->required_passes()) {
    auto required = LookupPassInfo(name);
    if (!required->is_analysis() && required->min_opt_level() > opt_level()) {
      continue;
    }
    RunPass(valid, required);
  }
  return false;
}

bool PassManager::RunRequiredPassesOnFunction(PassNameSet       &valid,
                                              const PassInfoPtr &info,
                                              const FuncPtr     &F) {
  for (const auto &name : info->required_passes()) {
    auto required = LookupPassInfo(name);
    if (!required->is_analysis() && required->min_opt_level() > opt_level()) {
      continue;
    }

    if (required->pass()->IsFunctionPass()) {
      RunPassOnFunction(name, F);
    } else {
      RunPass(valid, required);
    }
  }
  return false;
}

void PassManager::InvalidatePass(PassNameSet &valid, const std::string &name) {
  // return if this pass is already erased from valid
  if (!valid.erase(name))
    return;

  // invalidate all passes that required current pass
  for (const auto &child : GetRequiredBy()[name]) {
    InvalidatePass(valid, child);
  }
}

void PassManager::InvalidateAnalyses(PassNameSet &valid) {
  for (const auto &[name, info] : GetPasses()) {
    if (info->is_analysis()) {
      InvalidatePass(valid, name);
    }
  }
}

void PassManager::ClearAnalysisResult(const std::string &name) {
  GetPassManager()->_analysis_results.erase(name);
}

bool PassManager::RunPassOnFunction(const std::string &name, const FuncPtr &F) {
  auto info = LookupPassInfo(name);
  if (info->pass()->IsModulePass()) {
    if (auto *active_valid = GetPassManager()->_active_valid) {
      return RunPass(*active_valid, info);
    }
    PassNameSet local_valid;
    return RunPass(local_valid, info);
  }

  DBG_ASSERT(info->pass()->IsFunctionPass(), "pass is not function pass");

  PassNameSet local_valid;
  auto       &valid = GetPassManager()->_active_valid != nullptr
                          ? *GetPassManager()->_active_valid
                          : local_valid;

  ActiveValidGuard guard(GetPassManager()->_active_valid, valid);
  RunRequiredPassesOnFunction(valid, info, F);

  auto pass = info->pass();
  pass->initialize();
  auto changed = pass->runOnFunction(F);
  pass->finalize();

  if (info->is_analysis()) {
    InvalidatePass(valid, info->name());
  } else if (changed) {
    InvalidateAnalyses(valid);
    for (const auto &invalidated : info->invalidated_passes()) {
      InvalidatePass(valid, invalidated);
    }
  }

  return changed;
}

void PassManager::RunPasses() {
  auto candidates = Candidates();
  for (const auto &[_, info] : GetPasses()) {
    if (!info->is_analysis() && opt_level() >= info->min_opt_level()) {
      candidates.push_back(info);
    }
  }
  std::sort(candidates.begin(), candidates.end(), compare);
  RunPasses(candidates);
}

void PassManager::init() {
  for (std::size_t i = _initialized_factories; i < _factories.size(); ++i) {
    auto pass      = _factories[i]->CreatePass(this);
    auto pass_name = pass->name();
    DBG_ASSERT(_pass_infos.find(pass_name) == _pass_infos.end(),
               "pass %s has been registered", pass_name.c_str());
    pass->pass()->SetName(pass_name);
    _pass_infos.insert(std::make_pair(pass_name, pass));
  }
  _initialized_factories = _factories.size();
}

bool compare(const PassInfoPtr &ptr1, const PassInfoPtr &ptr2) {
  return ptr1->pass_order() < ptr2->pass_order();
}

} // namespace lava::opt
