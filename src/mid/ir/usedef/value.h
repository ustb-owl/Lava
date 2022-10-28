#ifndef LAVA_VALUE_H
#define LAVA_VALUE_H

#include <ostream>

#include "define/type.h"
#include "front/logger.h"
#include "common/classid.h"
#include "mid/ir/usedef/use.h"

namespace lava {
  class IdManager;
}

namespace lava::mid {


class Constant;
class Instruction;
class BasicBlock;
class Function;
class BinaryOperator;
class GlobalVariable;
class ConstantArray;

// type aliases
using FuncPtr       = std::shared_ptr<Function>;
using InstPtr       = std::shared_ptr<Instruction>;
using BlockPtr      = std::shared_ptr<BasicBlock>;
using ArrayPtr      = std::shared_ptr<ConstantArray>;
using BinaryPtr     = std::shared_ptr<BinaryOperator>;
using GlobalVarPtr  = std::shared_ptr<GlobalVariable>;
using Blocks        = std::vector<BlockPtr>;

class Value {
private:
  define::TypePtr   _type;
  front::LoggerPtr  _logger;
  BlockPtr          _parent;  // block
  UseList           _use_list;
  ClassId           _class_id;

public:
  explicit Value(ClassId classId) : _class_id(classId) {};
  virtual ~Value() = default;

  void addUse(Use *U) { _use_list.emplace_back(U); }

  void removeUse(Use *U) { _use_list.remove(U); }

  void set_logger(const front::LoggerPtr &logger) { _logger = logger;}

  void set_type(const define::TypePtr &type) { _type = type; }

  virtual const BlockPtr &GetParent() const { return _parent; }
  virtual void SetParent(const BlockPtr &BB) { _parent = BB; }

  void ReplaceBy(const SSAPtr &value) {
    if (value.get() == this) return;
    // reroute all uses to new value
    while (!_use_list.empty()) {
      _use_list.front()->set(value);
    }
  }

  void RemoveFromUser();

  // get address value of current value
  virtual SSAPtr GetAddr() const { return nullptr; }

  virtual bool IsConst() const { return false; }

  virtual bool IsArgument() const { return false; }

  // dump the content of SSA value to output stream
  virtual void Dump(std::ostream &os, IdManager &id_mgr) const = 0;

  // DEBUG method: dump DEBUG IR info
  void dump() const;
  void AssignId(IdManager &id_mgr) const;

//   dump to C code
//  virtual void DumpToC(std::ostream &os)

  virtual bool isInstruction() const { return false; }
  virtual bool isBlock() const { return false; }
  const UseList &uses() const { return _use_list; }
  const front::LoggerPtr &logger() const { return _logger; }
  const define::TypePtr &type() const { return _type; }

  // methods for dyn_cast
  static inline bool classof(Value *) { return true; }
  static inline bool classof(const Value *) { return true; }

  ClassId classId() const { return _class_id; }

  // check if this object is User
  virtual bool isUser() const { return false; }
};
};

#endif //LAVA_VALUE_H
