#pragma once

#include <fmt/core.h>
#include <type_traits>

#include <array>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <ratio>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tsp {

template<typename T>
using Matrix = std::vector<std::vector<T>>;

using Time = std::chrono::duration<double, std::milli>;

enum class State : uint_fast8_t {
  OK,
  ERROR,
};

enum class Algorithm : uint_fast8_t {
#if defined(ZADANIE1) && ZADANIE1 == 1
  BRUTE_FORCE,
  NEAREST_NEIGHBOUR,
  RANDOM,
#endif

#if defined(ZADANIE2) && ZADANIE2 == 1
  BXB_LEAST_COST,
  BXB_BFS,
  BXB_DFS,
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
  TABU_SEARCH,
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
  GENETIC,
#endif

  INVALID,
};

enum class ErrorConfig : uint_fast8_t {
  BAD_READ,
  BAD_CONFIG,
  CAN_NOT_PROCEED,
};

enum class ErrorAlgorithm : uint_fast8_t {
  NO_PATH,
  INVALID_PARAM,
};

enum class ErrorArg : uint_fast8_t {
  NO_ARG,
  MULTIPLE_ARG,
  BAD_ARG,
};

enum class ErrorRead : uint_fast8_t {
  BAD_READ,
  BAD_DATA,
};

enum class ErrorMeasure : uint_fast8_t {
  ALGORITHM_ERROR,
  FILE_ERROR,
};

struct MeasuringRun {
  std::array<Algorithm, 8> algorithms;
  bool                     verbose;
};

struct SingleRun {
  Algorithm             algorithm;
  std::filesystem::path config_file;
};

using Arguments = std::variant<MeasuringRun, SingleRun>;

struct Solution {
  std::vector<int> path;
  int              cost;
};

#if defined(ZADANIE1) && ZADANIE1 == 1
struct ParamRandom {
  int millis;
};
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
struct ParamTabuSearch {
  int itr;
  int max_itr_no_improve;
  int tabu_itr;
};
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
struct ParamGenetic {
  int itr;
  int population_size;
  int children_per_itr;
  int max_children_per_pair;
  int max_v_count_crossover;
  int mutations_per_1000;
};
#endif

struct Param {
#if defined(ZADANIE1) && ZADANIE1 == 1
  ParamRandom random;
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
  ParamTabuSearch tabu_search;
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
  ParamGenetic genetic;
#endif
};

struct GraphInfo {
  bool symmetric_graph;
  bool full_graph;
};

struct Instance {
  Matrix<int>           matrix;
  std::filesystem::path config_file;
  std::filesystem::path input_file;
  Solution              optimal;
  Param                 params;
  GraphInfo             graph_info;
};

struct Error {
  int    absolute;
  double relative_percent;
};

struct Result {
  Solution             solution;
  Time                 time;
  std::optional<Error> error_info;
};

struct Duration {
  double      count;
  std::string unit;
};

}    // namespace tsp

namespace util::config {

[[nodiscard]] std::variant<tsp::Instance, tsp::ErrorConfig> read(
const std::filesystem::path& config_file) noexcept;

void help_page() noexcept;

}    // namespace util::config

namespace util::output {

void report(const tsp::SingleRun& arguments,
            const tsp::Instance&  instance,
            const tsp::Result&    result) noexcept;

}    // namespace util::output

namespace util::input {

[[nodiscard]] std::variant<tsp::Matrix<int>, tsp::ErrorRead> tsp_matrix(
const std::filesystem::path& input_file) noexcept;

void help_page() noexcept;

}    // namespace util::input

namespace util::arg {

[[nodiscard]] std::variant<tsp::Arguments, tsp::ErrorArg> read(
int          argc,
const char** argv) noexcept;

void help_page() noexcept;

}    // namespace util::arg

namespace util::error {

template<typename Ret, typename Error>
requires std::same_as<std::remove_cvref_t<Error>, tsp::ErrorAlgorithm> ||
         std::same_as<std::remove_cvref_t<Error>, tsp::ErrorArg> ||
         std::same_as<std::remove_cvref_t<Error>, tsp::ErrorConfig> ||
         std::same_as<std::remove_cvref_t<Error>, tsp::ErrorRead> ||
         std::same_as<std::remove_cvref_t<Error>, tsp::ErrorMeasure>
tsp::State handle(const std::variant<Ret, Error>& result) noexcept {
  using Err = std::remove_cvref_t<Error>;

  if constexpr (std::is_same_v<Err, tsp::ErrorAlgorithm>) {
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
      fmt::print("[E] Algorithm Error: ");

      switch (std::get<1>(result)) {
        case tsp::ErrorAlgorithm::NO_PATH:
          fmt::println("No complete path found!");
          return tsp::State::ERROR;

        case tsp::ErrorAlgorithm::INVALID_PARAM:
          fmt::println("Invalid parameter specified in config!");
          config::help_page();
          return tsp::State::ERROR;

        default:
          fmt::println("Something went wrong!");
          return tsp::State::ERROR;
      }
    }
  }

