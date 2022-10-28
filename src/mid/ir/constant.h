#ifndef LAVA_CONSTANT_H
#define LAVA_CONSTANT_H

#include <utility>

#include "mid/ir/usedef/user.h"
#include "define/type.h"


namespace lava::mid {

using lava::define::PrimType;

class ConstantValue : public User {
public:
  explicit ConstantValue(ClassId id) : User(id) {}

  bool IsConst() const final { return true; }

  virtual SSAPtr Copy() const = 0;
};

// Constant int value ssa
class ConstantInt : public ConstantValue {
private:
  int _value;

public:
  explicit ConstantInt(unsigned int value) : ConstantValue(ClassId::ConstantIntId), _value(value) {}

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  int value() const { return _value; }

  bool IsZero() const { return _value == 0; }

  SSAPtr Copy() const final {
    auto res = std::make_shared<ConstantInt>(_value);
    res->set_type(type());
    return res;
  }

  static inline bool classof(ConstantInt *) { return true; }

  static inline bool classof(const ConstantInt *) { return true; }

  static bool classof(Value *value) {
    return value->classId() == ClassId::ConstantIntId;
  }

  static bool classof(const Value *value) {
    return value->classId() == ClassId::ConstantIntId;
  }
};

// Constant string value ssa
class ConstantString : public ConstantValue {
private:
  std::string _str;

public:
  explicit ConstantString(std::string str)
      : ConstantValue(ClassId::ConstantStringId), _str(std::move(str)) {}

  SSAPtr Copy() const final {
    auto res = std::make_shared<ConstantString>(_str);
    res->set_type(type());
    return res;
  }

  // dump ir
  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  const std::string &value() const { return _str; }
};


// constant array value ssa
// operands: elem1, elem2, ...
class ConstantArray : public ConstantValue {
private:
  std::string _name;
public:
  explicit ConstantArray(const SSAPtrList &elems, std::string name)
      : ConstantValue(ClassId::ConstantArrayId), _name(std::move(name)) {
    for (const auto &it : elems) AddValue(it);
  }

  SSAPtr Copy() const final {
    SSAPtrList vals;
    for (const auto &it : *this) {
      DBG_ASSERT(it.value()->IsConst(), "the element of constant array should be constant value");
      vals.push_back(std::static_pointer_cast<ConstantValue>(it.value())->Copy());
    }
    DBG_ASSERT(vals.size() == size(), "the size of the new constant array is wrong");
    auto res = std::make_shared<ConstantArray>(vals, _name);
    res->set_type(type());
    return res;
  }

  void Dump(std::ostream &os, IdManager &id_mgr) const override;

  void Dump(std::ostream &os, IdManager &id_mgr, const std::string &separator) const;
};

SSAPtr GetZeroValue(define::Type type);

SSAPtr GetAllOneValue(define::Type type);

}
#endif //LAVA_CONSTANT_H
