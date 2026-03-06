#ifndef LAVA_MODULE_H
#define LAVA_MODULE_H

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "ssa.h"

using namespace lava::define;

namespace lava::mid {

using FunctionList = std::vector<FuncPtr>;

/* Module
 * Contain all information about program.
 * Record function definitions and global variables.
 */
class Module {
private:
  SSAPtrList   _global_vars;
  FunctionList _functions;
  std::string  _filename;

  template <typename T, typename... Args> auto CreateValue(Args &&...args) {
    static_assert(std::is_base_of_v<Value, T>);
    return std::make_shared<T>(std::forward<Args>(args)...);
  }

public:
  Module() { reset(); }

  void reset();

  // dump ir
  void Dump(std::ostream &os);

  void DumpCFG(const std::string &output_name);

  BlockPtr     CreateBlock(const FuncPtr &parent);
  BlockPtr     CreateBlock(const FuncPtr &parent, const std::string &name);
  BlockPtr     CreateBlock(Function *parent);
  BlockPtr     CreateBlock(Function *parent, const std::string &name);
  SSAPtr       CreateArgRef(const SSAPtr &func, std::size_t index,
                            const std::string &arg_name);
  SSAPtr       CreateConstInt(unsigned int value,
                              define::Type type = define::Type::Int32);
  ArrayPtr     CreateArray(const SSAPtrList &elems, const define::TypePtr &type,
                           const std::string &name);
  GlobalVarPtr CreateGlobalVar(bool is_var, const std::string &name,
                               const define::TypePtr &type);
  GlobalVarPtr CreateGlobalVar(bool is_var, const std::string &name,
                               const define::TypePtr &type, const SSAPtr &init);
  FuncPtr CreateFunction(const std::string &name, const define::TypePtr &type,
                         bool is_decl = false);

  SSAPtr GetZeroValue(define::Type type);

  FuncPtr GetFunction(const std::string &func_name);

  // checkers
  bool IsGlobalVariable(const SSAPtr &var) const;

  std::string File() const { return _filename; }
  void        SetFile(const std::string &file) { _filename = file; }
  bool IsFile(const std::string &file) const { return _filename == file; }

  // getters
  typedef FunctionList::iterator       iterator;
  typedef FunctionList::const_iterator const_iterator;

  SSAPtrList   &GlobalVars() { return _global_vars; }
  FunctionList &Functions() { return _functions; }

  iterator       begin() { return _functions.begin(); }
  iterator       end() { return _functions.end(); }
  const_iterator begin() const { return _functions.begin(); }
  const_iterator end() const { return _functions.end(); }
};

} // namespace lava::mid

#endif // LAVA_MODULE_H
