#include "module.h"

#include <string>

#include "common/casting.h"
#include "common/idmanager.h"
#include "mid/ir/dump_cfg.h"

using namespace lava::define;

namespace lava::mid {

void Module::reset() {
  _global_vars.clear();
  _functions.clear();
}

void Module::Dump(std::ostream &os) {
  IdManager id_mgr;

  // dump global value
  for (const auto &it : _global_vars) {
    it->Dump(os, id_mgr);
  }

  os << std::endl;

  // dump functions
  for (const auto &it : _functions) {
    it->Dump(os, id_mgr);
  }
}

FuncPtr Module::CreateFunction(const std::string     &name,
                               const define::TypePtr &type, bool is_decl) {
  DBG_ASSERT(type->IsFunction(), "not function type");
  auto func = CreateValue<Function>(name, is_decl, this);
  func->set_type(type);
  _functions.push_back(func);
  return func;
}

FuncPtr Module::GetFunction(const std::string &func_name) {
  for (const auto &it : _functions) {
    it->GetFunctionName();
    if (it->GetFunctionName() == func_name) {
      return it;
    }
  }
  return nullptr;
}

BlockPtr Module::CreateBlock(const FuncPtr &parent) {
  return CreateBlock(parent, "block");
}

BlockPtr Module::CreateBlock(const FuncPtr &parent, const std::string &name) {
  return CreateBlock(parent.get(), name);
}

BlockPtr Module::CreateBlock(Function *parent) {
  return CreateBlock(parent, "block");
}

BlockPtr Module::CreateBlock(Function *parent, const std::string &name) {
  DBG_ASSERT((parent != nullptr) && parent->type()->IsFunction(),
             "block's getParent should be function type");
  auto block = CreateValue<BasicBlock>(parent, name);
  block->set_type(nullptr);
  parent->AppendBlock(block);
  return block;
}

SSAPtr Module::CreateArgRef(const SSAPtr &func, std::size_t index,
                            const std::string &arg_name) {
  auto function = dyn_cast<Function>(func);
  DBG_ASSERT(function != nullptr, "function argument owner is null");
  // checking
  auto args_type = *function->type()->GetArgsType();
  DBG_ASSERT(index < args_type.size(), "index out of range");

  // set arg type
  auto arg_ref = CreateValue<ArgRefSSA>(function, index, arg_name);
  arg_ref->set_type(args_type[index]);
  function->set_arg(index, arg_ref);
  return arg_ref;
}

SSAPtr Module::CreateConstInt(unsigned int value, Type type) {
  auto const_int = CreateValue<ConstantInt>(value);
  const_int->set_type(MakeConst(type));
  DBG_ASSERT(const_int != nullptr, "emit const int value failed");
  return const_int;
}

GlobalVarPtr Module::CreateGlobalVar(bool is_var, const std::string &name,
                                     const lava::define::TypePtr &type) {
  return CreateGlobalVar(is_var, name, type, nullptr);
}

GlobalVarPtr Module::CreateGlobalVar(bool is_var, const std::string &name,
                                     const TypePtr &type, const SSAPtr &init) {
  DBG_ASSERT(!type->IsVoid(), "global variable shouldn't be void type");
  auto var_type = type->GetTrivialType();
  DBG_ASSERT(!init || var_type->IsIdentical(init->type()),
             "init value type is not allow for global variable");
  DBG_ASSERT(!init || init->IsConst(),
             "init value of global variable should be const");

  auto global = CreateValue<GlobalVariable>(is_var, name, init, this);
  global->set_type(MakePointer(var_type, false));
  _global_vars.push_back(global);
  return global;
}

SSAPtr Module::GetZeroValue(define::Type type) {
  return mid::GetZeroValue(type);
}

ArrayPtr Module::CreateArray(const SSAPtrList &elems, const TypePtr &type,
                             const std::string &name) {
  // type checking
  DBG_ASSERT(type->IsArray() && type->GetLength() == elems.size(),
             "init values of array are not fit");

  auto array_ty = type->GetTrivialType();
  for (const auto &it : elems) {
    DBG_ASSERT(it->IsConst(), "init value should be const");
    DBG_ASSERT(array_ty->GetDerefedType()->GetSize() == it->type()->GetSize(),
               "init value type not fit");
    if (it)
      continue;
  }

  // create constant array
  auto const_array = CreateValue<ConstantArray>(elems, name);
  const_array->set_type(MakePointer(array_ty));
  return const_array;
}

bool Module::IsGlobalVariable(const SSAPtr &var) const {
  for (const auto &it : _global_vars) {
    if (var == it)
      return true;
  }
  return false;
}

void Module::DumpCFG(const std::string &output_name) {
#ifdef ENABLE_CFG
  IdManager id_mgr;
  GVC_t    *gvc = gvContext();
  graph_t  *g   = agopen((char *)"g", Agdirected, nullptr);

  MakeGlobalVariables(g, this, id_mgr);
  lava::opt::PassManager::SetModule(*this);

  for (const auto &func : _functions) {
    static_cast<void>(lava::opt::PassManager::RequireAnalysisResultOnFunction<
                      lava::opt::DomInfo>("DominanceInfo", func));
    MakeCFG(g, func, id_mgr);
  }

  gvLayout(gvc, g, "dot");
  //  gvRender(gvc, g, "dot", stdout);
  gvRenderFilename(gvc, g, "pdf", (output_name + ".pdf").c_str());
  gvRenderFilename(gvc, g, "png", (output_name + ".png").c_str());
  gvRenderFilename(gvc, g, "svg", (output_name + ".svg").c_str());

  gvFreeLayout(gvc, g);
  agclose(g);
  gvFreeContext(gvc);
#else
  ERROR("Build with graphviz please");
#endif
}

} // namespace lava::mid
