#include "mid/ir/usedef/user.h"
#include "mid/ir/usedef/value.h"

namespace lava::mid {

void Value::RemoveFromUser()  {
  // remove from all users
  while (!_use_list.empty()) {
    _use_list.front()->getUser()->RemoveValue(this);
  }
}
}
