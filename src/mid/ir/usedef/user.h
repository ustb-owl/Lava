#ifndef LAVA_USER_H
#define LAVA_USER_H

#include <vector>
#include <utility>
#include <algorithm>
#include "value.h"

namespace lava::mid {

class User : public Value {
private:
  Operands          _operands;
  unsigned int      _operands_num;

public:
  explicit User(ClassId classId) : Value(classId), _operands_num(0) {}
  User(ClassId classId, unsigned operands_num)
    : Value(classId), _operands_num(operands_num) {}

  User(ClassId classId, unsigned operands_num, const Operands &operands)
    : Value(classId), _operands_num(operands_num) {
    DBG_ASSERT(_operands.size() <= _operands_num, "User() operands out of range");
    for (const auto &it : operands) {
      AddValue(it.get());
    }
  }

  unsigned operandNum() const { return _operands.size(); }

  virtual SSAPtr GetOperand(unsigned i) const {
    DBG_ASSERT(i < _operands_num, "getOperand() out of range");
    return _operands[i].get();
  }

  virtual void SetOperand(unsigned i, const SSAPtr& V) {
    DBG_ASSERT(i < _operands_num, "setOperand() out of range");
    _operands[i].set(V);
  }

  void AddValue(const SSAPtr &V) {
    DBG_ASSERT((_operands.size() < _operands_num) || ( _operands_num == 0), " out of range");
    _operands.push_back(Use(V, this));
  }

  void RemoveValue(const SSAPtr& V) {
    RemoveValue(V.get());
  }

  void RemoveValue(Value *V) {
    _operands.erase(
        std::remove_if(_operands.begin(), _operands.end(),
               [&V](const Use &use) {
                 return use.get().get() == V;
               }), _operands.end());
  }

  // clear all uses
  void Clear() { _operands.clear(); }

  // access value in current user
  Use &operator[](std::size_t pos) {
    if(_operands_num == 0) { _operands_num = _operands.size(); }
    DBG_ASSERT(pos < _operands_num, "position out of range");
    return _operands[pos];
  }

  // access value in current user (const)
  const Use &operator[](std::size_t pos) const {
    return _operands[pos];
  }

  // begin iterator
  auto begin() { return _operands.begin(); }
  auto begin() const { return _operands.begin(); }

  //end iterator
  auto end() { return _operands.end(); }
  auto end() const { return _operands.end(); }

  bool empty() const { return _operands.empty(); }
  unsigned size() const { return _operands.size(); }

  // methods for dyn_cast
  static inline bool classof(User *) { return true; }
  static inline bool classof(const User *) { return true; }
  static inline bool classof(const Value *value) {
    switch (value->classId()) {
      case ClassId::ArgRefSSAId:
      case ClassId::ConstantIntId:
      case ClassId::ConstantStringId: return false;
      default: return true;
    }
  }

};
}

#endif //LAVA_USER_H
