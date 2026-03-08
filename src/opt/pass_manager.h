#ifndef XY_LANG_PASS_MANAGER_H
#define XY_LANG_PASS_MANAGER_H

#include <any>
#include <map>
#include <string_view>
#include <unordered_set>
#include <utility>

#include "mid/ir/module.h"
#include "opt/pass.h"

namespace lava::opt {
class PassInfo;
class PassManager;
class PassFactory;

using PassInfoPtr     = std::shared_ptr<PassInfo>;
using PassFactoryPtr  = std::shared_ptr<PassFactory>;
using PassPtrList     = std::vector<PassInfoPtr>;
using PassFactoryList = std::vector<PassFactoryPtr>;
using PassNameList    = std::vector<std::string>;
using PassInfoMap     = std::unordered_map<std::string, PassInfoPtr>;
using PassNameSet     = std::unordered_set<std::string>;
using RequirementMap  = std::unordered_map<std::string, PassNameSet>;
using AnalysisStore   = std::unordered_map<std::string, std::any>;

// pass factory
class PassFactory {
public:
  virtual ~PassFactory()                        = default;
  virtual PassInfoPtr CreatePass(PassManager *) = 0;
};

// pass information
class PassInfo {
private:
  PassPtr      _pass;
  std::string  _pass_name;
  bool         _is_analysis;
  std::size_t  _min_opt_level;
  std::size_t  _pass_order;
  PassNameList _required_passes;
  PassNameList _invalidated_passes;

public:
  PassInfo(PassPtr pass, std::string name, bool is_analysis,
           std::size_t min_opt_level, std::size_t order)
      : _pass(std::move(pass)), _pass_name(std::move(name)),
        _is_analysis(is_analysis), _min_opt_level(min_opt_level),
        _pass_order(order) {}

  // add required pass by name for current pass
  // all required passes should be run before running current pass
  PassInfo &Requires(const std::string &pass_name);

  // add invalidated pass by name for current pass
  // all invalidated passes should be run again after running current pass
  PassInfo &Invalidates(const std::string &pass_name);

  // setters
  // set if current pass is an analysis pass
  PassInfo &set_is_analysis(bool is_analysis) {
    _is_analysis = is_analysis;
    return *this;
  }

  // set minimum optimization level of current pass required
  PassInfo &set_min_opt_level(std::size_t min_opt_level) {
    _min_opt_level = min_opt_level;
    return *this;
  }

  PassInfo &set_pass_order(std::size_t order) {
    _pass_order = order;
    return *this;
  }

  // getter/setter
  const PassPtr &pass() const { return _pass; }
  std::string    name() const { return _pass_name; }
  bool           is_analysis() const { return _is_analysis; }
  std::size_t    pass_order() const { return _pass_order; };
  std::size_t    min_opt_level() const { return _min_opt_level; }

  const PassNameList &required_passes() const { return _required_passes; }
  const PassNameList &invalidated_passes() const { return _invalidated_passes; }
};

bool compare(const PassInfoPtr &ptr1, const PassInfoPtr &ptr2);

// pass manager
class PassManager {
private:
  std::size_t     _opt_level;
  mid::Module    *_module;
  std::size_t     _initialized_factories;
  PassInfoMap     _pass_infos;
  RequirementMap  _requirements;
  PassPtrList     _candidates;
  PassFactoryList _factories;
  AnalysisStore   _analysis_results;
  PassNameSet    *_active_valid;

  void AddFactory(const std::shared_ptr<PassFactory> &factory) {
    _factories.push_back(factory);
  }

  void init();

public:
  static PassManager *_instance;

  PassManager()
      : _opt_level(0), _module(nullptr), _initialized_factories(0),
        _active_valid(nullptr) {}

  explicit PassManager(mid::Module &module)
      : _opt_level(0), _module(&module), _initialized_factories(0),
        _active_valid(nullptr) {}

  static void Initialize() { GetPassManager()->init(); }

  static PassManager *GetPassManager() {
    if (_instance == nullptr)
      _instance = new PassManager();
    return _instance;
  }

  static PassInfoMap &GetPasses() { return GetPassManager()->_pass_infos; }

  static RequirementMap &GetRequiredBy() {
    return GetPassManager()->_requirements;
  }

  static PassPtrList &Candidates() { return GetPassManager()->_candidates; }

  static void RequiredBy(const std::string &slave, const std::string &master);

