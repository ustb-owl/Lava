#include "driver/driver.h"

#include <fstream>
#include <iostream>
#include <memory>

#include "driver/compiler.h"
#include "driver/pipeline.h"
#include "opt/pass_manager.h"
#include "opt/register.h"

namespace lava::driver {

namespace {

std::ostream *OpenOutputStream(const DriverOptions            &options,
                               std::unique_ptr<std::ofstream> &owned_stream) {
  if (options.output_file.empty() || options.emit == EmitKind::Cfg) {
    return &std::cout;
  }

  owned_stream = std::make_unique<std::ofstream>(
      options.output_file, std::fstream::out | std::fstream::trunc);
  if (!owned_stream->is_open()) {
    return nullptr;
  }
  return owned_stream.get();
}

} // namespace

int RunDriver(const DriverOptions &options) {
  opt::RegisterAllMiddleEndPasses();
  opt::PassManager::Initialize();

  if (options.list_passes) {
    PrintMiddleEndPasses(std::cout);
  }

  if (options.print_pipeline) {
    opt::PassPtrList pipeline;
    std::string      error;
    if (!BuildMiddleEndPipeline(options, pipeline, error)) {
      std::cerr << "error: " << error << '\n';
      return 1;
    }
    PrintMiddleEndPipeline(std::cout, pipeline);
  }

  if (options.list_passes || options.print_pipeline) {
    return 0;
  }

  if (options.input_file.empty()) {
    std::cerr << "error: no input file provided\n";
    return 1;
  }

  std::unique_ptr<std::ofstream> owned_stream;
  auto                          *os = OpenOutputStream(options, owned_stream);
  if (os == nullptr) {
    std::cerr << "error: failed to open output file '" << options.output_file
              << "'\n";
    return 1;
  }

  std::ifstream ifs(options.input_file);
  if (!ifs.is_open()) {
    std::cerr << "error: failed to open input file '" << options.input_file
              << "'\n";
    return 1;
  }

  Compiler compiler;
  compiler.SetFile(options.input_file);
  compiler.Open(&ifs);
  compiler.Parse();

  if (options.emit == EmitKind::Ast) {
    if (HasMiddleEndExecutionControls(options)) {
      std::cerr << "error: middle-end pass controls require IR execution and "
                   "cannot be used with --emit=ast\n";
      return 1;
    }
    compiler.ast()->Dump(*os);
    return 0;
  }

  compiler.EmitIR();
  std::string error;
  if (!RunMiddleEndPipeline(compiler, options, std::cerr, error)) {
    std::cerr << "error: " << error << '\n';
    return 1;
  }

  switch (options.emit) {
  case EmitKind::Ast:
    return 0;
  case EmitKind::Ir:
    compiler.DumpIR(*os);
    return 0;
  case EmitKind::Asm:
    compiler.CodeGeneAction(options.no_ra);
    compiler.DumpASM(*os);
    return 0;
  case EmitKind::Cfg:
    compiler.DumpCFG(options.output_file.empty() ? "ir" : options.output_file);
    return 0;
  }

  std::cerr << "error: unsupported emit mode\n";
  return 1;
}

} // namespace lava::driver
