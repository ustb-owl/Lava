#include "driver/pipeline.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <ranges>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace {

std::string LowerCase(std::string value) {
  std::ranges::transform(value, value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::string KebabCase(std::string_view value) {
  std::string result;
  result.reserve(value.size() + 8);

  for (std::size_t i = 0; i < value.size(); ++i) {
    const auto ch = value[i];
    const auto is_upper = std::isupper(static_cast<unsigned char>(ch)) != 0;
    const auto is_digit = std::isdigit(static_cast<unsigned char>(ch)) != 0;

    if (i > 0 && is_upper) {
      const auto prev = value[i - 1];
      const auto prev_is_lower = std::islower(static_cast<unsigned char>(prev)) != 0;
      const auto prev_is_digit = std::isdigit(static_cast<unsigned char>(prev)) != 0;
      const auto next_is_lower =
          (i + 1 < value.size()) &&
          (std::islower(static_cast<unsigned char>(value[i + 1])) != 0);
      if (prev_is_lower || prev_is_digit || next_is_lower) {
        result.push_back('-');
      }
    } else if (i > 0 && is_digit &&
               std::isalpha(static_cast<unsigned char>(value[i - 1])) != 0 &&
               result.back() != '-') {
      result.push_back('-');
    }

    result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }

  return result;
}

const lava::driver::PassCliMetadata *FindMetadataByInternalName(std::string_view internal_name) {
  const auto &table = lava::opt::GetPassCliMetadata();
  auto it = std::find_if(table.begin(), table.end(), [&](const lava::driver::PassCliMetadata &entry) {
    return entry.internal_name == internal_name;
  });
  return it == table.end() ? nullptr : &*it;
}

std::optional<std::string> ResolveTransformPassInternalName(std::string_view token) {
  auto lowered = LowerCase(std::string(token));

  for (const auto &entry : lava::opt::GetPassCliMetadata()) {
    if (lowered == entry.cli_name || lowered == LowerCase(entry.internal_name)) {
      return entry.internal_name;
    }
    if (lowered == KebabCase(entry.internal_name)) {
      return entry.internal_name;
    }
    for (const auto &alias : entry.aliases) {
      if (lowered == alias) return entry.internal_name;
    }
  }

  const auto &passes = lava::opt::PassManager::GetPasses();
  for (const auto &[internal_name, info] : passes) {
    if (info->is_analysis()) continue;
    if (lowered == LowerCase(internal_name) || lowered == KebabCase(internal_name)) {
      return internal_name;
    }
  }

  return std::nullopt;
}

std::string DisplayPassName(const lava::opt::PassInfoPtr &info) {
  if (const auto *metadata = FindMetadataByInternalName(info->name())) {
    return metadata->cli_name;
  }
  return KebabCase(info->name());
}

std::string PassDescription(const lava::opt::PassInfoPtr &info) {
  if (const auto *metadata = FindMetadataByInternalName(info->name())) {
    return metadata->description;
  }
  return "";
}

lava::opt::PassInfoPtr FindPassInfoByInternalName(const std::string &internal_name) {
  const auto &passes = lava::opt::PassManager::GetPasses();
  auto it = passes.find(internal_name);
  if (it == passes.end()) return nullptr;
  return it->second;
}

lava::opt::PassPtrList AllTransformPasses() {
  lava::opt::PassPtrList pipeline;
  for (const auto &[_, info] : lava::opt::PassManager::GetPasses()) {
    if (info->is_analysis()) continue;
    pipeline.push_back(info);
  }
  std::sort(pipeline.begin(), pipeline.end(), lava::opt::compare);
  return pipeline;
}

std::unordered_set<std::string> PipelineNames(const lava::opt::PassPtrList &pipeline) {
  std::unordered_set<std::string> names;
  for (const auto &info : pipeline) {
    names.insert(info->name());
  }
  return names;
}

bool ResolvePipelinePasses(const std::vector<std::string> &requested,
                          const lava::opt::PassPtrList &pipeline,
                          std::unordered_set<std::string> &resolved,
                          std::string &error) {
  const auto pipeline_names = PipelineNames(pipeline);
  for (const auto &token : requested) {
    auto internal_name = ResolveTransformPassInternalName(token);
    if (!internal_name.has_value()) {
      error = "unknown pass name '" + token + "'";
      return false;
    }
    if (!pipeline_names.contains(*internal_name)) {
      error = "pass '" + token + "' is not present in the active pipeline";
      return false;
    }
    resolved.insert(*internal_name);
  }
  return true;
}

bool DumpModuleIR(lava::driver::Compiler &compiler,
                  const lava::driver::DriverOptions &options,
                  const lava::opt::PassInfoPtr &info,
                  std::size_t index,
                  std::string_view phase,
                  std::ostream &diag,
                  std::string &error) {
  namespace fs = std::filesystem;

  auto dump_dir = options.dump_ir_dir.empty() ? fs::path("lava-ir-dumps")
                                              : fs::path(options.dump_ir_dir);
  std::error_code ec;
  fs::create_directories(dump_dir, ec);
  if (ec) {
    error = "failed to create dump directory '" + dump_dir.string() + "'";
    return false;
  }

  auto stem = fs::path(options.input_file).stem().string();
  if (stem.empty()) stem = "module";

  std::ostringstream filename;
  filename << std::setw(2) << std::setfill('0') << index << '-'
           << phase << '-' << DisplayPassName(info) << '.'
           << stem << ".ll";
  auto path = dump_dir / filename.str();

  std::ofstream os(path);
  if (!os.is_open()) {
    error = "failed to open IR dump file '" + path.string() + "'";
    return false;
  }

  compiler.DumpIR(os);
  diag << "dumped IR to " << path << '\n';
  return true;
}

}

namespace lava::driver {

std::vector<PassCliMetadata> GetMiddleEndPassMetadata() {
  std::vector<PassCliMetadata> metadata;
  auto pipeline = AllTransformPasses();
  metadata.reserve(pipeline.size());
  for (const auto &info : pipeline) {
    if (const auto *entry = FindMetadataByInternalName(info->name())) {
      metadata.push_back(*entry);
    } else {
      metadata.push_back({info->name(), KebabCase(info->name()), {}, ""});
    }
  }
  return metadata;
}

bool HasMiddleEndExecutionControls(const DriverOptions &options) {
  return options.time_passes ||
         !options.disable_passes.empty() ||
         !options.run_passes.empty() ||
         options.start_after.has_value() ||
         options.stop_after.has_value() ||
         !options.dump_ir_before.empty() ||
         !options.dump_ir_after.empty();
}

bool BuildMiddleEndPipeline(const DriverOptions &options,
                            opt::PassPtrList &pipeline,
                            std::string &error) {
  auto all_passes = AllTransformPasses();

  if (!options.run_passes.empty() &&
      (!options.disable_passes.empty() || options.start_after.has_value() || options.stop_after.has_value())) {
    error = "--run-pass cannot be combined with --disable-pass, --start-after, or --stop-after";
    return false;
  }

  if (!options.run_passes.empty()) {
    std::unordered_set<std::string> seen;
    for (const auto &token : options.run_passes) {
      auto internal_name = ResolveTransformPassInternalName(token);
      if (!internal_name.has_value()) {
        error = "unknown pass name '" + token + "'";
        return false;
      }
      if (!seen.insert(*internal_name).second) {
        continue;
      }
      auto info = FindPassInfoByInternalName(*internal_name);
      if (info == nullptr || info->is_analysis()) {
        error = "pass '" + token + "' is not a transform pass";
        return false;
      }
      pipeline.push_back(info);
    }
    return true;
  }

  for (const auto &info : all_passes) {
    if (options.opt_level >= static_cast<int>(info->min_opt_level())) {
      pipeline.push_back(info);
    }
  }

  if (!options.disable_passes.empty()) {
    std::unordered_set<std::string> disabled;
    if (!ResolvePipelinePasses(options.disable_passes, pipeline, disabled, error)) {
      return false;
    }
    std::erase_if(pipeline, [&](const opt::PassInfoPtr &info) {
      return disabled.contains(info->name());
    });
  }

  if (options.start_after.has_value()) {
    auto internal_name = ResolveTransformPassInternalName(*options.start_after);
    if (!internal_name.has_value()) {
      error = "unknown pass name '" + *options.start_after + "'";
      return false;
    }
    auto it = std::find_if(pipeline.begin(), pipeline.end(), [&](const opt::PassInfoPtr &info) {
      return info->name() == *internal_name;
    });
    if (it == pipeline.end()) {
      error = "pass '" + *options.start_after + "' is not present in the active pipeline";
      return false;
    }
    pipeline.erase(pipeline.begin(), std::next(it));
  }

  if (options.stop_after.has_value()) {
    auto internal_name = ResolveTransformPassInternalName(*options.stop_after);
    if (!internal_name.has_value()) {
      error = "unknown pass name '" + *options.stop_after + "'";
      return false;
    }
    auto it = std::find_if(pipeline.begin(), pipeline.end(), [&](const opt::PassInfoPtr &info) {
      return info->name() == *internal_name;
    });
    if (it == pipeline.end()) {
      error = "pass '" + *options.stop_after + "' is not present in the active pipeline";
      return false;
    }
    pipeline.erase(std::next(it), pipeline.end());
  }

  return true;
}

void PrintMiddleEndPasses(std::ostream &os) {
  auto pipeline = AllTransformPasses();
  os << "Available middle-end transform passes:\n";
  for (const auto &info : pipeline) {
    os << "  " << std::left << std::setw(28) << DisplayPassName(info)
       << "  internal=" << std::setw(36) << info->name()
       << "  O" << info->min_opt_level()
       << "  order=" << info->pass_order();
    auto description = PassDescription(info);
    if (!description.empty()) {
      os << "  " << description;
    }
    os << '\n';
  }
}

void PrintMiddleEndPipeline(std::ostream &os, const opt::PassPtrList &pipeline) {
  os << "Middle-end pipeline:\n";
  for (std::size_t i = 0; i < pipeline.size(); ++i) {
    os << "  " << (i + 1) << ". " << DisplayPassName(pipeline[i])
       << " (" << pipeline[i]->name() << ")\n";
  }
}

bool RunMiddleEndPipeline(Compiler &compiler,
                          const DriverOptions &options,
                          std::ostream &diag,
                          std::string &error) {
  opt::PassPtrList pipeline;
  if (!BuildMiddleEndPipeline(options, pipeline, error)) {
    return false;
  }

  std::unordered_set<std::string> dump_before;
  std::unordered_set<std::string> dump_after;
  if (!ResolvePipelinePasses(options.dump_ir_before, pipeline, dump_before, error)) {
    return false;
  }
  if (!ResolvePipelinePasses(options.dump_ir_after, pipeline, dump_after, error)) {
    return false;
  }

  opt::PassManager::set_opt_level(options.opt_level);
  opt::PassManager::SetModule(compiler.module());

  opt::PassNameSet valid;
  std::vector<std::pair<std::string, std::chrono::steady_clock::duration>> timings;
  auto total_start = std::chrono::steady_clock::now();

  for (std::size_t i = 0; i < pipeline.size(); ++i) {
    const auto &info = pipeline[i];

    if (dump_before.contains(info->name()) &&
        !DumpModuleIR(compiler, options, info, i + 1, "before", diag, error)) {
      return false;
    }

    auto start = std::chrono::steady_clock::now();
    opt::PassManager::RunPass(valid, info);
    auto end = std::chrono::steady_clock::now();

    if (options.time_passes) {
      timings.emplace_back(DisplayPassName(info), end - start);
    }

    if (dump_after.contains(info->name()) &&
        !DumpModuleIR(compiler, options, info, i + 1, "after", diag, error)) {
      return false;
    }
  }

  if (options.time_passes) {
    auto total = std::chrono::steady_clock::now() - total_start;
    diag << "middle-end pass timings:\n";
    for (const auto &[name, duration] : timings) {
      auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
      diag << "  " << std::left << std::setw(28) << name << ms << " ms\n";
    }
    diag << "  " << std::left << std::setw(28) << "total"
         << std::chrono::duration_cast<std::chrono::milliseconds>(total).count()
         << " ms\n";
  }

  return true;
}

}
