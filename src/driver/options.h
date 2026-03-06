#ifndef LAVA_DRIVER_OPTIONS_H
#define LAVA_DRIVER_OPTIONS_H

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace lava::driver {

enum class EmitKind {
  Ast,
  Ir,
  Asm,
  Cfg,
};

struct DriverOptions {
  std::string                input_file;
  std::string                output_file;
  EmitKind                   emit           = EmitKind::Asm;
  int                        opt_level      = 0;
  bool                       no_ra          = false;
  bool                       list_passes    = false;
  bool                       print_pipeline = false;
  bool                       time_passes    = false;
  std::vector<std::string>   disable_passes;
  std::vector<std::string>   run_passes;
  std::optional<std::string> start_after;
  std::optional<std::string> stop_after;
  std::vector<std::string>   dump_ir_before;
  std::vector<std::string>   dump_ir_after;
  std::string                dump_ir_dir;
};

inline constexpr std::string_view EmitKindName(EmitKind emit) {
  switch (emit) {
  case EmitKind::Ast:
    return "ast";
  case EmitKind::Ir:
    return "ir";
  case EmitKind::Asm:
    return "asm";
  case EmitKind::Cfg:
    return "cfg";
  }
  return "asm";
}

inline constexpr bool RequiresCodeGen(EmitKind emit) {
  return emit == EmitKind::Asm;
}

inline std::optional<EmitKind> ParseEmitKind(std::string_view emit_name) {
  if (emit_name == "ast")
    return EmitKind::Ast;
  if (emit_name == "ir")
    return EmitKind::Ir;
  if (emit_name == "asm")
    return EmitKind::Asm;
  if (emit_name == "cfg")
    return EmitKind::Cfg;
  return std::nullopt;
}

} // namespace lava::driver

#endif // LAVA_DRIVER_OPTIONS_H
