#include "opt/register.h"

#include <unordered_map>
#include <utility>

namespace {

std::vector<lava::opt::PassCliMetadata> &PassCliMetadataTable() {
  static std::vector<lava::opt::PassCliMetadata> table;
  return table;
}

std::unordered_map<std::string, std::size_t> &PassCliMetadataIndex() {
  static std::unordered_map<std::string, std::size_t> index;
  return index;
}

} // namespace

namespace lava::opt {

void RegisterPassCliMetadata(PassCliMetadata metadata) {
  auto &table = PassCliMetadataTable();
  auto &index = PassCliMetadataIndex();

  auto [it, inserted] = index.emplace(metadata.internal_name, table.size());
  if (inserted) {
    table.push_back(std::move(metadata));
    return;
  }

  table[it->second] = std::move(metadata);
}

const std::vector<PassCliMetadata> &GetPassCliMetadata() {
  return PassCliMetadataTable();
}

void RegisterNeedGcmPass();
void RegisterLoopInfoPass();
void RegisterDominanceInfoPass();
void RegisterFunctionInfoPass();
void RegisterPostDominanceInfoPass();

void RegisterBlockSimplificationPass();
void RegisterDeadCodeEliminationPass();
void RegisterDeadGlobalCodeEliminationPass();
void RegisterDirtyArrayConvertPass();
void RegisterDirtyFunctionNameConvertPass();
void RegisterFunctionInliningPasses();
void RegisterGlobalConstPropagationPass();
void RegisterLocalValueNumberingPass();
void RegisterInstSimplifyPass();
void RegisterLocalMemoryPropagationPass();
void RegisterLoopInvariantHoistPass();
void RegisterMem2RegPass();
void RegisterSanitizeIRPass();
void RegisterStrengthReductionPass();
void RegisterTailRecursionPass();

void RegisterAllMiddleEndPasses() {
  RegisterNeedGcmPass();
  RegisterLoopInfoPass();
  RegisterDominanceInfoPass();
  RegisterFunctionInfoPass();
  RegisterPostDominanceInfoPass();

  RegisterBlockSimplificationPass();
  RegisterDeadCodeEliminationPass();
  RegisterDeadGlobalCodeEliminationPass();
  RegisterDirtyArrayConvertPass();
  RegisterDirtyFunctionNameConvertPass();
  RegisterFunctionInliningPasses();
  RegisterGlobalConstPropagationPass();
  RegisterLocalValueNumberingPass();
  RegisterInstSimplifyPass();
  RegisterLocalMemoryPropagationPass();
  RegisterLoopInvariantHoistPass();
  RegisterMem2RegPass();
  RegisterSanitizeIRPass();
  RegisterStrengthReductionPass();
  RegisterTailRecursionPass();
}

} // namespace lava::opt
