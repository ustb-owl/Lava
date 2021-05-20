#include "builtin.h"

namespace lava::define {
bool IsBuiltinFunction(const std::string &name) {
  for (const auto &it : reg_funcs) {
    if (name == it) return true;
  }
  return false;
}
}