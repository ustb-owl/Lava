#include "define/type.h"

#include <sstream>
#include <utility>
#include <stack>
#include <cassert>

#include "lib/guard.h"

using namespace lava::define;

namespace {

// used in 'StructType::IsIdentical' to prevent infinite loop
std::stack<std::pair<const void *, const void *>> ident_types;
// used in 'StructType::GetTrivialType' to prevent infinite loop
std::stack<std::pair<const void *, TypePtr>> trivial_types;

}  // namespace

// definition of static member variables in 'BaseType'
// default to 32-bit
std::size_t BaseType::ptr_size_ = 4;

bool PrimType::CanAccept(const TypePtr &type) const {
  if (is_right_ || IsVoid()) return false;
  return type->IsInteger();
}

bool PrimType::CanCastTo(const TypePtr &type) const {
  if (IsVoid()) return false;
  return type->IsInteger() || type->IsPointer();
}

bool PrimType::IsIdentical(const TypePtr &type) const {
  if (IsVoid() && type->IsVoid()) return true;
  if (IsInteger() && type->IsInteger()) {
    return IsUnsigned() == type->IsUnsigned() &&
           GetSize() == type->GetSize();
  }
  return false;
}

// bits
std::size_t PrimType::GetSize() const {
  switch (type_) {
    case Type::Bool: return 1;
    case Type::Int8: case Type::UInt8: return 8;
    case Type::Int32: case Type::UInt32: return 32;
    default: return 0;
  }
}

std::string PrimType::GetTypeId() const {
  switch (type_) {
    case Type::Void: return "void";
    case Type::Bool: return "i1";
    case Type::UInt8:
    case Type::Int8: return "i8";
    case Type::UInt32:
    case Type::Int32: return "i32";
    default: return "";
  }
}

TypePtr PrimType::GetValueType(bool is_right) const {
  return std::make_shared<PrimType>(type_, is_right);
}

void StructType::CalcSize() {
  std::size_t sum = 0, max_base_size = 1;
  for (const auto &[_, t] : elems_) {
    sum += t->GetSize();
    // update 'max_base_size'
    auto base_size = t->GetAlignSize();
    if (base_size > max_base_size) max_base_size = base_size;
  }
  size_ = (((sum - 1) / max_base_size) + 1) * max_base_size;
  base_size_ = max_base_size;
}

bool StructType::CanAccept(const TypePtr &type) const {
  return !is_right_ && IsIdentical(type);
}

bool StructType::IsIdentical(const TypePtr &type) const {
  // check if is in recursion
  if (!ident_types.empty()) {
    const auto &[t1, t2] = ident_types.top();
    if ((t1 == this && t2 == type.get()) ||
        (t2 == this && t1 == type.get())) {
      return true;
    }
  }
  // prevent infinite loop
  ident_types.push({this, type.get()});
  auto pop = xstl::Guard([] { ident_types.pop(); });
  // check if is identical
  if (!type->IsStruct()) return false;
  if (elems_.size() != type->GetLength()) return false;
  for (std::size_t i = 0; i < elems_.size(); ++i) {
    if (!elems_[i].second->IsIdentical(type->GetElem(i))) return false;
  }
  return true;
}

TypePtr StructType::GetElem(const std::string &name) const {
  for (const auto &[n, t] : elems_) if (name == n) return t;
  return nullptr;
}

std::optional<std::size_t> StructType::GetElemIndex(
    const std::string &name) const {
  for (std::size_t i = 0; i < elems_.size(); ++i) {
    if (elems_[i].first == name) return i;
  }
  return {};
}

TypePtr StructType::GetValueType(bool is_right) const {
  return std::make_shared<StructType>(elems_, id_, is_right);
}

TypePtr StructType::GetTrivialType() const {
  // check if is in recursion
  if (!trivial_types.empty() && trivial_types.top().first == this) {
    return trivial_types.top().second;
  }
  // initialize as an empty struct type
  TypePairList elems;
  auto type = std::make_shared<StructType>(elems, id_, false);
  trivial_types.push({this, type});
  // convert elements
  for (const auto &i : elems_) {
    elems.push_back({i.first, i.second->GetTrivialType()});
  }
  // update type
  type->set_elems(std::move(elems));
  trivial_types.pop();
  return type;
}

TypePtr ConstType::GetElem(std::size_t index) const {
  auto type = type_->GetElem(index);
  return std::make_shared<ConstType>(std::move(type));
}

