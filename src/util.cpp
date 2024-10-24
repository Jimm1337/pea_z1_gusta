#include "util.hpp"

#include <fmt/core.h>

#include <INIReader.h>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <vector>

namespace util::config {

void help_page() noexcept {
  fmt::println(
  "Expecting ini config file of the form:\n\n"
  "[instance]\n"
  "input_path = <input path string>\n"
  "(optional) algorithm_parameter = <integer parameter>\n\n"
  "[optimal]\n"
  "(optional) path = <integer node><space><integer node><space>...\n"
  "(optional) cost = <integer cost>\n\n"
  "Example:\n"
  "[instance]\n"
  "input_path = C:/dev/pea_z1_gusta/data/test_6_as.txt\n"
  "algorithm_parameter = 1000\n\n"
  "[optimal]\n"
  "path = 2 3 0 1 4 5\n"
  "cost = 150\n");
}

[[nodiscard]] std::variant<tsp::Instance, tsp::ErrorConfig> read(
const std::filesystem::path& config_file) noexcept {
  const INIReader reader {config_file.generic_string()};

  if (reader.ParseError() < 0) [[unlikely]] {
    return tsp::ErrorConfig::BAD_READ;
  }

  const std::filesystem::path input_file {
    reader.Get("instance", "input_path", "")};
  if (input_file.empty()) [[unlikely]] {
    return tsp::ErrorConfig::BAD_CONFIG;
  }

  const std::filesystem::path input_file_parsed {
    input_file.is_absolute()
    ? input_file
    : [&input_file, &config_file]() noexcept {
        std::filesystem::path path {config_file.parent_path()};
        path /= input_file;
        return std::filesystem::absolute(path);
      }()};

  const auto matrix_result {input::tsp_matrix(input_file_parsed)};
  if (error::handle(matrix_result) == tsp::State::ERROR) [[unlikely]] {
    return tsp::ErrorConfig::CAN_NOT_PROCEED;
  }
  const tsp::Matrix<int> matrix {std::get<tsp::Matrix<int>>(matrix_result)};

  const int algorithm_parameter {
    static_cast<int>(reader.GetInteger("instance", "algorithm_parameter", -1))};

  const std::string optimal_solution_path {reader.Get("optimal", "path", "")};
  const std::vector optimal_solution_path_parsed {
    optimal_solution_path.empty()
    ? std::vector<int> {}
    : [&optimal_solution_path, &matrix]() noexcept {
        std::stringstream stream {optimal_solution_path};

        std::vector<int> path {};

        int extracted {-1};
        while (stream >> extracted) [[likely]] {
          if (stream.fail()) [[unlikely]] {
            return std::vector {-1};
          }

          if (extracted < 0 || extracted >= matrix.size()) [[unlikely]] {
            return std::vector {-1};
          }

          path.emplace_back(extracted);
        }

        if (path.size() != matrix.size()) [[unlikely]] {
          return std::vector {-1};
        }

        return path;
      }()};
  if (!optimal_solution_path_parsed.empty() &&
      optimal_solution_path_parsed.at(0) == -1) [[unlikely]] {
    return tsp::ErrorConfig::BAD_CONFIG;
  }

  const int optimal_solution_cost {static_cast<int>(
  reader.GetInteger("optimal", "cost", std::numeric_limits<int>::max()))};

  return tsp::Instance {
    .matrix      = matrix,
    .config_file = config_file,
    .input_file  = input_file_parsed,
    .optimal     = {.path = optimal_solution_path_parsed,
                    .cost = optimal_solution_cost},
    .param       = algorithm_parameter
  };
}

}    // namespace util::config

namespace util::output {

[[nodiscard]] constexpr static tsp::Duration parse_duration(
const tsp::Time& duration) noexcept {
  if (duration.count() < 1.) {    // microseconds
    return {.count = duration.count() * 1000, .unit = "us"};
  }

  if (duration.count() >= 1000.) {    // seconds
    return {.count = duration.count() / 1000, .unit = "s"};
  }

  if (duration.count() >= 60000.) {    // minutes
    return {.count = duration.count() / 1000 / 60, .unit = "min"};
  }

  if (duration.count() >= 360000.) {    // hours
    return {.count = duration.count() / 1000 / 60 / 60, .unit = "hours"};
  }

  return {.count = duration.count(), .unit = "ms"};
}

void report(const tsp::Arguments& arguments,
            const tsp::Instance&  instance,
            const tsp::Result&    result) noexcept {
  const auto& [matrix,
               config_filename,
               input_filename,
               optimal_solution,
               parameter] {instance};
  const auto& [solution, time] {result};
  const auto [count, unit] {parse_duration(time)};

  fmt::println("Config ({})", config_filename.generic_string());
  fmt::println("- Input file: {}", input_filename.generic_string());
  if (optimal_solution.cost == std::numeric_limits<int>::max()) {
    fmt::println("- Optimal cost: NOT PROVIDED");
  } else {
    fmt::println("- Optimal cost: {}", optimal_solution.cost);
  }
  if (optimal_solution.path.empty()) {
    fmt::println("- Optimal path: NOT PROVIDED\n");
  } else {
    fmt::print("- Optimal path: ");
    for (const auto& node : optimal_solution.path) {
      fmt::print("{} ", node);
    }
    fmt::println("\n");
  }

  switch (arguments.algorithm) {
    case tsp::Algorithm::BRUTE_FORCE:
      fmt::println("Algorithm (Brute Force)\n");
      break;
    case tsp::Algorithm::NEAREST_NEIGHBOUR:
      fmt::println("Algorithm (Nearest Neighbour)\n");
      break;
    case tsp::Algorithm::RANDOM:
      fmt::println("Algorithm (Random)");
      fmt::println("- Running time: {} ms\n", parameter);
      break;
    case tsp::Algorithm::BXB_LEAST_COST:
      fmt::println("Algorithm (BxB Least Cost)");
    default:
      fmt::println("[E] Something went wrong");
      break;
  }

  fmt::println("-- RESULTS --");
  fmt::println("Time: {:.2f} {}\n", count, unit);
  fmt::println("Cost: {}", solution.cost);
  if (matrix.size() < 16) {
    fmt::print("Path: ");
    for (const auto& node : solution.path) {
      fmt::print("{} ", node);
    }
    fmt::println("\n");
  }
}

}    // namespace util::output

