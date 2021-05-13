#include "lib/debug.h"
#include "pass_manager.h"

namespace lava::opt {

PassManager PassManager::_instance;

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
  if (!valid.insert(info->name()).second) return changed;
  // check dependencies, run required passes first
  changed = RunRequiredPasses(valid, info);
  // run current pass
  if (RunPass(info->pass())) {
    changed = true;
    // invalidate passes
    for (const auto &name : info->invalidated_passes()) {
      InvalidatePass(valid, name);
    }
  }
  return changed;
}


bool PassManager::RunPass(const PassPtr &pass) {
  bool changed = false;

  if (pass->IsModulePass()) {
    changed = pass->runOnModule(module());
  } else {
    DBG_ASSERT(pass->IsFunctionPass(), "unknown pass class");
    for (const auto &it : module().Functions()) {
      auto func = std::static_pointer_cast<Function>(it);
      changed = pass->runOnFunction(func);
    }
  }

  return changed;
}

void PassManager::RunPasses(const PassPtrList &passes) {
  bool changed = true;
  PassNameSet valid;

  while (changed) {
    changed = false;
    valid.clear();

    // run all passes
    for (const auto &it : passes) {
      changed |= RunPass(valid, it);
    }
  }
}

bool PassManager::RunRequiredPasses(PassNameSet &valid, const PassInfoPtr &info) {
  bool changed = false, rerun = true;
  while (rerun) {
    rerun = false;
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

      // run if not valid
      if (!valid.count(name) && RunPass(valid, it->second)) {
        changed = true;
        rerun = !it->second->invalidated_passes().empty();
      }
    }
  }
  return changed;
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
  RunPasses(candidates);
}

void PassManager::init() {
  for (const auto &it : _factories) {
    auto pass = it->CreatePass(this);
    auto pass_name = pass->name();
    DBG_ASSERT(_pass_infos.find(pass_name) == _pass_infos.end(), "pass %s has been registered", pass_name.c_str());
    _pass_infos.insert(std::make_pair(pass_name, pass));
  }
}

}