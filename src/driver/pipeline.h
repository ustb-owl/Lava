#ifndef LAVA_DRIVER_PIPELINE_H
#define LAVA_DRIVER_PIPELINE_H

#include <ostream>
#include <string>
#include <vector>

#include "driver/compiler.h"
#include "driver/options.h"
#include "opt/pass_manager.h"

namespace lava::driver {

struct PassCliMetadata {
  std::string internal_name;
  std::string cli_name;
  std::vector<std::string> aliases;
  std::string description;
};

void EnsureMiddleEndPassesLinked();

std::vector<PassCliMetadata> GetMiddleEndPassMetadata();

bool BuildMiddleEndPipeline(const DriverOptions &options,
                            opt::PassPtrList &pipeline,
                            std::string &error);

void PrintMiddleEndPasses(std::ostream &os);

void PrintMiddleEndPipeline(std::ostream &os, const opt::PassPtrList &pipeline);

bool HasMiddleEndExecutionControls(const DriverOptions &options);

bool RunMiddleEndPipeline(Compiler &compiler,
                          const DriverOptions &options,
                          std::ostream &diag,
                          std::string &error);

}

#endif // LAVA_DRIVER_PIPELINE_H