  if constexpr (std::is_same_v<Err, tsp::ErrorArg>) {
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
      fmt::print("[E] Argument error: ");

      switch (std::get<1>(result)) {
        case tsp::ErrorArg::NO_ARG:
          fmt::println("No arguments.\n");
          arg::help_page();
          return tsp::State::ERROR;

        case tsp::ErrorArg::MULTIPLE_ARG:
          fmt::println("Multiple arguments.\n");
          arg::help_page();
          return tsp::State::ERROR;

        case tsp::ErrorArg::BAD_ARG:
          fmt::println("Invalid arguments.\n");
          arg::help_page();
          return tsp::State::ERROR;

        default:
          fmt::println("Something went wrong!");
          return tsp::State::ERROR;
      }
    }
  }

  if constexpr (std::is_same_v<Err, tsp::ErrorConfig>) {
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
      fmt::print("[E] Config Error: ");

      switch (std::get<1>(result)) {
        case tsp::ErrorConfig::BAD_READ:
          fmt::println("Could not read config file at specified path!");
          return tsp::State::ERROR;

        case tsp::ErrorConfig::BAD_CONFIG:
          fmt::println("Invalid format!\n");
          config::help_page();
          return tsp::State::ERROR;

        case tsp::ErrorConfig::CAN_NOT_PROCEED:
          fmt::println("Can not proceed due to previous errors.");
          return tsp::State::ERROR;

        default:
          fmt::println("Something went wrong!");
          return tsp::State::ERROR;
      }
    }
  }

  if constexpr (std::is_same_v<Err, tsp::ErrorRead>) {
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
      fmt::print("[E] Input Error: ");

      switch (std::get<1>(result)) {
        case tsp::ErrorRead::BAD_READ:
          fmt::println("Could not read input file specified in config!");
          return tsp::State::ERROR;

        case tsp::ErrorRead::BAD_DATA:
          fmt::println("Wrong format of data!\n");
          input::help_page();
          return tsp::State::ERROR;

        default:
          fmt::println("Something went wrong!");
          return tsp::State::ERROR;
      }
    }
  }

  if constexpr (std::is_same_v<Err, tsp::ErrorMeasure>) {
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
      fmt::print("[E] Error during measurements: ");

      switch (std::get<1>(result)) {
        case tsp::ErrorMeasure::ALGORITHM_ERROR:
          fmt::println("Algorithm error occurred during measurements!");
          return tsp::State::ERROR;
        case tsp::ErrorMeasure::FILE_ERROR:
          fmt::println("File error occurred during measurements!");
          return tsp::State::ERROR;
        default:
          fmt::println("Something went wrong!");
          return tsp::State::ERROR;
      }
    }
  }

  return tsp::State::OK;
}

}    // namespace util::error

namespace util {

template<typename Func, typename... Params>
requires std::invocable<Func,
                        const tsp::Matrix<int>&,
                        const tsp::GraphInfo&,
                        const std::optional<int>&,
                        Params...> &&
         std::is_same_v<std::invoke_result_t<Func,
                                             const tsp::Matrix<int>&,
                                             const tsp::GraphInfo&,
                                             const std::optional<int>&,
                                             Params...>,
                        std::variant<tsp::Solution, tsp::ErrorAlgorithm>>
[[nodiscard]] std::variant<tsp::Result, tsp::ErrorAlgorithm> measured_run(
Func                      algorithm,
const tsp::Matrix<int>&   matrix,
const tsp::GraphInfo&     graph_info,
const std::optional<int>& optimal_cost,
Params&&... params) noexcept {
  const auto start {std::chrono::high_resolution_clock::now()};

  const std::variant<tsp::Solution, tsp::ErrorAlgorithm> solution_result {
    algorithm(matrix,
              graph_info,
              optimal_cost,
              std::forward<Params>(params)...)};

  const auto end {std::chrono::high_resolution_clock::now()};

  if (std::holds_alternative<tsp::ErrorAlgorithm>(solution_result))
  [[unlikely]] {
    return std::get<tsp::ErrorAlgorithm>(solution_result);
  }
  const tsp::Solution solution {std::get<tsp::Solution>(solution_result)};

  return tsp::Result {
    solution,
    tsp::Time {end - start},
    optimal_cost.has_value()
    ? std::optional {tsp::Error {
        .absolute = solution.cost - *optimal_cost,
        .relative_percent =
        ((static_cast<double>(solution.cost) / *optimal_cost) - 1.) * 100.}}
    : std::nullopt};
}

}    // namespace util
