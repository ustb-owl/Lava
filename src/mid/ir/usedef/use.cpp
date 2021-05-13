#include "mid/ir/usedef/use.h"
#include "mid/ir/usedef/value.h"

namespace lava::mid {
void Use::removeFromList(const SSAPtr &V) {
  V->removeUse(this);
}

void Use::addToList(const SSAPtr &V) {
  V->addUse(this);
}

Use::Use(Use &&use) noexcept
    : _value(std::move(use._value)), _user(use._user) {
  if (_value) {
    _value->removeUse(&use);
    _value->addUse(this);
  }
}

} //lava::mid
