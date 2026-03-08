#ifndef LAVA_EXPRESSION_KEY_H
#define LAVA_EXPRESSION_KEY_H

#include <functional>
#include <optional>
#include <string>

#include "mid/ir/ssa.h"
#include "opt/expression_table.h"

namespace lava::opt {

using LeaderLookup =
    std::function<mid::SSAPtr(const mid::SSAPtr &)>;
using PureCallPredicate =
    std::function<bool(const std::shared_ptr<mid::CallInst> &)>;

std::string CanonicalTypeId(const define::TypePtr &type);

std::optional<ExprKey> BuildExprKey(const mid::SSAPtr        &value,
                                    const LeaderLookup       &leader_lookup,
                                    const PureCallPredicate  &is_pure_call);

} // namespace lava::opt

#endif // LAVA_EXPRESSION_KEY_H