  // run required passes
  static bool RunRequiredPasses(PassNameSet &valid, const PassInfoPtr &info);
  static bool RunRequiredPassesOnFunction(PassNameSet       &valid,
                                          const PassInfoPtr &info,
                                          const FuncPtr     &F);

  // invalidate the specific pass
  static void InvalidatePass(PassNameSet &valid, const std::string &name);

  static void InvalidateAnalyses(PassNameSet &valid);
  static void ClearAnalysisResult(const std::string &name);

  /* methods related with analysis result */
  template <typename ResultType>
  static ResultType &GetMutableAnalysisResult(const std::string &name) {
    auto manager    = GetPassManager();
    auto &[_, slot] = *manager->_analysis_results.try_emplace(name).first;
    if (!slot.has_value()) {
      slot.emplace<ResultType>();
    } else if (slot.type() != typeid(ResultType)) {
      ERROR("analysis result %s has unexpected type", name.c_str());
    }
    return std::any_cast<ResultType &>(slot);
  }

  template <typename ResultType>
  static const ResultType &GetAnalysisResult(const std::string &name) {
    auto manager = GetPassManager();
    auto it      = manager->_analysis_results.find(name);
    if (it == manager->_analysis_results.end() || !it->second.has_value()) {
      ERROR("analysis result %s is not available", name.c_str());
    }
    if (it->second.type() != typeid(ResultType)) {
      ERROR("analysis result %s has unexpected type", name.c_str());
    }
    return std::any_cast<const ResultType &>(it->second);
  }

  template <typename ResultType>
  static const ResultType &RequireAnalysisResult(const std::string &name) {
    auto manager = GetPassManager();
    auto pass    = manager->_pass_infos.find(name);
    if (pass == manager->_pass_infos.end()) {
      ERROR("analysis pass %s not found", name.c_str());
    }
    if (!pass->second->is_analysis()) {
      ERROR("pass %s is not analysis pass", name.c_str());
    }

    if (manager->_active_valid != nullptr) {
      RunPass(*manager->_active_valid, pass->second);
    } else {
      PassNameSet local_valid;
      RunPass(local_valid, pass->second);
    }

    return GetAnalysisResult<ResultType>(name);
  }

  template <typename ResultType>
  static const ResultType &
  RequireAnalysisResultOnFunction(const std::string &name, const FuncPtr &F) {
    auto manager = GetPassManager();
    auto pass    = manager->_pass_infos.find(name);
    if (pass == manager->_pass_infos.end()) {
      ERROR("analysis pass %s not found", name.c_str());
    }
    if (!pass->second->is_analysis()) {
      ERROR("pass %s is not analysis pass", name.c_str());
    }

    if (pass->second->pass()->IsFunctionPass()) {
      RunPassOnFunction(name, F);
    } else if (manager->_active_valid != nullptr) {
      RunPass(*manager->_active_valid, pass->second);
    } else {
      PassNameSet local_valid;
      RunPass(local_valid, pass->second);
    }

    return GetAnalysisResult<ResultType>(name);
  }

  // register pass
  static void RegisterPassFactory(const PassFactoryPtr &factory) {
    GetPassManager()->AddFactory(factory);
  }

  // run a specific pass
  static bool RunPass(const PassPtr &pass);

  // run a specific pass if it's not valid
  // returns true if changed
  static bool RunPass(PassNameSet &valid, const PassInfoPtr &info);

  // run a specific function pass on a single function through the manager
  // without exposing manual initialize/run/finalize in transforms.
  static bool RunPassOnFunction(const std::string &name, const FuncPtr &F);

  // run all passes
  static void RunPasses(const PassPtrList &passes);
  static void RunPasses();

  // getter/setter
  static std::size_t  opt_level() { return GetPassManager()->_opt_level; }
  static mid::Module &module() { return *GetPassManager()->_module; }
  static void set_opt_level(int value) { GetPassManager()->_opt_level = value; }

  static void SetModule(mid::Module &module) {
    GetPassManager()->_module = &module;
  }
};

template <typename PassClassFactory> class PassRegisterFactory {
public:
  PassRegisterFactory() {
    auto pass_factory = std::make_shared<PassClassFactory>();
    // make sure PassManager has been created
    PassManager::GetPassManager();
    DBG_ASSERT(PassManager::GetPassManager() != nullptr,
               "PassManager hasn't been created");
    PassManager::RegisterPassFactory(pass_factory);
  }
};

} // namespace lava::opt

#endif // XY_LANG_PASS_MANAGER_H
