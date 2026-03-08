#ifndef LAVA_IDENTIY_H
#define LAVA_IDENTIY_H

#include "common/casting.h"
#include "opt/pass.h"
#include "opt/pass_manager.h"

namespace lava::opt {

struct NeedGcmInfo {
  bool need_gcm  = false;
  bool is_crypto = false;
};

class NeedGcm : public ModulePass {
private:
  NeedGcmInfo &GetMutableInfo() {
    return PassManager::GetMutableAnalysisResult<NeedGcmInfo>(name());
  }

public:
  bool runOnModule(Module &M) final {
    auto &info     = GetMutableInfo();
    info.need_gcm  = false;
    info.is_crypto = false;
    IsMM(M);
    //    IsMv(M);
    //    IsConv(M);
    IsFFT(M);
    IsCrypto(M);
    return false;
  }

  void IsMM(Module &M);

  void IsMv(Module &M);

  void IsConv(Module &M);

  void IsFFT(Module &M);

  void IsCrypto(Module &M);
};

class NeedGcmFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<NeedGcm>();
    auto passinfo =
        std::make_shared<PassInfo>(pass, "NeedGcm", true, 2, NEED_GCM);
    return passinfo;
  }
};
} // namespace lava::opt

#endif // LAVA_IDENTIY_H
