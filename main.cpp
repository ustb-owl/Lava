#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

#include "driver/driver.h"
#include "driver/options.h"
#include "lib/CLI11.hpp"

namespace {

using lava::driver::DriverOptions;
using lava::driver::EmitKind;

std::string LowerCase(std::string value) {
  std::ranges::transform(value, value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

void SetEmitKindOrThrow(DriverOptions &options,
                        EmitKind emit,
                        bool &emit_selected,
                        std::string_view source) {
  if (emit_selected && options.emit != emit) {
    throw CLI::ValidationError(std::string(source), "multiple emit modes specified");
  }
  options.emit = emit;
  emit_selected = true;
}

}

int main(int argc, char **argv) {
  CLI::App app{"Lava compiler"};
  DriverOptions options;
  bool emit_selected = false;
  std::string emit_name;
  int opt_level = 0;

  app.add_option("input", options.input_file, "input filename")
      ->check(CLI::ExistingFile);

  app.add_option("-o,--output", options.output_file, "set output file name");

  app.add_option("--emit", emit_name, "emit ast, ir, asm, or cfg")
      ->check(CLI::IsMember({"ast", "ir", "asm", "cfg"}, CLI::ignore_case));

  app.add_flag_callback("-T,--dump-ast",
                        [&] { SetEmitKindOrThrow(options, EmitKind::Ast, emit_selected, "--dump-ast"); },
                        "dump AST");
  app.add_flag_callback("-I,--dump-ir",
                        [&] { SetEmitKindOrThrow(options, EmitKind::Ir, emit_selected, "--dump-ir"); },
                        "dump LLVM IR");
  app.add_flag_callback("-S,--dump-asm",
                        [&] { SetEmitKindOrThrow(options, EmitKind::Asm, emit_selected, "--dump-asm"); },
                        "dump assembly");
  app.add_flag_callback("-C,--dump-cfg",
                        [&] { SetEmitKindOrThrow(options, EmitKind::Cfg, emit_selected, "--dump-cfg"); },
                        "dump CFG");

  app.add_option("-O,--opt-level", opt_level, "optimization level")
      ->check(CLI::Range(0, 2))
      ->capture_default_str();

  app.add_flag("--no-ra", options.no_ra, "disable register allocation");
  app.add_flag("--list-passes", options.list_passes, "list middle-end transform passes");
  app.add_flag("--print-pipeline", options.print_pipeline, "print active middle-end pipeline");
  app.add_flag("--time-passes", options.time_passes, "print middle-end pass timings");

  app.add_option("--disable-pass", options.disable_passes, "disable one or more middle-end passes")
      ->delimiter(',');
  app.add_option("--run-pass", options.run_passes, "run only the listed middle-end passes")
      ->delimiter(',');
  app.add_option("--start-after", options.start_after, "start the pipeline after the named pass");
  app.add_option("--stop-after", options.stop_after, "stop the pipeline after the named pass");
  app.add_option("--dump-ir-before", options.dump_ir_before, "dump LLVM IR before the named pass")
      ->delimiter(',');
  app.add_option("--dump-ir-after", options.dump_ir_after, "dump LLVM IR after the named pass")
      ->delimiter(',');
  app.add_option("--dump-ir-dir", options.dump_ir_dir, "directory for IR dumps");

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    return app.exit(e);
  }

  if (!emit_name.empty()) {
    auto parsed_emit = lava::driver::ParseEmitKind(LowerCase(emit_name));
    if (!parsed_emit.has_value()) {
      std::cerr << "error: unsupported emit mode '" << emit_name << "'\n";
      return 1;
    }
    if (emit_selected && options.emit != *parsed_emit) {
      std::cerr << "error: multiple emit modes specified\n";
      return 1;
    }
    options.emit = *parsed_emit;
  }

  options.opt_level = opt_level;

  if (options.input_file.empty() && !options.list_passes && !options.print_pipeline) {
    std::cerr << "error: no input file provided\n";
    return 1;
  }

  return lava::driver::RunDriver(options);
}
