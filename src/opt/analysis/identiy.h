#ifndef LAVA_IDENTIY_H
#define LAVA_IDENTIY_H

#include "opt/pass.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

namespace lava::opt {
class NeedGcm : public ModulePass {
private:
  bool _need_gcm;

  bool _is_crypto;

public:
  bool runOnModule(Module &M) final {
    _need_gcm = false;
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

  bool IsCrypto() { return _is_crypto; }

  bool IsNeedGcm() const { return _need_gcm; }
};

class NeedGcmFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<NeedGcm>();
    auto passinfo =  std::make_shared<PassInfo>(pass, "NeedGcm", true, 2, NEED_GCM);
    return passinfo;
  }
};
}

#endif //LAVA_IDENTIY_H