namespace util::input {

void help_page() noexcept {
  fmt::println(
  "Expecting input file of the format:\n"
  "<number of vertices>\n"
  "<integer cost><space><integer cost>... (number of vertices times)\n"
  ".\n.\n.\n(number of vertices times)\n"
  "<new line>\n\n"
  "No connection cost is -1\n\n"
  "Example:\n"
  "6\n"
  "-1 99 19 83 23 12\n"
  "67 -1  3 71 85 74\n"
  "54 76 -1 55 62 78\n"
  "32 29 68 -1 76 14\n"
  "20 51 84 68 -1 93\n"
  "96 38 82 24  9 -1\n");
}

// return: cost matrix from matrix format tsp
[[nodiscard]] std::variant<tsp::Matrix<int>, tsp::ErrorRead> tsp_matrix(
const std::filesystem::path& input_file) noexcept {
  size_t vertex_count {0};

  std::ifstream file {input_file};

  if (file.fail()) [[unlikely]] {
    return tsp::ErrorRead::BAD_READ;
  }

  file >> vertex_count;

  if (file.eof()) [[unlikely]] {
    return tsp::ErrorRead::BAD_DATA;
  }

  if (vertex_count == 0) [[unlikely]] {
    return tsp::ErrorRead::BAD_DATA;
  }

  tsp::Matrix<int> cost_matrix {};
  cost_matrix.resize(vertex_count);

  for (int i = 0; i < vertex_count; ++i) {
    cost_matrix.at(i).resize(vertex_count, -2);

    for (int j = 0; j < vertex_count; ++j) {
      int cost_read {-2};
      file >> cost_read;

      if (file.eof()) [[unlikely]] {
        return tsp::ErrorRead::BAD_DATA;
      }

      cost_matrix.at(i).at(j) = cost_read;
    }
  }

  return cost_matrix;
}

}    // namespace util::input

namespace util::arg {

void help_page() noexcept {
  fmt::println(
  "Usage:\n"
  "pea_z1_gusta --config=<config file path> <one of the algorithm flags>\n\n"
  "Flags:\n"
  " -bf: Use Brute Force algorithm\n"
  " -nn: Use Nearest Neighbour algorithm\n"
  " -r : Use Random algorithm\n\n"
  "Example:\n"
  "pea_z1_gusta --config=C:/dev/pea_z1_gusta/configs/test_6.ini -r\n");
}

[[nodiscard]] std::variant<tsp::Arguments, tsp::ErrorArg> read(
int          argc,
const char** argv) noexcept {
  if (argc <= 1) [[unlikely]] {
    return tsp::ErrorArg::NO_ARG;
  }

  const std::vector arg_vec {[&argc, &argv]() noexcept {
    std::vector vec {std::vector<std::string> {static_cast<size_t>(argc)}};
    for (int i = 0; i < argc; ++i) {
      vec.at(i) = std::string {argv[i]};
    }
    return vec;
  }()};

  const bool algo_nn {std::ranges::find(arg_vec, "-nn") != arg_vec.end()};
  const bool algo_bf {std::ranges::find(arg_vec, "-bf") != arg_vec.end()};
  const bool algo_random {std::ranges::find(arg_vec, "-r") != arg_vec.end()};
  const bool algo_bxblc {std::ranges::find(arg_vec, "-lc") != arg_vec.end()};

  const std::string config_path {[&arg_vec]() noexcept {
    const auto itr {std::ranges::find_if(arg_vec, [](const std::string& str) {
      return str.substr(0, 9) == "--config=";
    })};

    if (itr == arg_vec.end()) [[unlikely]] {
      return std::string {""};
    }

    return itr->substr(9);
  }()};
  if (config_path.empty()) [[unlikely]] {
    return tsp::ErrorArg::BAD_ARG;
  }

  const int algo_count {
    [&algo_bf, &algo_random, &algo_nn, &algo_bxblc]() noexcept {
      int count {0};
      if (algo_bf) {
        ++count;
      }
      if (algo_nn) {
        ++count;
      }
      if (algo_random) {
        ++count;
      }
      if (algo_bxblc) {
        ++count;
      }
      return count;
    }()};
  if (algo_count > 1) [[unlikely]] {
    return tsp::ErrorArg::MULTIPLE_ARG;
  }
  if (algo_count == 0) [[unlikely]] {
    return tsp::ErrorArg::NO_ARG;
  }

  return tsp::Arguments {
    .algorithm   = algo_nn     ? tsp::Algorithm::NEAREST_NEIGHBOUR
                 : algo_bf     ? tsp::Algorithm::BRUTE_FORCE
                 : algo_random ? tsp::Algorithm::RANDOM
                 : algo_bxblc  ? tsp::Algorithm::BXB_LEAST_COST
                               : tsp::Algorithm::INVALID,
    .config_file = std::filesystem::absolute(config_path)};
}

}    // namespace util::arg
