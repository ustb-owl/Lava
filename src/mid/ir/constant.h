#ifndef LAVA_CONSTANT_H
#define LAVA_CONSTANT_H

#include <utility>

#include "mid/ir/usedef/user.h"
#include "define/type.h"


namespace lava::mid {
using lava::define::PrimType;

// Constant int value ssa
class ConstantInt : public Value {
private:
  unsigned int _value;

public:
  explicit ConstantInt(unsigned int value) : _value(value) {}

  bool IsConst() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  unsigned int value() const { return _value; }

  bool IsZero() const { return _value == 0; }
};

// Constant string value ssa
class ConstantString : public Value {
private:
  std::string _str;

public:
  explicit ConstantString(std::string str) : _str(std::move(str)) {}

  bool IsConst() const override { return true; }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  const std::string &value() const { return _str; }
};


// constant array value ssa
// operands: elem1, elem2, ...
class ConstantArray : public User {
private:
  std::string _name;
public:
  explicit ConstantArray(const SSAPtrList &elems, std::string name)
  : _name(std::move(name)) {
    for (const auto &it : elems) AddValue(it);
  }

  bool IsConst() const override { return true; }

  void Dump(std::ostream &os, IdManager &id_mgr) const override;
};

SSAPtr GetZeroValue(define::Type type);

SSAPtr GetAllOneValue(define::Type type);

}
#endif //LAVA_CONSTANT_H
