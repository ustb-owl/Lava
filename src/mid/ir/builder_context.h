#ifndef LAVA_BUILDER_CONTEXT_H
#define LAVA_BUILDER_CONTEXT_H

#include <deque>
#include <stack>
#include <string>
#include <unordered_map>

#include "define/ast.h"
#include "lib/guard.h"
#include "lib/nestedmap.h"
#include "module.h"

namespace lava::mid {

using ValueEnvPtr = lib::Nested::NestedMapPtr<std::string, SSAPtr>;
using BreakContPair =
    std::pair<BlockPtr,
              BlockPtr>; // pair for storing target block of break & continue

class IRBuilderContext {
private:
  Module                                 *_module;
  int                                     _array_id;
  SSAPtr                                  _return_val;
  BlockPtr                                _func_entry;
  BlockPtr                                _func_exit;
  BlockPtr                                _insert_point;
  ValueEnvPtr                             _value_symtab;
  InstList::iterator                      _insert_pos;
  std::stack<front::LoggerPtr>            _loggers;
  std::stack<BreakContPair>               _break_cont;
  std::deque<int>                         _array_lens;
  std::unordered_map<std::string, SSAPtr> _origin_array;

  template <typename T> void ApplyLogger(const std::shared_ptr<T> &ssa) {
    if (!_loggers.empty())
      ssa->set_logger(_loggers.top());
  }

public:
  explicit IRBuilderContext(Module &module) : _module(&module) { reset(); }

  void reset();

  template <typename T, typename... Args> auto MakeSSA(Args &&...args) {
    static_assert(std::is_base_of_v<Value, T>);
    auto ssa = std::make_shared<T>(std::forward<Args>(args)...);
    ApplyLogger(ssa);
    return ssa;
  }

  template <typename T, typename... Args> auto AddInst(Args &&...args) {
    auto inst   = MakeSSA<T>(std::forward<Args>(args)...);
    _insert_pos = ++_insert_point->InsertInst(_insert_pos, inst);
    return inst;
  }

  Module       &module() { return *_module; }
  const Module &module() const { return *_module; }

  xstl::Guard NewEnv();
  xstl::Guard SetContext(const front::Logger &logger);
  xstl::Guard SetContext(const front::LoggerPtr &logger);

  BlockPtr CreateBlock(const FuncPtr &parent);
  BlockPtr CreateBlock(const FuncPtr &parent, const std::string &name);
  BlockPtr CreateBlock(Function *parent);
  BlockPtr CreateBlock(Function *parent, const std::string &name);

  SSAPtr CreateJump(const BlockPtr &target);
  SSAPtr CreateStore(const SSAPtr &V, const SSAPtr &P);
  SSAPtr CreateArgRef(const SSAPtr &func, std::size_t index,
                      const std::string &arg_name);
  SSAPtr CreateAlloca(const define::TypePtr &type);
  SSAPtr CreateReturn(const SSAPtr &value);
  SSAPtr CreateLoad(const SSAPtr &ptr);
  SSAPtr CreateBranch(const SSAPtr &cond, const BlockPtr &true_block,
                      const BlockPtr &false_block);
  SSAPtr CreateBinaryOperator(define::BinaryStmt::Operator opcode,
                              const SSAPtr &S1, const SSAPtr &S2);
  SSAPtr CreatePureBinaryInst(Instruction::BinaryOps opcode, const SSAPtr &S1,
                              const SSAPtr &S2);
  SSAPtr CreateAssign(const SSAPtr &S1, const SSAPtr &S2);
  SSAPtr CreateConstInt(unsigned int value,
                        define::Type type = define::Type::Int32);
  SSAPtr CreateCallInst(const SSAPtr &callee, const std::vector<SSAPtr> &args);
  SSAPtr CreateICmpInst(define::BinaryStmt::Operator opcode, const SSAPtr &lhs,
                        const SSAPtr &rhs);
  SSAPtr CreateCastInst(const SSAPtr &operand, const define::TypePtr &type);
  SSAPtr CreateElemAccess(const SSAPtr &ptr, const SSAPtrList &index);
  ArrayPtr     CreateArray(const SSAPtrList &elems, const define::TypePtr &type,
                           const std::string &name);
  GlobalVarPtr CreateGlobalVar(bool is_var, const std::string &name,
                               const define::TypePtr &type);
  GlobalVarPtr CreateGlobalVar(bool is_var, const std::string &name,
                               const define::TypePtr &type, const SSAPtr &init);
  FuncPtr CreateFunction(const std::string &name, const define::TypePtr &type,
                         bool is_decl = false);

  SSAPtr GetZeroValue(define::Type type);

  FuncPtr     GetFunction(const std::string &func_name);
  SSAPtr      GetValues(const std::string &var_name);
  std::string GetArrayName();

  bool IsGlobalVariable(const SSAPtr &var) const;

  void SetRetValue(const SSAPtr &val) { _return_val = val; }
  void SetFuncEntry(const BlockPtr &BB) { _func_entry = BB; }
  void SetFuncExit(const BlockPtr &BB) { _func_exit = BB; }

  void SetInsertPoint(const BlockPtr &BB) {
    SetInsertPoint(BB, BB->insts().end());
  }

  void SetInsertPoint(const BlockPtr &BB, InstList::iterator it) {
    _insert_point = BB;
    _insert_pos   = it;
  }

  void SetArrayLens(std::deque<int> &array_lens) { _array_lens = array_lens; }

  void ReplaceValue(const std::string &id, const SSAPtr &ptr) {
    _value_symtab->Replace(id, ptr);
  }

  void SaveOriginArray(const std::string &id, const SSAPtr &ptr) {
    _origin_array.insert(std::make_pair(id, ptr));
  }

  void RecoverArrays() {
    for (const auto &it : _origin_array) {
      _value_symtab->Replace(it.first, it.second);
    }
    _origin_array.clear();
  }

  ValueEnvPtr               &ValueSymTab() { return _value_symtab; }
  SSAPtr                    &ReturnValue() { return _return_val; }
  BlockPtr                  &InsertPoint() { return _insert_point; }
  BlockPtr                  &FuncEntry() { return _func_entry; }
  BlockPtr                  &FuncExit() { return _func_exit; }
  InstList::iterator         InsertPos() { return _insert_pos; }
  std::deque<int>           &array_lens() { return _array_lens; }
  std::stack<BreakContPair> &BreakCont() { return _break_cont; }
};

} // namespace lava::mid

#endif // LAVA_BUILDER_CONTEXT_H
