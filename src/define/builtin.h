#ifndef LAVA_BUILTIN_H
#define LAVA_BUILTIN_H

#include "type.h"

namespace lava::define{

struct BT_Function {
  Type        _ret_type;
  std::string _func_name;
  std::string _args;
};

#define BT_FUNCTIONS(Type, Name, Args) Name,
static const char* reg_funcs[] = {
#include "builtin.inc"
};

bool IsBuiltinFunction(const std::string &name);

}
#endif //LAVA_BUILTIN_H
