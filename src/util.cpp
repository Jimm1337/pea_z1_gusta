#include "util.hpp"

#include <fmt/core.h>
#include <string_view>

#include <INIReader.h>
#include <fstream>
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
  "cost = 150");
}

std::variant<tsp::Instance, tsp::ErrorConfig> read(
std::string_view filename) noexcept {
  // const auto filename = "C:/dev/pea_z1_gusta/configs/instance_6.ini";

  const INIReader reader {std::string {filename}};

  if (reader.ParseError() < 0) [[unlikely]] {
    return tsp::ErrorConfig::BAD_READ;
  }

  const std::string input_path {reader.Get("instance", "input_path", "")};
  if (input_path == "") [[unlikely]] {
    return tsp::ErrorConfig::BAD_CONFIG;
  }

  const long algorithm_parameter {
    reader.GetInteger("instance", "algorithm_parameter", -1)};

  const std::string optimal_solution_path {reader.Get("optimal", "path", "")};
  const std::vector<int> optimal_solution_path_parsed {
    optimal_solution_path == ""
    ? std::vector<int> {}
    : [&optimal_solution_path]() noexcept {
        std::stringstream stream {optimal_solution_path};

        std::vector<int> path {};

        int extracted {-1};
        while (stream >> extracted) {
          if (stream.fail()) {
            return std::vector<int> {-1};
          }

          path.emplace_back(extracted);
        }

        return path;
      }()};
  if (!optimal_solution_path_parsed.empty() &&
      optimal_solution_path_parsed.at(0) == -1) {
    return tsp::ErrorConfig::BAD_CONFIG;
  }

  const long optimal_solution_cost {
    reader.GetInteger("optimal", "cost", std::numeric_limits<long>::max())};

  const auto matrix_result {input::tsp_matrix(input_path)};
  if (error::handle(matrix_result) == tsp::State::ERROR) {
    return tsp::ErrorConfig::CAN_NOT_PROCEED;
  }
  const tsp::Matrix<int> matrix {std::get<tsp::Matrix<int>>(matrix_result)};

  return tsp::Instance {
    matrix,
    std::string {filename},
    input_path,
    {optimal_solution_path_parsed, optimal_solution_cost},
    algorithm_parameter
  };
}

}    // namespace util::config

namespace util::output {

tsp::Duration parse_duration(const tsp::Time& duration) noexcept {
  if (duration.count() < 0.001) {    // nanoseconds
    return {duration.count() * 1000 * 1000, "ns"};
  }

  if (duration.count() < 1.) {    // microseconds
    return {duration.count() * 1000, "us"};
  }

  if (duration.count() >= 1000.) {    // seconds
    return {duration.count() / 1000, "s"};
  }

  if (duration.count() >= 60000.) {    // minutes
    return {duration.count() / 1000 / 60, "min"};
  }

  if (duration.count() >= 360000.) {    // hours
    return {duration.count() / 1000 / 60 / 60, "hours"};
  }

  return {duration.count(), "ms"};
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
  const tsp::Duration duration {parse_duration(time)};

  fmt::println("Config ({})", config_filename);
  fmt::println("- Input file: {}", input_filename);
  if (optimal_solution.cost == std::numeric_limits<long>::max()) {
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
      fmt::println("- Count of iterations: {}\n", parameter);
      break;
  }

  fmt::println("-- RESULTS --");
  fmt::println("Time: {} {}\n", duration.count, duration.unit);
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

// return: cost matrix from mierzwa format tsp
std::variant<tsp::Matrix<int>, tsp::ErrorRead> tsp_matrix(
std::string_view filename) noexcept {
  size_t vertex_count {0};

  std::ifstream file {filename.data()};

  if (file.fail()) {
    return tsp::ErrorRead::BAD_READ;
  }

  file >> vertex_count;

  if (file.eof()) {
    return tsp::ErrorRead::BAD_DATA;
  }

  if (vertex_count == 0) {
    return tsp::ErrorRead::BAD_DATA;
  }

  tsp::Matrix<int> cost_matrix {};
  cost_matrix.resize(vertex_count);

  for (int i = 0; i < vertex_count; ++i) {
    cost_matrix.at(i).resize(vertex_count, -2);

    for (int j = 0; j < vertex_count; ++j) {
      int cost_read {-2};
      file >> cost_read;

      if (file.eof()) {
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
  "pea_z1_gusta --config=C:/dev/pea_z1_gusta/configs/test_6.ini -r");
}

std::variant<tsp::Arguments, tsp::ErrorArg> read(int          argc,
                                                 const char** argv) noexcept {
  if (argc <= 1) {
    return tsp::ErrorArg::NO_ARG;
  }

  const std::vector<std::string> arg_vec {[&argc, &argv]() noexcept {
    std::vector<std::string> vec {
      std::vector<std::string> {static_cast<size_t>(argc)}};
    for (int i = 0; i < argc; ++i) {
      vec.at(i) = std::string {argv[i]};
    }
    return vec;
  }()};

  const bool algo_nn {std::ranges::find(arg_vec, "-nn") != arg_vec.end()};
  const bool algo_bf {std::ranges::find(arg_vec, "-bf") != arg_vec.end()};
  const bool algo_random {std::ranges::find(arg_vec, "-r") != arg_vec.end()};

  const std::string config_path {[&arg_vec]() noexcept {
    const auto itr {std::ranges::find_if(arg_vec, [](const std::string& str) {
      if (str.substr(0, 9) == "--config=") {
        return true;
      }

      return false;
    })};

    if (itr == arg_vec.end()) {
      return std::string {""};
    }

    return itr->substr(9);
  }()};
  if (config_path == "") {
    return tsp::ErrorArg::BAD_ARG;
  }

  const int algo_count {[&algo_bf, &algo_random, &algo_nn]() noexcept {
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
    return count;
  }()};
  if (algo_count > 1) {
    return tsp::ErrorArg::MULTIPLE_ARG;
  } else if (algo_count == 0) {
    return tsp::ErrorArg::NO_ARG;
  }

  return tsp::Arguments {algo_nn         ? tsp::Algorithm::NEAREST_NEIGHBOUR
                         : algo_bf       ? tsp::Algorithm::BRUTE_FORCE
                           : algo_random ? tsp::Algorithm::RANDOM
                                         : tsp::Algorithm::INVALID,
                         config_path};
}

}    // namespace util::arg
