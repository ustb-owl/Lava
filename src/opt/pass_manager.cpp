#include "lib/debug.h"
#include "pass_manager.h"


namespace lava::opt {

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


void PassManager::RequiredBy(const std::string &slave, const std::string &master) {
  GetRequiredBy()[slave].insert(master);
}

bool PassManager::RunPass(PassNameSet &valid, const PassInfoPtr &info) {
  bool changed = false;

  // check dependencies, run required passes first
  RunRequiredPasses(valid, info);

  // run current pass
  if (RunPass(info->pass())) {
    changed = true;
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
    for (const auto &it : module().Functions()) {
      auto func = std::static_pointer_cast<Function>(it);
//      TRACE("%s\n", pass->name().c_str());
      changed = pass->runOnFunction(func);
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
    if (it->is_analysis()) continue;
    RunPass(valid, it);
  }
}

bool PassManager::RunRequiredPasses(const PassPtr &info) {
  return RunRequiredPasses(info.get());
}

bool PassManager::RunRequiredPasses(const Pass *info) {
  PassNameSet valid;

  auto pass = GetPasses().find(info->name());
  DBG_ASSERT(pass != GetPasses().end(), "find pass failed");

  for (const auto &name : (*pass).second->required_passes()) {
    // get pointer of pass
    const auto &passes = GetPasses();
    auto it = passes.find(name);
    DBG_ASSERT(it != passes.end(), "required pass not found");

    // check if current pass can be run
    if (!it->second->is_analysis() &&
        it->second->min_opt_level() > opt_level()) {
      continue;
    }
    RunPass(valid, it->second);
  }
  return false;
}

bool PassManager::RunRequiredPasses(PassNameSet &valid, const PassInfoPtr &info) {
  for (const auto &name : info->required_passes()) {

    // get pointer of pass
    const auto &passes = GetPasses();
    auto it = passes.find(name);
    DBG_ASSERT(it != passes.end(), "required pass not found");

    // check if current pass can be run
    if (!it->second->is_analysis() &&
        it->second->min_opt_level() > opt_level()) {
      continue;
    }

    RunPass(valid, it->second);
  }
  return false;
}

void PassManager::InvalidatePass(PassNameSet &valid, const std::string &name) {
  // return if this pass is already erased from valid
  if (!valid.erase(name)) return;

  // invalidate all passes that required current pass
  for (const auto &child : GetRequiredBy()[name]) {
    InvalidatePass(valid, child);
  }
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
  for (const auto &it : _factories) {
    auto pass = it->CreatePass(this);
    auto pass_name = pass->name();
    DBG_ASSERT(_pass_infos.find(pass_name) == _pass_infos.end(), "pass %s has been registered", pass_name.c_str());
    pass->pass()->SetName(pass_name);
    _pass_infos.insert(std::make_pair(pass_name, pass));
  }
}

bool compare(const PassInfoPtr &ptr1, const PassInfoPtr &ptr2) {
  return ptr1->pass_order() < ptr2->pass_order();
}

}