#ifndef LAVA_USE_H
#define LAVA_USE_H

#include <list>
#include <vector>
#include <memory>
#include "lib/debug.h"


namespace lava::mid {

class Use;
class User;
class Value;

using UseList    = std::list<Use *>;
using Operands   = std::vector<Use>;
using SSAPtr     = std::shared_ptr<Value>;
using UserPtr    = std::shared_ptr<User>;
using SSAPtrList = std::list<SSAPtr>;

class Use {
private:
  SSAPtr _value;
  User  *_user;

public:
  Use(const SSAPtr& V, User *U) : _value(V), _user(U) {
    if (V) addToList(V);
  }

  // copy constructor
  Use(const Use &use) : _value(use._value), _user(use._user) {
    if (_value) addToList(_value);
  }

  // move constructor
  Use(Use &&use) noexcept;

  ~Use() { if (_value) removeFromList(_value); }

  void removeFromList(const SSAPtr &V);

  void addToList(const SSAPtr &V);

  void set(const SSAPtr& V) {
    if (_value) removeFromList(_value);
    _value = V;
    if (V) addToList(V);
  }

  const SSAPtr &value() const { return _value; }

  const User *getUser() const { return _user; }

  User *getUser() { return _user; }

  SSAPtr operator->() { return _value; }

  Use &operator=(const Use &RHS) {
    if (this != &RHS) {
      DBG_ASSERT(_user == RHS._user, "user of the two instructions are different");
      set(RHS._value);
    }
    return *this;
  }

  SSAPtr operator=(const SSAPtr &RHS) {
    set(RHS);
    return RHS;
  }

};
}

#endif // LAVA_USE_H
