#ifndef LAVA_OPT_REGISTER_H
#define LAVA_OPT_REGISTER_H

#include <string>
#include <vector>

namespace lava::opt {

struct PassCliMetadata {
  std::string internal_name;
  std::string cli_name;
  std::vector<std::string> aliases;
  std::string description;
};

void RegisterPassCliMetadata(PassCliMetadata metadata);

const std::vector<PassCliMetadata> &GetPassCliMetadata();

void RegisterAllMiddleEndPasses();

}

#endif // LAVA_OPT_REGISTER_H
