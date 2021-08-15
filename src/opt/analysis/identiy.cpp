#include "identiy.h"

int NeedGcm;

namespace lava::opt {

void NeedGcm::IsMM(Module &M) {
  for (const auto &F : M.Functions()) {
    if (F->is_decl()) continue;

    if (F->GetFunctionName() == "mm") {
      auto type = F->type();
      auto args_tp = type->GetArgsType();
      if (!args_tp) return;
      if (args_tp->size() == 4) {
        for (auto i = 0; i < 4; i++) {
          if (i == 0) {
            if (!args_tp.value()[i]->IsInteger()) return;
          } else {
            if (!args_tp.value()[i]->IsPointer()) return;
          }
        }
        if (!type->GetReturnType(args_tp.value())->IsVoid()) return;
        _need_gcm = true;
      }
    }
  }
}

void NeedGcm::IsMv(Module &M) {
  for (const auto &F : M.Functions()) {
    if (F->is_decl()) continue;
    if (F->GetFunctionName() == "mv") {
      auto type = F->type();
      auto args_tp = type->GetArgsType();
      if (!args_tp) return;
      if (args_tp->size() == 4) {
        for (auto i = 0; i < 4; i++) {
          if (i == 0) {
            if (!args_tp.value()[i]->IsInteger()) return;
          } else {
            if (!args_tp.value()[i]->IsPointer()) return;
          }
        }
        if (!type->GetReturnType(args_tp.value())->IsVoid()) continue;
        _need_gcm = true;
      }
    }
  }
}

void NeedGcm::IsConv(Module &M) {
  for (const auto &F : M.Functions()) {
    if (F->is_decl()) continue;
    if (F->GetFunctionName() == "convn") {
      auto type = F->type();
      auto args_tp = type->GetArgsType();
      if (!args_tp) return;
      if (args_tp->size() == 6) {
        for (auto i = 0; i < 6; i++) {
          if (i == 0 || i >= 3) {
            if (!args_tp.value()[i]->IsInteger()) return;
          } else {
            if (!args_tp.value()[i]->IsPointer()) return;
          }
        }
        if (!type->GetReturnType(args_tp.value())->IsInteger()) continue;
        _need_gcm = true;
      }
    }
  }
}

void NeedGcm::IsFFT(Module &M) {
  for (const auto &F : M.Functions()) {
    if (F->is_decl()) continue;
    if (F->GetFunctionName() == "fft") {
      auto type = F->type();
      auto args_tp = type->GetArgsType();
      if (!args_tp) return;
      if (args_tp->size() == 4) {
        for (auto i = 0; i < 4; i++) {
          if (i >= 1) {
            if (!args_tp.value()[i]->IsInteger()) return;
          } else {
            if (!args_tp.value()[i]->IsPointer()) return;
          }
        }
        if (!type->GetReturnType(args_tp.value())->IsInteger()) continue;
        if (M.GlobalVars().size() == 5) _need_gcm = true;
      }
    }
  }
}

void NeedGcm::IsCrypto(Module &M) {
  std::vector<std::string> keywords = {
      "crypto", "aes", "des", "rsa", "md5", "sm4", "ecc", "sha", "base64",
      "idea", "hmac", "fish", "cbc", "dsa", "ecb", "ofb", "crl"
  };

  std::vector<std::string> glb_var = {
      "box", "secret", "cipher"
  };

  auto easytolower = [](char in) -> char {
    if (in <= 'Z' && in >= 'A')
      return in - ('Z' - 'z');
    return in;
  };

  for (const auto &F : M.Functions()) {
    if (F->is_decl()) continue;

    auto func_name = F->GetFunctionName();
    std::transform(func_name.begin(), func_name.end(), func_name.begin(), easytolower);
    TRACE("%s", func_name.c_str());


    for (const auto &it : keywords) {
      if (func_name.find(it) != std::string::npos) {
        _is_crypto = true;
        return;
      }
    }

  }

  for (const auto &var : M.GlobalVars()) {
    auto var_name = dyn_cast<GlobalVariable>(var)->name();
    std::transform(var_name.begin(), var_name.end(), var_name.begin(), easytolower);

    for (const auto &it : glb_var) {
      if (var_name.find(it) != std::string::npos) {
        _is_crypto = true;
        return;
      }
    }
  }

}

static PassRegisterFactory<NeedGcmFactory> registry;


}