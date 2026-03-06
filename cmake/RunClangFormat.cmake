if(NOT DEFINED PROJECT_SOURCE_DIR)
  message(FATAL_ERROR "PROJECT_SOURCE_DIR is not set")
endif()

if(NOT DEFINED CLANG_FORMAT_BIN OR CLANG_FORMAT_BIN STREQUAL ""
   OR CLANG_FORMAT_BIN MATCHES "-NOTFOUND$")
  message(FATAL_ERROR
          "clang-format was not found. Set LAVA_CLANG_FORMAT or install clang-format.")
endif()

if(NOT DEFINED GIT_EXECUTABLE OR GIT_EXECUTABLE STREQUAL ""
   OR GIT_EXECUTABLE MATCHES "-NOTFOUND$")
  message(FATAL_ERROR "git was not found. The format target requires git ls-files.")
endif()

execute_process(
    COMMAND "${GIT_EXECUTABLE}" -C "${PROJECT_SOURCE_DIR}" ls-files
            "*.cpp" "*.h" "*.hpp"
    RESULT_VARIABLE git_result
    OUTPUT_VARIABLE git_output
    ERROR_VARIABLE git_error
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT git_result EQUAL 0)
  message(FATAL_ERROR "git ls-files failed: ${git_error}")
endif()

if(git_output STREQUAL "")
  message(STATUS "No source files matched for formatting")
  return()
endif()

string(REPLACE "\n" ";" format_files "${git_output}")
list(FILTER format_files EXCLUDE REGEX "^src/lib/CLI11\\.hpp$")
list(TRANSFORM format_files PREPEND "${PROJECT_SOURCE_DIR}/")

if(format_files STREQUAL "")
  message(STATUS "No source files matched for formatting")
  return()
endif()

execute_process(
    COMMAND "${CLANG_FORMAT_BIN}" -style=file -i ${format_files}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    RESULT_VARIABLE format_result)

if(NOT format_result EQUAL 0)
  message(FATAL_ERROR "clang-format failed with exit code ${format_result}")
endif()