TypePtr ConstType::GetElem(const std::string &name) const {
  auto type = type_->GetElem(name);
  if (!type) return nullptr;
  return std::make_shared<ConstType>(std::move(type));
}

TypePtr ConstType::GetValueType(bool is_right) const {
  auto type = type_->GetValueType(is_right);
  return std::make_shared<ConstType>(std::move(type));
}

bool FuncType::CanAccept(const TypePtr &type) const {
  return !is_right_ && IsIdentical(type);
}

bool FuncType::CanCastTo(const TypePtr &type) const {
  return type->IsInteger() || type->IsPointer();
}

bool FuncType::IsIdentical(const TypePtr &type) const {
  if (!type->IsFunction()) return false;
  auto ret = type->GetReturnType(args_);
  return ret ? ret_->IsIdentical(ret) : false;
}

std::size_t FuncType::GetSize() const {
  return ptr_size();
}

TypePtr FuncType::GetReturnType(const TypePtrList &args) const {
  if (args_.size() != args.size()) return nullptr;
  for (std::size_t i = 0; i < args_.size(); ++i) {
    if (!args_[i]->IsIdentical(args[i]) && !args_[i]->CanAccept(args[i])) {
      return nullptr;
    }
    if (args_[i]->IsPointer() && args[i]->IsPointer()) {
      // check pointer's const cast
      // NOTE: stricter than C
      if (args[i]->GetDerefedType()->IsConst() &&
          !args_[i]->GetDerefedType()->IsConst()) {
        return nullptr;
      }
    }
  }
  return ret_;
}

std::string FuncType::GetTypeId() const {
  std::ostringstream oss;
  oss << '$' << args_.size() << 'f';
  for (const auto &i : args_) oss << i->GetTypeId();
  oss << '$';
  oss << ret_->GetTypeId();
  return oss.str();
}

TypePtr FuncType::GetValueType(bool is_right) const {
  return std::make_shared<FuncType>(args_, ret_, is_right);
}

TypePtr FuncType::GetTrivialType() const {
  TypePtrList args;
  for (const auto &i : args_) args.push_back(i->GetTrivialType());
  return std::make_shared<FuncType>(std::move(args),
                                    ret_->GetTrivialType(), false);
}

bool ArrayType::CanAccept(const TypePtr &type) const {
  // array can not be assigned in C/C++
  return false;
}

bool ArrayType::CanCastTo(const TypePtr &type) const {
  return !is_right_ && (type->IsInteger() || type->IsPointer());
}

bool ArrayType::IsIdentical(const TypePtr &type) const {
  return type->IsArray() && base_->IsIdentical(type->GetDerefedType()) &&
         len_ == type->GetLength();
}

std::string ArrayType::GetTypeId() const {
  std::ostringstream oss;
  oss << "[" << len_ << " x ";
  oss << base_->GetTypeId();
  oss << "]";
  return oss.str();
}

TypePtr ArrayType::GetValueType(bool is_right) const {
  return std::make_shared<ArrayType>(base_, len_, is_right);
}

TypePtr ArrayType::GetTrivialType() const {
  return std::make_shared<ArrayType>(base_->GetTrivialType(), len_, false);
}

bool PointerType::CanAccept(const TypePtr &type) const {
  if (!type->IsPointer() && !type->IsArray()) return false;
  auto deref = type->GetDerefedType();
  // check if is const pointer
  // NOTE: stricter than C
  if (!base_->IsConst() && deref->IsConst()) return false;
  // pointers can accept other void pointers in C/C++
  return !is_right_ && (deref->IsVoid() || base_->IsIdentical(deref));
}

bool PointerType::CanCastTo(const TypePtr &type) const {
  // pointer can be casted to any const/non-const pointers in C/C++
  return type->IsInteger() || type->IsPointer();
}

bool PointerType::IsIdentical(const TypePtr &type) const {
  return type->IsPointer() && base_->IsIdentical(type->GetDerefedType());
}

std::size_t PointerType::GetSize() const {
  return ptr_size();
}

std::string PointerType::GetTypeId() const {
  std::ostringstream oss;
  oss << base_->GetTypeId() << "*";
  return oss.str();
}

TypePtr PointerType::GetValueType(bool is_right) const {
  return std::make_shared<PointerType>(base_, is_right);
}

TypePtr PointerType::GetTrivialType() const {
  return std::make_shared<PointerType>(base_->GetTrivialType(), false);
}
