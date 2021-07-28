#ifndef LAVA_BLOCKREARRANGE_H
#define LAVA_BLOCKREARRANGE_H

#include "pass.h"

namespace lava::back {

class BlockRearrange : public PassBase {
private:
  LLBlockPtr                     _exit;
  std::vector<LLBlockPtr>        _blocks;
  std::unordered_set<LLBlockPtr> _records;

public:
  explicit BlockRearrange(LLModule &module)
  : PassBase(module), _exit(nullptr) {}

  void Reset() final {
    _exit = nullptr;
    _blocks.clear();
    _records.clear();
  }

  void runOn(const LLFunctionPtr &func) final;

  void DFS(LLBlockPtr BB);
};

}

#endif //LAVA_BLOCKREARRANGE_H
