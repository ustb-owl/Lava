#ifndef LAVA_EXPRESSION_TABLE_H
#define LAVA_EXPRESSION_TABLE_H

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "mid/ir/usedef/value.h"

namespace lava::opt {

namespace detail {

template <typename T>
inline void HashCombine(std::size_t &seed, const T &value) {
  seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

} // namespace detail

struct ConstantIntKey {
  std::string type_id;
  int         value = 0;

  bool operator==(const ConstantIntKey &) const = default;
};

struct ConstantIntKeyHash {
  std::size_t operator()(const ConstantIntKey &key) const {
    std::size_t seed = 0;
    detail::HashCombine(seed, key.type_id);
    detail::HashCombine(seed, key.value);
    return seed;
  }
};

enum class ExprKind {
  Binary,
  ICmp,
  Cast,
  Access,
  Call,
};

struct ExprKey {
  ExprKind                        kind   = ExprKind::Binary;
  unsigned                        opcode = 0;
  int                             extra  = 0;
  std::string                     result_type_id;
  const void                     *symbol = nullptr;
  std::vector<const mid::Value *> operands;

  bool operator==(const ExprKey &) const = default;
};

struct ExprKeyHash {
  std::size_t operator()(const ExprKey &key) const {
    std::size_t seed = 0;
    detail::HashCombine(seed, static_cast<int>(key.kind));
    detail::HashCombine(seed, key.opcode);
    detail::HashCombine(seed, key.extra);
    detail::HashCombine(seed, key.result_type_id);
    detail::HashCombine(seed, key.symbol);
    for (auto *operand : key.operands) {
      detail::HashCombine(seed, operand);
    }
    return seed;
  }
};

class ExprTable {
private:
  std::unordered_map<ExprKey, mid::SSAPtr, ExprKeyHash> _leaders;

public:
  void Clear() { _leaders.clear(); }

  mid::SSAPtr LookupOrInsert(const ExprKey &key, const mid::SSAPtr &leader) {
    auto [it, _] = _leaders.emplace(key, leader);
    return it->second;
  }
};

using ConstantIntTable =
    std::unordered_map<ConstantIntKey, mid::SSAPtr, ConstantIntKeyHash>;

} // namespace lava::opt

#endif // LAVA_EXPRESSION_TABLE_H
