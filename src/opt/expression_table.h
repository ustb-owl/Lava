#ifndef LAVA_EXPRESSION_TABLE_H
#define LAVA_EXPRESSION_TABLE_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "lib/debug.h"
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

using ExprId = std::uint32_t;
inline constexpr ExprId kInvalidExprId = std::numeric_limits<ExprId>::max();

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

struct ExprTableEntry {
  ExprId      id        = kInvalidExprId;
  ExprKey     key;
  mid::SSAPtr canonical = nullptr;
};

class ExprTable {
private:
  std::unordered_map<ExprKey, ExprId, ExprKeyHash> _expr_ids;
  std::vector<ExprTableEntry>                       _entries;

public:
  void Clear() {
    _expr_ids.clear();
    _entries.clear();
  }

  bool empty() const { return _entries.empty(); }

  std::size_t size() const { return _entries.size(); }

  std::optional<ExprId> LookupId(const ExprKey &key) const {
    auto it = _expr_ids.find(key);
    if (it == _expr_ids.end())
      return std::nullopt;
    return it->second;
  }

  ExprId LookupOrInsertId(const ExprKey &key, const mid::SSAPtr &canonical) {
    DBG_ASSERT(_entries.size() < kInvalidExprId, "too many expressions");
    auto next_id = static_cast<ExprId>(_entries.size());
    auto [it, inserted] = _expr_ids.emplace(key, next_id);
    if (inserted) {
      _entries.push_back({next_id, key, canonical});
    }
    return it->second;
  }

  mid::SSAPtr LookupOrInsert(const ExprKey &key, const mid::SSAPtr &canonical) {
    return GetCanonical(LookupOrInsertId(key, canonical));
  }

  const ExprTableEntry &Get(ExprId id) const {
    DBG_ASSERT(id < _entries.size(), "expression id out of range");
    return _entries[id];
  }

  const ExprKey &GetKey(ExprId id) const { return Get(id).key; }

  const mid::SSAPtr &GetCanonical(ExprId id) const { return Get(id).canonical; }

  const std::vector<ExprTableEntry> &entries() const { return _entries; }
};

using ConstantIntTable =
    std::unordered_map<ConstantIntKey, mid::SSAPtr, ConstantIntKeyHash>;

} // namespace lava::opt

#endif // LAVA_EXPRESSION_TABLE_H
