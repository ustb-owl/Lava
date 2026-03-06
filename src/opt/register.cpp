#include "opt/register.h"

namespace lava::opt {

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
void RegisterGlobalValueNumberingPass();
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
  RegisterGlobalValueNumberingPass();
  RegisterLocalMemoryPropagationPass();
  RegisterLoopInvariantHoistPass();
  RegisterMem2RegPass();
  RegisterSanitizeIRPass();
  RegisterStrengthReductionPass();
  RegisterTailRecursionPass();
}

}
