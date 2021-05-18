#ifndef LAVA_CONSTANT_H
#define LAVA_CONSTANT_H

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
  ConstantString(const std::string &str) : _str(str) {}

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  const std::string &value() const { return _str; }
};

SSAPtr GetZeroValue(define::Type type);

SSAPtr GetAllOneValue(define::Type type);

}

#endif //LAVA_CONSTANT_H