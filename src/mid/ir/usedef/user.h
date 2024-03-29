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
      AddValue(it.value());
    }
  }

  unsigned operandNum() const { return _operands.size(); }

  const Operands &GetOperands() const {
    return _operands;
  }

  void SetOperandNum(std::size_t size) {
    _operands_num = size;
  }

  virtual SSAPtr GetOperand(unsigned i) const {
    DBG_ASSERT(i < _operands_num, "getOperand() out of range");
    return _operands[i].value();
  }

  virtual void SetOperand(unsigned i, const SSAPtr& V) {
    DBG_ASSERT(i < _operands_num, "setOperand() out of range");
    _operands[i].set(V);
  }

  void AddValue(const SSAPtr &V) {
//    DBG_ASSERT((_operands.size() < _operands_num) || ( _operands_num == 0), " out of range");
    _operands.push_back(Use(V, this));
    _operands_num = _operands.size();
  }

  void RemoveValue(const SSAPtr& V) {
    RemoveValue(V.get());
    _operands_num = _operands.size();
  }

  void RemoveValue(Value *V) {
    _operands.erase(
        std::remove_if(_operands.begin(), _operands.end(),
              [&V](const Use &use) {
                  return use.value().get() == V;
              }),_operands.end());
    _operands_num = _operands.size();
  }

  void RemoveValue(unsigned idx) {
    DBG_ASSERT(idx < size(), "idx out of bound");
    auto it = _operands.begin();
    for (unsigned i = 0; i < idx; i++) it++;
    DBG_ASSERT(it != _operands.end(), "it is out of bound");
    _operands.erase(it);
    _operands_num = _operands.size();
  }

  void Reserve() {
    for (unsigned int i = 0; i < _operands_num; i++) {
      _operands.push_back(Use(nullptr, this));
    }
  }

  // clear all uses
  void Clear() { _operands.clear(); }

  // access value in current user
  Use &operator[](std::size_t pos) {
    _operands_num = _operands.size();
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

  Use &back() { return _operands.back(); }
  const Use &back() const { return _operands.back(); }

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

  bool isUser() const override { return true; }

  // Dump method
  void Dump(std::ostream &os, IdManager &id_mgr) const override {
    DBG_ASSERT(false, "Should not reach here!");
  }

};
}

#endif //LAVA_USER_H
