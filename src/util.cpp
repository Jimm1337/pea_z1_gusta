#include "util.hpp"

#include "bf.hpp"
#include "bxb.hpp"
#include "gen.hpp"
#include "nn.hpp"
#include "random.hpp"
#include "ts.hpp"
#include <fmt/core.h>

#include <INIReader.h>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ranges>
#include <sstream>
#include <variant>
#include <vector>

namespace util::config {

void help_page() noexcept {
  fmt::println(
  "\n\nHelp:\n\nExpecting ini config file of the form:\n\n"
  "[instance]\n"
  "input_path = <input path string>\n"
  "(optional) symmetric = <symmetric graph? bool>\n"
  "(optional) full = <full graph? bool>\n\n"
  "[optimal]\n"
  "(optional) path = <integer node><space><integer node><space>...\n"
  "(optional) cost = <integer cost>\n\n"
  "[random]\n"
  "millis = <integer running time in ms>\n\n"
  "[tabu_search]\n"
  "itr = <integer max iterations>\n"
  "max_itr_no_improve = <integer iterations to halt with no improvement>\n"
  "tabu_itr = <integer iterations in tabu>\n\n"
  "[genetic]\n"
  "itr = <integer iterations>\n"
  "population_size = <integer population after selection>\n"
  "children_per_itr = <integer count of all children in itr>\n"
  "max_children_per_pair = <integer max children per pair>\n"
  "max_v_count_crossover = <integer max vertices to crossover>\n"
  "mutations_per_1000 = <integer ppt of chromosomes to mutate>\n\n"
  "- Example:\n\n"
  "[instance]\n"
  "input_path = C:/dev/pea_z1_gusta/data/test_6_as.txt\n"
  "symmetric = false\n"
  "full = true\n\n"
  "[optimal]\n"
  "path = 2 3 0 1 4 5\n"
  "cost = 150\n\n"
  "[random]\n"
  "millis = 1000\n\n"
  "[tabu_search]\n"
  "itr = 10000\n"
  "max_itr_no_improve = 50\n"
  "tabu_itr = 10\n\n"
  "[genetic]\n"
  "itr = 100000\n"
  "population_size = 50\n"
  "count_of_children = 50\n"
  "max_children_per_pair = 4\n"
  "max_v_count_crossover = 5\n"
  "mutations_per_1000 = 5\n\n");
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

  const bool symmetric_graph {
    reader.GetBoolean("instance", "symmetric", false)};
  const bool full_graph {reader.GetBoolean("instance", "full", false)};

  const tsp::Param algo_params {
    .random      = {.millis =
                    static_cast<int>(reader.GetInteger("random", "millis", -1))},
    .tabu_search = {.itr = static_cast<int>(
                    reader.GetInteger("tabu_search", "itr", -1)),
                    .max_itr_no_improve = static_cast<int>(
                    reader.GetInteger("tabu_search", "max_itr_no_improve", -1)),
                    .tabu_itr = static_cast<int>(
                    reader.GetInteger("tabu_search", "tabu_itr", -1))},
    .genetic     = {
                    .itr = static_cast<int>(reader.GetInteger("genetic", "itr", -1)),
                    .population_size =
      static_cast<int>(reader.GetInteger("genetic", "population_size", -1)),
                    .children_per_itr =
      static_cast<int>(reader.GetInteger("genetic", "children_per_itr", -1)),
                    .max_children_per_pair = static_cast<int>(
      reader.GetInteger("genetic", "max_children_per_pair", -1)),
                    .max_v_count_crossover = static_cast<int>(
      reader.GetInteger("genetic", "max_v_count_crossover", -1)),
                    .mutations_per_1000 = static_cast<int>(
      reader.GetInteger("genetic", "mutations_per_1000", -1))}
  };

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

  const int optimal_solution_cost {
    static_cast<int>(reader.GetInteger("optimal", "cost", -1))};

  return tsp::Instance {
    .matrix      = matrix,
    .config_file = config_file,
    .input_file  = input_file_parsed,
    .optimal     = {.path = optimal_solution_path_parsed,
                    .cost = optimal_solution_cost},
    .params      = algo_params,
    .graph_info  = {  .symmetric_graph = symmetric_graph,
                    .full_graph      = full_graph}
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

  return {.count = duration.count(), .unit = "ms"};    // milliseconds
}

void report(const tsp::SingleRun& arguments,
            const tsp::Instance&  instance,
            const tsp::Result&    result) noexcept {
  const auto& [matrix,
               config_filename,
               input_filename,
               optimal_solution,
               params,
               graph_info] {instance};
  const auto& [solution, time, error_info] {result};
  const auto [count, unit] {parse_duration(time)};

  const bool optimized {graph_info.full_graph && graph_info.symmetric_graph};

  fmt::println("Config ({})", config_filename.generic_string());
  fmt::println("- Input file: {}", input_filename.generic_string());
  if (optimal_solution.cost == -1) {
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
      if (optimized) {
        fmt::println(
        "Optimization: Single Starting Vertex (full, symmetric)\n");
      }
      break;
    case tsp::Algorithm::NEAREST_NEIGHBOUR:
      fmt::println("Algorithm (Nearest Neighbour)\n");
      if (optimized) {
        fmt::println(
        "Optimization: Single Starting Vertex (full, symmetric)\n");
      }
      break;
    case tsp::Algorithm::RANDOM:
      fmt::println("Algorithm (Random)");
      fmt::println("- Running time: {} ms\n", params.random.millis);
      break;
    case tsp::Algorithm::BXB_LEAST_COST:
      fmt::println("Algorithm (BxB Least Cost)\n");
      if (optimized) {
        fmt::println(
        "Optimization: Single Starting Vertex (full, symmetric)\n");
      }
      break;
    case tsp::Algorithm::BXB_BFS:
      fmt::println("Algorithm (BxB BFS)\n");
      if (optimized) {
        fmt::println(
        "Optimization: Single Starting Vertex (full, symmetric)\n");
      }
      break;
    case tsp::Algorithm::BXB_DFS:
      fmt::println("Algorithm (BxB DFS)\n");
      if (optimized) {
        fmt::println(
        "Optimization: Single Starting Vertex (full, symmetric)\n");
      }
      break;
    case tsp::Algorithm::TABU_SEARCH:
      fmt::println("Algorithm (Tabu Search)");
      fmt::println("- Count of iterations: {}", params.tabu_search.itr);
      fmt::println("- Max iterations with no improvement: {}",
                   params.tabu_search.max_itr_no_improve);
      fmt::println("- Count of iterations in tabu: {}\n",
                   params.tabu_search.tabu_itr);
      break;
    case tsp::Algorithm::GENETIC:
      fmt::println("Algorithm (Genetic)");
      fmt::println("- Count of iterations: {}", params.genetic.itr);
      fmt::println("- Population size: {}", params.genetic.population_size);
      fmt::println("- Children per iteration: {}",
                   params.genetic.children_per_itr);
      fmt::println("- Maximum children per pair: {}",
                   params.genetic.max_children_per_pair);
      fmt::println("- Maximum number of vertices to crossover: {}",
                   params.genetic.max_v_count_crossover);
      fmt::println("- Mutation chance: {:.1f}%\n",
                   static_cast<double>(params.genetic.mutations_per_1000) / 10);
      break;
    default:
      fmt::println("[E] Something went wrong\n");
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
    fmt::println("");
  }
  if (error_info.has_value()) {
    fmt::println("");
    fmt::println("Absolute error: {}", error_info->absolute);
    fmt::println("Relative error: {:.2f}%", error_info->relative_percent);
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
  "(Single Run) pea_z1_gusta --config=<config file path> <one of the algorithm flags>\n"
  "(Measuring Run) pea_z1_gusta --measure (optional)--verbose <one or more of the algorithm flags>\n\n"
  "Flags:\n"
  " -bf: Use Brute Force algorithm\n"
  " -nn: Use Nearest Neighbour algorithm\n"
  " -r : Use Random algorithm\n"
  " -lc: Use Branch and Bound Least Cost algorithm\n"
  " -bb: Use Branch and Bound BFS algorithm\n"
  " -bd: Use Branch and Bound DFS algorithm\n"
  " -ts: Use Tabu Search algorithm\n"
  " -g : Use Genetic algorithm\n\n"
  "Example:\n"
  "pea_z1_gusta --config=C:/dev/pea_z1_gusta/configs/test_6.ini -r\n"
  "pea_z1_gusta --measure --verbose -nn -r -lc -bb -bd -ts -g\n");
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
  const bool algo_bxbbfs {std::ranges::find(arg_vec, "-bb") != arg_vec.end()};
  const bool algo_bxbdfs {std::ranges::find(arg_vec, "-bd") != arg_vec.end()};
  const bool algo_ts {std::ranges::find(arg_vec, "-ts") != arg_vec.end()};
  const bool algo_gen {std::ranges::find(arg_vec, "-g") != arg_vec.end()};

  // do measuring run
  if (std::ranges::find(arg_vec, "--measure") != arg_vec.end()) {
    tsp::MeasuringRun run {};
    run.algorithms.fill(tsp::Algorithm::INVALID);

    if (std::ranges::find(arg_vec, "--verbose") != arg_vec.end()) {
      run.verbose = true;
    }

    if (algo_nn) {
      run.algorithms.at(0) = tsp::Algorithm::NEAREST_NEIGHBOUR;
    }

    if (algo_bf) {
      run.algorithms.at(1) = tsp::Algorithm::BRUTE_FORCE;
    }

    if (algo_random) {
      run.algorithms.at(2) = tsp::Algorithm::RANDOM;
    }

    if (algo_bxblc) {
      run.algorithms.at(3) = tsp::Algorithm::BXB_LEAST_COST;
    }

    if (algo_bxbbfs) {
      run.algorithms.at(4) = tsp::Algorithm::BXB_BFS;
    }

    if (algo_bxbdfs) {
      run.algorithms.at(5) = tsp::Algorithm::BXB_DFS;
    }

    if (algo_ts) {
      run.algorithms.at(6) = tsp::Algorithm::TABU_SEARCH;
    }

    if (algo_gen) {
      run.algorithms.at(7) = tsp::Algorithm::GENETIC;
    }

    if (std::ranges::all_of(run.algorithms, [](const tsp::Algorithm& algo) {
          return algo == tsp::Algorithm::INVALID;
        })) [[unlikely]] {
      return tsp::ErrorArg::BAD_ARG;
    }

    return run;
  }

  // do single run
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

  const int algo_count {[&algo_bf,
                         &algo_random,
                         &algo_nn,
                         &algo_bxblc,
                         &algo_bxbbfs,
                         &algo_bxbdfs,
                         &algo_ts,
                         &algo_gen]() noexcept {
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
    if (algo_bxbbfs) {
      ++count;
    }
    if (algo_bxbdfs) {
      ++count;
    }
    if (algo_ts) {
      ++count;
    }
    if (algo_gen) {
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

  return tsp::SingleRun {
    .algorithm   = algo_nn     ? tsp::Algorithm::NEAREST_NEIGHBOUR
                 : algo_bf     ? tsp::Algorithm::BRUTE_FORCE
                 : algo_random ? tsp::Algorithm::RANDOM
                 : algo_bxblc  ? tsp::Algorithm::BXB_LEAST_COST
                 : algo_bxbbfs ? tsp::Algorithm::BXB_BFS
                 : algo_bxbdfs ? tsp::Algorithm::BXB_DFS
                 : algo_ts     ? tsp::Algorithm::TABU_SEARCH
                 : algo_gen    ? tsp::Algorithm::GENETIC
                               : tsp::Algorithm::INVALID,
    .config_file = std::filesystem::absolute(config_path)};
}

}    // namespace util::arg

namespace util::measure {

template<typename AlgoRun, typename... Params>
requires std::invocable<AlgoRun,
                        const tsp::Matrix<int>&,
                        const tsp::GraphInfo&,
                        const std::optional<int>&,
                        Params...>
static std::optional<tsp::ErrorMeasure> z1z2_measure_trivial_symmetric(
AlgoRun     algorithm_run_func,
int         max_v,
bool        verbose,
const char* out,
Params... algo_params) noexcept {
  const tsp::GraphInfo graph_info_s {true, true};

  std::ofstream file_s {out};

  if (!file_s.is_open()) {
    fmt::println("[E] Could not open file for writing!");
    return tsp::ErrorMeasure::FILE_ERROR;
  }

  file_s << "Nazwa;Koszt optymalny;Koszt obliczony;Czas [us]\n";

  for (int s {5}; s < std::min(20, max_v); ++s) {
    if (verbose) {
      fmt::print("Random  Symmetric {:>2}: ", s);
    }

    const auto instance_result {
      config::read(fmt::format("./data/rand_tsp/configs/{}_rand_s.ini", s))};
    if (error::handle(instance_result) == tsp::State::ERROR) {
      fmt::println(
      "[E] Make sure the config files are in (./data/rand_tsp/configs/<instance>_rand_s.ini)!");
      return tsp::ErrorMeasure::FILE_ERROR;
    }
    const tsp::Instance instance {std::get<tsp::Instance>(instance_result)};

    std::array<tsp::Result, 3>  cache_runs {};
    std::array<tsp::Result, 10> runs {};

    for (int i {0}; i < 3; ++i) {
      auto result {measured_run(algorithm_run_func,
                                instance.matrix,
                                graph_info_s,
                                std::optional {instance.optimal.cost},
                                algo_params...)};
      if (error::handle(result) == tsp::State::ERROR) {
        return tsp::ErrorMeasure::ALGORITHM_ERROR;
      }
      cache_runs.at(i) = std::move(std::get<tsp::Result>(result));
    }

    if (verbose) {
      fmt::print("[");
    }

    for (int i {0}; i < 10; ++i) {
      auto result_ {measured_run(algorithm_run_func,
                                 instance.matrix,
                                 graph_info_s,
                                 std::optional {instance.optimal.cost},
                                 algo_params...)};
      if (error::handle(result_) == tsp::State::ERROR) [[unlikely]] {
        return tsp::ErrorMeasure::ALGORITHM_ERROR;
      }

      runs.at(i) = std::move(std::get<tsp::Result>(result_));

      if (verbose) {
        fmt::print("-");
      }
    }

    if (verbose) {
      fmt::println("]");
    }

    const std::string instance_name {instance.input_file.filename().string()};
    const int         optimal_cost {instance.optimal.cost};
    for (const tsp::Result& run : runs) {
      const int         cost {run.solution.cost};
      const std::string time_us {
        fmt::format("{:.2f}", run.time.count() / 1000.)};

      file_s << fmt::format("{};{};{};{}\n",
                            instance_name,
                            optimal_cost,
                            cost,
                            time_us);
    }
  }

  return std::nullopt;
}

template<typename AlgoRun, typename... Params>
requires std::invocable<AlgoRun,
                        const tsp::Matrix<int>&,
                        const tsp::GraphInfo&,
                        const std::optional<int>&,
                        Params...>
static std::optional<tsp::ErrorMeasure> z1z2_measure_trivial_asymmetric(
AlgoRun     algorithm_run_func,
int         max_v,
bool        verbose,
const char* out,
Params... algo_params) noexcept {
  const tsp::GraphInfo graph_info_as {false, false};

  std::ofstream file_as {out};

  if (!file_as.is_open()) {
    fmt::println("[E] Could not open file for writing!");
    return tsp::ErrorMeasure::FILE_ERROR;
  }

  file_as << "Nazwa;Koszt optymalny;Koszt obliczony;Czas [us]\n";

  for (int s {5}; s < std::min(19, max_v); ++s) {
    if (verbose) {
      fmt::print("Random ASymmetric {:>2}: ", s);
    }

    const auto instance_result {
      config::read(fmt::format("./data/rand_atsp/configs/{}_rand_as.ini", s))};
    if (error::handle(instance_result) == tsp::State::ERROR) {
      fmt::println(
      "[E] Make sure the config files are in (./data/rand_atsp/configs/<instance>_rand_as.ini)!");
      return tsp::ErrorMeasure::FILE_ERROR;
    }
    const tsp::Instance instance {std::get<tsp::Instance>(instance_result)};

    std::array<tsp::Result, 3>  cache_runs {};
    std::array<tsp::Result, 10> runs {};

    for (int i {0}; i < 3; ++i) {
      auto result {measured_run(algorithm_run_func,
                                instance.matrix,
                                graph_info_as,
                                std::optional {instance.optimal.cost},
                                algo_params...)};
      if (error::handle(result) == tsp::State::ERROR) {
        return tsp::ErrorMeasure::ALGORITHM_ERROR;
      }
      cache_runs.at(i) = std::move(std::get<tsp::Result>(result));
    }

    if (verbose) {
      fmt::print("[");
    }

    for (int i {0}; i < 10; ++i) {
      auto result_ {measured_run(algorithm_run_func,
                                 instance.matrix,
                                 graph_info_as,
                                 std::optional {instance.optimal.cost},
                                 algo_params...)};
      if (error::handle(result_) == tsp::State::ERROR) [[unlikely]] {
        return tsp::ErrorMeasure::ALGORITHM_ERROR;
      }

      runs.at(i) = std::move(std::get<tsp::Result>(result_));

      if (verbose) {
        fmt::print("-");
      }
    }

    if (verbose) {
      fmt::println("]");
    }

    const std::string instance_name {instance.input_file.filename().string()};
    const int         optimal_cost {instance.optimal.cost};
    for (const tsp::Result& run : runs) {
      const int         cost {run.solution.cost};
      const std::string time_us {
        fmt::format("{:.2f}", run.time.count() / 1000.)};

      file_as << fmt::format("{};{};{};{}\n",
                             instance_name,
                             optimal_cost,
                             cost,
                             time_us);
    }
  }

  return std::nullopt;
}

struct Z3MeasureInstance {
  std::filesystem::path name;
  tsp::Matrix<int>      matrix;
  tsp::GraphInfo        graph_info;
  std::optional<int>    optimal_cost;
  std::optional<int>    calc_tabu_itr;
  std::optional<int>    calc_itr;
};

template<typename Itr>
requires std::forward_iterator<Itr> &&
         std::is_same_v<
         std::remove_cvref_t<typename std::iterator_traits<Itr>::value_type>,
         Z3MeasureInstance> &&
         !std::is_const_v<typename std::iterator_traits<Itr>::value_type>
         static std::optional<tsp::ErrorMeasure> z3_measure_itr_impact(
         Itr         begin,
         Itr         end,
         int         min_itr,
         int         max_itr,
         int         step,
         bool        verbose,
         const char* out) noexcept {
  std::ofstream file {out};

  if (!file.is_open()) {
    return tsp::ErrorMeasure::FILE_ERROR;
  }

  file
  << "Ilosc miast;Nazwa;Koszt optymalny;Koszt obliczony;Ilosc iteracji w tabu;Ilosc iteracji;Czas [us];Blad [%]\n";

  for (Itr it {begin}; it != end; ++it) {
    double last_percent {100.};

    for (int i {min_itr}; i <= max_itr; i += step) {
      if (verbose) {
        fmt::print("Tabu Search (Itr)  [{:<20}] {:>5}: ",
                   it->name.stem().string(),
                   i);
      }

      std::array<tsp::Result, 2> cache_runs {};
      std::array<tsp::Result, 5> runs {};

      for (int j {0}; j < 2; ++j) {
        auto result {measured_run(ts::run,
                                  it->matrix,
                                  it->graph_info,
                                  it->optimal_cost,
                                  i,
                                  i,
                                  *it->calc_tabu_itr)};
        if (error::handle(result) == tsp::State::ERROR) {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }
        cache_runs.at(j) = std::move(std::get<tsp::Result>(result));
      }

      if (verbose) {
        fmt::print("[");
      }

      for (int j {0}; j < 5; ++j) {
        auto result_ {measured_run(ts::run,
                                   it->matrix,
                                   it->graph_info,
                                   it->optimal_cost,
                                   i,
                                   i,
                                   *it->calc_tabu_itr)};
        if (error::handle(result_) == tsp::State::ERROR) [[unlikely]] {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }

        runs.at(j) = std::move(std::get<tsp::Result>(result_));

        if (verbose) {
          fmt::print("-");
        }
      }

      if (verbose) {
        fmt::println("]");
      }

      const std::string instance_name {it->name.stem().string()};
      const int         optimal_cost {it->optimal_cost.value()};
      const int         cost {runs.at(0).solution.cost};
      const int         tabu_itr {*it->calc_tabu_itr};
      const double      error_percent {runs.at(0).error_info->relative_percent};
      const int         v_count {static_cast<int>(it->matrix.size())};

      if (last_percent - error_percent > 0.1) {
        last_percent = error_percent;
        it->calc_itr = i;
      }

      for (const tsp::Result& run : runs) {
        const std::string time_us {
          fmt::format("{:.2f}", run.time.count() / 1000.)};

        file << fmt::format("{};{};{};{};{};{};{};{:.2f}\n",
                            v_count,
                            instance_name,
                            optimal_cost,
                            cost,
                            tabu_itr,
                            i,
                            time_us,
                            error_percent);
      }
    }
  }

  return std::nullopt;
}

template<typename Itr>
requires std::forward_iterator<Itr> &&
         std::is_same_v<
         std::remove_cvref_t<typename std::iterator_traits<Itr>::value_type>,
         Z3MeasureInstance> &&
         !std::is_const_v<typename std::iterator_traits<Itr>::value_type>
         static std::optional<tsp::ErrorMeasure> z3_measure_tabu_impact(
         Itr         begin,
         Itr         end,
         int         min_tabu,
         int         max_tabu,
         int         step,
         int         itr,
         bool        verbose,
         const char* out) noexcept {
  std::ofstream file {out};

  if (!file.is_open()) {
    return tsp::ErrorMeasure::FILE_ERROR;
  }

  file
  << "Ilosc miast;Nazwa;Koszt optymalny;Koszt obliczony;Ilosc iteracji w tabu;Ilosc iteracji;Czas [us];Blad [%]\n";

  for (Itr it {begin}; it != end; ++it) {
    double last_percent {100.};

    for (int i {min_tabu}; i <= max_tabu; i += step) {
      if (verbose) {
        fmt::print("Tabu Search (Tabu) [{:<20}] {:>5}: ",
                   it->name.stem().string(),
                   i);
      }

      std::array<tsp::Result, 2> cache_runs {};
      std::array<tsp::Result, 5> runs {};

      for (int j {0}; j < 2; ++j) {
        auto result {measured_run(ts::run,
                                  it->matrix,
                                  it->graph_info,
                                  it->optimal_cost,
                                  itr,
                                  itr,
                                  i)};
        if (error::handle(result) == tsp::State::ERROR) {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }
        cache_runs.at(j) = std::move(std::get<tsp::Result>(result));
      }

      if (verbose) {
        fmt::print("[");
      }

      for (int j {0}; j < 5; ++j) {
        auto result_ {measured_run(ts::run,
                                   it->matrix,
                                   it->graph_info,
                                   it->optimal_cost,
                                   itr,
                                   itr,
                                   i)};
        if (error::handle(result_) == tsp::State::ERROR) [[unlikely]] {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }

        runs.at(j) = std::move(std::get<tsp::Result>(result_));

        if (verbose) {
          fmt::print("-");
        }
      }

      if (verbose) {
        fmt::println("]");
      }

      const std::string instance_name {it->name.stem().string()};
      const int         optimal_cost {it->optimal_cost.value()};
      const int         cost {runs.at(0).solution.cost};
      const int         tabu_itr {i};
      const double      error_percent {runs.at(0).error_info->relative_percent};
      const int         v_count {static_cast<int>(it->matrix.size())};

      if (last_percent - error_percent > 0.1) {
        last_percent      = error_percent;
        it->calc_tabu_itr = i;
      }

      for (const tsp::Result& run : runs) {
        const std::string time_us {
          fmt::format("{:.2f}", run.time.count() / 1000.)};

        file << fmt::format("{};{};{};{};{};{};{};{:.2f}\n",
                            v_count,
                            instance_name,
                            optimal_cost,
                            cost,
                            tabu_itr,
                            itr,
                            time_us,
                            error_percent);
      }
    }
  }

  return std::nullopt;
}

template<typename Itr>
requires std::forward_iterator<Itr> &&
         std::is_same_v<
         std::remove_cvref_t<typename std::iterator_traits<Itr>::value_type>,
         Z3MeasureInstance> &&
         !std::is_const_v<typename std::iterator_traits<Itr>::value_type>
         static std::optional<tsp::ErrorMeasure> z3_measure_n_impact(
         Itr         begin,
         Itr         end,
         int         itr,
         bool        verbose,
         const char* out) noexcept {
  std::ofstream file {out};

  if (!file.is_open()) {
    return tsp::ErrorMeasure::FILE_ERROR;
  }

  file
  << "Ilosc miast;Nazwa;Koszt optymalny;Koszt obliczony;Ilosc iteracji w tabu;Ilosc iteracji;Czas [us];Blad [%]\n";

  for (Itr it {begin}; it != end; ++it) {
    if (verbose) {
      fmt::print("Tabu Search (N)    [{:<20}] {:>5}: ",
                 it->name.stem().string(),
                 it->matrix.size());
    }

    std::array<tsp::Result, 3> cache_runs {};
    std::array<tsp::Result, 7> runs {};

    for (int j {0}; j < 3; ++j) {
      auto result {measured_run(ts::run,
                                it->matrix,
                                it->graph_info,
                                it->optimal_cost,
                                itr,
                                itr,
                                *it->calc_tabu_itr)};
      if (error::handle(result) == tsp::State::ERROR) {
        return tsp::ErrorMeasure::ALGORITHM_ERROR;
      }
      cache_runs.at(j) = std::move(std::get<tsp::Result>(result));
    }

    if (verbose) {
      fmt::print("[");
    }

    for (int j {0}; j < 7; ++j) {
      auto result_ {measured_run(ts::run,
                                 it->matrix,
                                 it->graph_info,
                                 it->optimal_cost,
                                 itr,
                                 itr,
                                 *it->calc_tabu_itr)};
      if (error::handle(result_) == tsp::State::ERROR) [[unlikely]] {
        return tsp::ErrorMeasure::ALGORITHM_ERROR;
      }

      runs.at(j) = std::move(std::get<tsp::Result>(result_));

      if (verbose) {
        fmt::print("-");
      }
    }

    if (verbose) {
      fmt::println("]");
    }

    const std::string instance_name {it->name.stem().string()};
    const int         optimal_cost {it->optimal_cost.value()};
    const int         tabu_itr {*it->calc_tabu_itr};
    const int         cost {runs.at(0).solution.cost};
    const double      error_percent {runs.at(0).error_info->relative_percent};
    const int         v_count {static_cast<int>(it->matrix.size())};

    for (const tsp::Result& run : runs) {
      const std::string time_us {
        fmt::format("{:.2f}", run.time.count() / 1000.)};

      file << fmt::format("{};{};{};{};{};{};{};{:.2f}\n",
                          v_count,
                          instance_name,
                          optimal_cost,
                          cost,
                          tabu_itr,
                          itr,
                          time_us,
                          error_percent);
    }
  }

  return std::nullopt;
}

template<size_t count, typename Itr>
requires std::forward_iterator<Itr> &&
         std::is_same_v<
         std::remove_cvref_t<typename std::iterator_traits<Itr>::value_type>,
         tsp::Instance>
static std::optional<tsp::ErrorMeasure> z3_measure_all(
Itr         begin,
Itr         end,
int         min_tabu,
int         max_tabu,
int         step_tabu,
int         itr_tabu,
int         min_itr,
int         max_itr,
int         step_itr,
int         itr_n,
bool        verbose,
const char* out_tabu,
const char* out_itr,
const char* out_n) noexcept {
  std::array<Z3MeasureInstance, count> instances {};

  std::optional<tsp::ErrorMeasure> err {std::nullopt};

  int i {0};
  for (Itr it {begin}; it != end; ++it) {
    instances.at(i).name          = std::move(it->input_file);
    instances.at(i).matrix        = std::move(it->matrix);
    instances.at(i).graph_info    = std::move(it->graph_info);
    instances.at(i).optimal_cost  = it->optimal.cost;
    instances.at(i).calc_tabu_itr = std::nullopt;
    instances.at(i).calc_itr      = std::nullopt;

    ++i;
  }

  err = z3_measure_tabu_impact(instances.begin(),
                               instances.end(),
                               min_tabu,
                               max_tabu,
                               step_tabu,
                               itr_tabu,
                               verbose,
                               out_tabu);
  if (err.has_value()) [[unlikely]] {
    return err;
  }

  err = z3_measure_itr_impact(instances.begin(),
                              instances.end(),
                              min_itr,
                              max_itr,
                              step_itr,
                              verbose,
                              out_itr);
  if (err.has_value()) [[unlikely]] {
    return err;
  }

  err = z3_measure_n_impact(instances.begin(),
                            instances.end(),
                            itr_n,
                            verbose,
                            out_n);

  return err;
}

struct Z4MeasureInstance {
  std::filesystem::path name;
  tsp::Matrix<int>      matrix;
  tsp::GraphInfo        graph_info;
  std::optional<int>    optimal_cost;
  std::optional<int>    calc_children_per_itr;
  std::optional<int>    calc_population_size;
  std::optional<int>    calc_max_children_per_pair;
  std::optional<int>    calc_max_v_count_crossover;
  std::optional<int>    calc_mutations_per_1000;
};

template<typename Itr>
requires std::forward_iterator<Itr> &&
         std::is_same_v<
         std::remove_cvref_t<typename std::iterator_traits<Itr>::value_type>,
         Z4MeasureInstance> &&
         !std::is_const_v<typename std::iterator_traits<Itr>::value_type>
         static std::optional<tsp::ErrorMeasure>
         z4_measure_children_per_itr_impact(Itr         begin,
                                            Itr         end,
                                            int         itr,
                                            int         min_children_per_itr,
                                            int         max_children_per_itr,
                                            int         step_children_per_itr,
                                            int         population_size,
                                            int         max_children_per_pair,
                                            int         max_v_count_crossover,
                                            int         mutations_per_1000,
                                            bool        verbose,
                                            const char* out) noexcept {
  std::ofstream file {out};

  if (!file.is_open()) {
    return tsp::ErrorMeasure::FILE_ERROR;
  }

  file
  << "Ilosc miast;Nazwa;Koszt optymalny;Koszt obliczony;Ilosc iteracji;Rozmiar populacji;Maks ilosc dzieci na iteracje;Maks dzieci na pare;Maks wierzcholkow krzyzowania;Szansa na mutacje [%];Czas [us];Blad [%]\n";

  for (Itr it {begin}; it != end; ++it) {
    double last_percent {100.};

    for (int i {min_children_per_itr}; i <= max_children_per_itr;
         i += step_children_per_itr) {
      if (verbose) {
        fmt::print("Genetic (CPI)     [{:<20}] {:>5}: ",
                   it->name.stem().string(),
                   i);
      }

      std::array<tsp::Result, 3>  cache_runs {};
      std::array<tsp::Result, 10> runs {};

      for (int j {0}; j < 3; ++j) {
        auto result {measured_run(gen::run,
                                  it->matrix,
                                  it->graph_info,
                                  it->optimal_cost,
                                  itr,
                                  population_size,
                                  i,
                                  max_children_per_pair,
                                  max_v_count_crossover,
                                  mutations_per_1000)};
        if (error::handle(result) == tsp::State::ERROR) {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }
        cache_runs.at(j) = std::move(std::get<tsp::Result>(result));
      }

      if (verbose) {
        fmt::print("[");
      }

      for (int j {0}; j < 10; ++j) {
        auto result_ {measured_run(gen::run,
                                   it->matrix,
                                   it->graph_info,
                                   it->optimal_cost,
                                   itr,
                                   population_size,
                                   i,
                                   max_children_per_pair,
                                   max_v_count_crossover,
                                   mutations_per_1000)};
        if (error::handle(result_) == tsp::State::ERROR) [[unlikely]] {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }

        runs.at(j) = std::move(std::get<tsp::Result>(result_));

        if (verbose) {
          fmt::print("-");
        }
      }

      if (verbose) {
        fmt::println("]");
      }

      const int         v_count {static_cast<int>(it->matrix.size())};
      const std::string instance_name {it->name.stem().string()};
      const int         optimal_cost {it->optimal_cost.value()};
      const int         itr_count {itr};
      const int         pop_size {population_size};
      const int         children_per_itr {i};
      const int         children_per_pair {max_children_per_pair};
      const int         v_count_crossover {max_v_count_crossover};
      const int         mutations {mutations_per_1000};

      for (const tsp::Result& run : runs) {
        const int         cost {run.solution.cost};
        const std::string time_us {
          fmt::format("{:.2f}", run.time.count() / 1000.)};
        const double error_percent {run.error_info->relative_percent};

        if (last_percent - error_percent > 0.1) {
          last_percent              = error_percent;
          it->calc_children_per_itr = i;
        }

        file << fmt::format("{};{};{};{};{};{};{};{};{};{};{};{:.2f}\n",
                            v_count,
                            instance_name,
                            optimal_cost,
                            cost,
                            itr_count,
                            pop_size,
                            children_per_itr,
                            children_per_pair,
                            v_count_crossover,
                            mutations,
                            time_us,
                            error_percent);
      }
    }
  }

  return std::nullopt;
}

template<typename Itr>
requires std::forward_iterator<Itr> &&
         std::is_same_v<
         std::remove_cvref_t<typename std::iterator_traits<Itr>::value_type>,
         Z4MeasureInstance> &&
         !std::is_const_v<typename std::iterator_traits<Itr>::value_type>
         static std::optional<tsp::ErrorMeasure>
         z4_measure_population_size_impact(Itr         begin,
                                           Itr         end,
                                           int         itr,
                                           int         min_population_size,
                                           int         max_population_size,
                                           int         step_population_size,
                                           int         max_children_per_pair,
                                           int         max_v_count_crossover,
                                           int         mutations_per_1000,
                                           bool        verbose,
                                           const char* out) noexcept {
  std::ofstream file {out};

  if (!file.is_open()) {
    return tsp::ErrorMeasure::FILE_ERROR;
  }

  file
  << "Ilosc miast;Nazwa;Koszt optymalny;Koszt obliczony;Ilosc iteracji;Rozmiar populacji;Maks ilosc dzieci na iteracje;Maks dzieci na pare;Maks wierzcholkow krzyzowania;Szansa na mutacje [%];Czas [us];Blad [%]\n";

  for (Itr it {begin}; it != end; ++it) {
    double last_percent {100.};

    for (int i {min_population_size}; i <= max_population_size;
         i += step_population_size) {
      if (verbose) {
        fmt::print("Genetic (Pop)     [{:<20}] {:>5}: ",
                   it->name.stem().string(),
                   i);
      }

      std::array<tsp::Result, 3>  cache_runs {};
      std::array<tsp::Result, 10> runs {};

      for (int j {0}; j < 3; ++j) {
        auto result {measured_run(gen::run,
                                  it->matrix,
                                  it->graph_info,
                                  it->optimal_cost,
                                  itr,
                                  i,
                                  *it->calc_children_per_itr,
                                  max_children_per_pair,
                                  max_v_count_crossover,
                                  mutations_per_1000)};
        if (error::handle(result) == tsp::State::ERROR) {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }
        cache_runs.at(j) = std::move(std::get<tsp::Result>(result));
      }

      if (verbose) {
        fmt::print("[");
      }

      for (int j {0}; j < 10; ++j) {
        auto result_ {measured_run(gen::run,
                                   it->matrix,
                                   it->graph_info,
                                   it->optimal_cost,
                                   itr,
                                   i,
                                   *it->calc_children_per_itr,
                                   max_children_per_pair,
                                   max_v_count_crossover,
                                   mutations_per_1000)};
        if (error::handle(result_) == tsp::State::ERROR) [[unlikely]] {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }

        runs.at(j) = std::move(std::get<tsp::Result>(result_));

        if (verbose) {
          fmt::print("-");
        }
      }

      if (verbose) {
        fmt::println("]");
      }

      const int         v_count {static_cast<int>(it->matrix.size())};
      const std::string instance_name {it->name.stem().string()};
      const int         optimal_cost {it->optimal_cost.value()};
      const int         itr_count {itr};
      const int         pop_size {i};
      const int         children_per_itr {*it->calc_children_per_itr};
      const int         children_per_pair {max_children_per_pair};
      const int         v_count_crossover {max_v_count_crossover};
      const int         mutations {mutations_per_1000};

      for (const tsp::Result& run : runs) {
        const int         cost {run.solution.cost};
        const std::string time_us {
          fmt::format("{:.2f}", run.time.count() / 1000.)};
        const double error_percent {run.error_info->relative_percent};

        if (last_percent - error_percent > 0.1) {
          last_percent             = error_percent;
          it->calc_population_size = i;
        }

        file << fmt::format("{};{};{};{};{};{};{};{};{};{};{};{:.2f}\n",
                            v_count,
                            instance_name,
                            optimal_cost,
                            cost,
                            itr_count,
                            pop_size,
                            children_per_itr,
                            children_per_pair,
                            v_count_crossover,
                            mutations,
                            time_us,
                            error_percent);
      }
    }
  }

  return std::nullopt;
}

template<typename Itr>
requires std::forward_iterator<Itr> &&
         std::is_same_v<
         std::remove_cvref_t<typename std::iterator_traits<Itr>::value_type>,
         Z4MeasureInstance> &&
         !std::is_const_v<typename std::iterator_traits<Itr>::value_type>
         static std::optional<tsp::ErrorMeasure>
         z4_measure_max_children_per_pair_impact(Itr begin,
                                                 Itr end,
                                                 int itr,
                                                 int min_max_children_per_pair,
                                                 int max_max_children_per_pair,
                                                 int step_max_children_per_pair,
                                                 int max_v_count_crossover,
                                                 int mutations_per_1000,
                                                 bool        verbose,
                                                 const char* out) noexcept {
  std::ofstream file {out};

  if (!file.is_open()) {
    return tsp::ErrorMeasure::FILE_ERROR;
  }

  file
  << "Ilosc miast;Nazwa;Koszt optymalny;Koszt obliczony;Ilosc iteracji;Rozmiar populacji;Maks ilosc dzieci na iteracje;Maks dzieci na pare;Maks wierzcholkow krzyzowania;Szansa na mutacje [%];Czas [us];Blad [%]\n";

  for (Itr it {begin}; it != end; ++it) {
    double last_percent {100.};

    for (int i {min_max_children_per_pair}; i <= max_max_children_per_pair;
         i += step_max_children_per_pair) {
      if (verbose) {
        fmt::print("Genetic (CPP)     [{:<20}] {:>5}: ",
                   it->name.stem().string(),
                   i);
      }

      std::array<tsp::Result, 3>  cache_runs {};
      std::array<tsp::Result, 10> runs {};

      for (int j {0}; j < 3; ++j) {
        auto result {measured_run(gen::run,
                                  it->matrix,
                                  it->graph_info,
                                  it->optimal_cost,
                                  itr,
                                  *it->calc_population_size,
                                  *it->calc_children_per_itr,
                                  i,
                                  max_v_count_crossover,
                                  mutations_per_1000)};
        if (error::handle(result) == tsp::State::ERROR) {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }
        cache_runs.at(j) = std::move(std::get<tsp::Result>(result));
      }

      if (verbose) {
        fmt::print("[");
      }

      for (int j {0}; j < 10; ++j) {
        auto result_ {measured_run(gen::run,
                                   it->matrix,
                                   it->graph_info,
                                   it->optimal_cost,
                                   itr,
                                   *it->calc_population_size,
                                   *it->calc_children_per_itr,
                                   i,
                                   max_v_count_crossover,
                                   mutations_per_1000)};
        if (error::handle(result_) == tsp::State::ERROR) [[unlikely]] {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }

        runs.at(j) = std::move(std::get<tsp::Result>(result_));

        if (verbose) {
          fmt::print("-");
        }
      }

      if (verbose) {
        fmt::println("]");
      }

      const int         v_count {static_cast<int>(it->matrix.size())};
      const std::string instance_name {it->name.stem().string()};
      const int         optimal_cost {it->optimal_cost.value()};
      const int         itr_count {itr};
      const int         pop_size {*it->calc_population_size};
      const int         children_per_itr {*it->calc_children_per_itr};
      const int         children_per_pair {i};
      const int         v_count_crossover {max_v_count_crossover};
      const int         mutations {mutations_per_1000};

      for (const tsp::Result& run : runs) {
        const int         cost {run.solution.cost};
        const std::string time_us {
          fmt::format("{:.2f}", run.time.count() / 1000.)};
        const double error_percent {run.error_info->relative_percent};

        if (last_percent - error_percent > 0.1) {
          last_percent                   = error_percent;
          it->calc_max_children_per_pair = i;
        }

        file << fmt::format("{};{};{};{};{};{};{};{};{};{};{};{:.2f}\n",
                            v_count,
                            instance_name,
                            optimal_cost,
                            cost,
                            itr_count,
                            pop_size,
                            children_per_itr,
                            children_per_pair,
                            v_count_crossover,
                            mutations,
                            time_us,
                            error_percent);
      }
    }
  }

  return std::nullopt;
}

template<typename Itr>
requires std::forward_iterator<Itr> &&
         std::is_same_v<
         std::remove_cvref_t<typename std::iterator_traits<Itr>::value_type>,
         Z4MeasureInstance> &&
         !std::is_const_v<typename std::iterator_traits<Itr>::value_type>
         static std::optional<tsp::ErrorMeasure>
         z4_measure_max_v_count_crossover_impact(Itr begin,
                                                 Itr end,
                                                 int itr,
                                                 int min_max_v_count_crossover,
                                                 int max_max_v_count_crossover,
                                                 int step_max_v_count_crossover,
                                                 int mutations_per_1000,
                                                 bool        verbose,
                                                 const char* out) noexcept {
  std::ofstream file {out};

  if (!file.is_open()) {
    return tsp::ErrorMeasure::FILE_ERROR;
  }

  file
  << "Ilosc miast;Nazwa;Koszt optymalny;Koszt obliczony;Ilosc iteracji;Rozmiar populacji;Maks ilosc dzieci na iteracje;Maks dzieci na pare;Maks wierzcholkow krzyzowania;Szansa na mutacje [%];Czas [us];Blad [%]\n";

  for (Itr it {begin}; it != end; ++it) {
    double last_percent {100.};

    for (int i {min_max_v_count_crossover}; i <= max_max_v_count_crossover;
         i += step_max_v_count_crossover) {
      if (verbose) {
        fmt::print("Genetic (VCC)     [{:<20}] {:>5}: ",
                   it->name.stem().string(),
                   i);
      }

      std::array<tsp::Result, 3>  cache_runs {};
      std::array<tsp::Result, 10> runs {};

      for (int j {0}; j < 3; ++j) {
        auto result {measured_run(gen::run,
                                  it->matrix,
                                  it->graph_info,
                                  it->optimal_cost,
                                  itr,
                                  *it->calc_population_size,
                                  *it->calc_children_per_itr,
                                  *it->calc_max_children_per_pair,
                                  i,
                                  mutations_per_1000)};
        if (error::handle(result) == tsp::State::ERROR) {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }
        cache_runs.at(j) = std::move(std::get<tsp::Result>(result));
      }

      if (verbose) {
        fmt::print("[");
      }

      for (int j {0}; j < 10; ++j) {
        auto result_ {measured_run(gen::run,
                                   it->matrix,
                                   it->graph_info,
                                   it->optimal_cost,
                                   itr,
                                   *it->calc_population_size,
                                   *it->calc_children_per_itr,
                                   *it->calc_max_children_per_pair,
                                   i,
                                   mutations_per_1000)};
        if (error::handle(result_) == tsp::State::ERROR) [[unlikely]] {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }

        runs.at(j) = std::move(std::get<tsp::Result>(result_));

        if (verbose) {
          fmt::print("-");
        }
      }

      if (verbose) {
        fmt::println("]");
      }

      const int         v_count {static_cast<int>(it->matrix.size())};
      const std::string instance_name {it->name.stem().string()};
      const int         optimal_cost {it->optimal_cost.value()};
      const int         itr_count {itr};
      const int         pop_size {*it->calc_population_size};
      const int         children_per_itr {*it->calc_children_per_itr};
      const int         children_per_pair {*it->calc_max_children_per_pair};
      const int         v_count_crossover {i};
      const int         mutations {mutations_per_1000};

      for (const tsp::Result& run : runs) {
        const int         cost {run.solution.cost};
        const std::string time_us {
          fmt::format("{:.2f}", run.time.count() / 1000.)};
        const double error_percent {run.error_info->relative_percent};

        if (last_percent - error_percent > 0.1) {
          last_percent                   = error_percent;
          it->calc_max_v_count_crossover = i;
        }

        file << fmt::format("{};{};{};{};{};{};{};{};{};{};{};{:.2f}\n",
                            v_count,
                            instance_name,
                            optimal_cost,
                            cost,
                            itr_count,
                            pop_size,
                            children_per_itr,
                            children_per_pair,
                            v_count_crossover,
                            mutations,
                            time_us,
                            error_percent);
      }
    }
  }

  return std::nullopt;
}

template<typename Itr>
requires std::forward_iterator<Itr> &&
         std::is_same_v<
         std::remove_cvref_t<typename std::iterator_traits<Itr>::value_type>,
         Z4MeasureInstance> &&
         !std::is_const_v<typename std::iterator_traits<Itr>::value_type>
         static std::optional<tsp::ErrorMeasure>
         z4_measure_mutations_per_1000_impact(Itr  begin,
                                              Itr  end,
                                              int  itr,
                                              int  min_mutations_per_1000,
                                              int  max_mutations_per_1000,
                                              int  step_mutations_per_1000,
                                              bool verbose,
                                              const char* out) noexcept {
  std::ofstream file {out};

  if (!file.is_open()) {
    return tsp::ErrorMeasure::FILE_ERROR;
  }

  file
  << "Ilosc miast;Nazwa;Koszt optymalny;Koszt obliczony;Ilosc iteracji;Rozmiar populacji;Maks ilosc dzieci na iteracje;Maks dzieci na pare;Maks wierzcholkow krzyzowania;Szansa na mutacje [%];Czas [us];Blad [%]\n";

  for (Itr it {begin}; it != end; ++it) {
    double last_percent {100.};

    for (int i {min_mutations_per_1000}; i <= max_mutations_per_1000;
         i += step_mutations_per_1000) {
      if (verbose) {
        fmt::print("Genetic (Mut)     [{:<20}] {:>5}: ",
                   it->name.stem().string(),
                   i);
      }

      std::array<tsp::Result, 3>  cache_runs {};
      std::array<tsp::Result, 10> runs {};

      for (int j {0}; j < 3; ++j) {
        auto result {measured_run(gen::run,
                                  it->matrix,
                                  it->graph_info,
                                  it->optimal_cost,
                                  itr,
                                  *it->calc_population_size,
                                  *it->calc_children_per_itr,
                                  *it->calc_max_children_per_pair,
                                  *it->calc_max_v_count_crossover,
                                  i)};
        if (error::handle(result) == tsp::State::ERROR) {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }
        cache_runs.at(j) = std::move(std::get<tsp::Result>(result));
      }

      if (verbose) {
        fmt::print("[");
      }

      for (int j {0}; j < 10; ++j) {
        auto result_ {measured_run(gen::run,
                                   it->matrix,
                                   it->graph_info,
                                   it->optimal_cost,
                                   itr,
                                   *it->calc_population_size,
                                   *it->calc_children_per_itr,
                                   *it->calc_max_children_per_pair,
                                   *it->calc_max_v_count_crossover,
                                   i)};
        if (error::handle(result_) == tsp::State::ERROR) [[unlikely]] {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }

        runs.at(j) = std::move(std::get<tsp::Result>(result_));

        if (verbose) {
          fmt::print("-");
        }
      }

      if (verbose) {
        fmt::println("]");
      }

      const int         v_count {static_cast<int>(it->matrix.size())};
      const std::string instance_name {it->name.stem().string()};
      const int         optimal_cost {it->optimal_cost.value()};
      const int         itr_count {itr};
      const int         pop_size {*it->calc_population_size};
      const int         children_per_itr {*it->calc_children_per_itr};
      const int         children_per_pair {*it->calc_max_children_per_pair};
      const int         v_count_crossover {*it->calc_max_v_count_crossover};
      const int         mutations {i};

      for (const tsp::Result& run : runs) {
        const int         cost {run.solution.cost};
        const std::string time_us {
          fmt::format("{:.2f}", run.time.count() / 1000.)};
        const double error_percent {run.error_info->relative_percent};

        if (last_percent - error_percent > 0.1) {
          last_percent                = error_percent;
          it->calc_mutations_per_1000 = i;
        }

        file << fmt::format("{};{};{};{};{};{};{};{};{};{};{};{:.2f}\n",
                            v_count,
                            instance_name,
                            optimal_cost,
                            cost,
                            itr_count,
                            pop_size,
                            children_per_itr,
                            children_per_pair,
                            v_count_crossover,
                            mutations,
                            time_us,
                            error_percent);
      }
    }
  }

  return std::nullopt;
}

template<typename Itr>
requires std::forward_iterator<Itr> &&
         std::is_same_v<
         std::remove_cvref_t<typename std::iterator_traits<Itr>::value_type>,
         Z4MeasureInstance>
static std::optional<tsp::ErrorMeasure> z4_measure_itr_impact(
Itr         begin,
Itr         end,
int         min_itr,
int         max_itr,
int         step_itr,
bool        verbose,
const char* out) noexcept {
  std::ofstream file {out};

  if (!file.is_open()) {
    return tsp::ErrorMeasure::FILE_ERROR;
  }

  file
  << "Ilosc miast;Nazwa;Koszt optymalny;Koszt obliczony;Ilosc iteracji;Rozmiar populacji;Maks ilosc dzieci na iteracje;Maks dzieci na pare;Maks wierzcholkow krzyzowania;Szansa na mutacje [%];Czas [us];Blad [%]\n";

  for (Itr it {begin}; it != end; ++it) {
    for (int i {min_itr}; i <= max_itr; i += step_itr) {
      if (verbose) {
        fmt::print("Genetic (Itr)     [{:<20}] {:>5}: ",
                   it->name.stem().string(),
                   i);
      }

      std::array<tsp::Result, 3>  cache_runs {};
      std::array<tsp::Result, 10> runs {};

      for (int j {0}; j < 3; ++j) {
        auto result {measured_run(gen::run,
                                  it->matrix,
                                  it->graph_info,
                                  it->optimal_cost,
                                  i,
                                  *it->calc_population_size,
                                  *it->calc_children_per_itr,
                                  *it->calc_max_children_per_pair,
                                  *it->calc_max_v_count_crossover,
                                  *it->calc_mutations_per_1000)};
        if (error::handle(result) == tsp::State::ERROR) {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }
        cache_runs.at(j) = std::move(std::get<tsp::Result>(result));
      }

      if (verbose) {
        fmt::print("[");
      }

      for (int j {0}; j < 10; ++j) {
        auto result_ {measured_run(gen::run,
                                   it->matrix,
                                   it->graph_info,
                                   it->optimal_cost,
                                   i,
                                   *it->calc_population_size,
                                   *it->calc_children_per_itr,
                                   *it->calc_max_children_per_pair,
                                   *it->calc_max_v_count_crossover,
                                   *it->calc_mutations_per_1000)};
        if (error::handle(result_) == tsp::State::ERROR) [[unlikely]] {
          return tsp::ErrorMeasure::ALGORITHM_ERROR;
        }

        runs.at(j) = std::move(std::get<tsp::Result>(result_));

        if (verbose) {
          fmt::print("-");
        }
      }

      if (verbose) {
        fmt::println("]");
      }

      const int         v_count {static_cast<int>(it->matrix.size())};
      const std::string instance_name {it->name.stem().string()};
      const int         optimal_cost {it->optimal_cost.value()};
      const int         itr_count {i};
      const int         pop_size {*it->calc_population_size};
      const int         children_per_itr {*it->calc_children_per_itr};
      const int         children_per_pair {*it->calc_max_children_per_pair};
      const int         v_count_crossover {*it->calc_max_v_count_crossover};
      const int         mutations {*it->calc_mutations_per_1000};

      for (const tsp::Result& run : runs) {
        const int         cost {run.solution.cost};
        const std::string time_us {
          fmt::format("{:.2f}", run.time.count() / 1000.)};
        const double error_percent {run.error_info->relative_percent};

        file << fmt::format("{};{};{};{};{};{};{};{};{};{};{};{:.2f}\n",
                            v_count,
                            instance_name,
                            optimal_cost,
                            cost,
                            itr_count,
                            pop_size,
                            children_per_itr,
                            children_per_pair,
                            v_count_crossover,
                            mutations,
                            time_us,
                            error_percent);
      }
    }
  }

  return std::nullopt;
}

template<typename Itr>
requires std::forward_iterator<Itr> &&
         std::is_same_v<
         std::remove_cvref_t<typename std::iterator_traits<Itr>::value_type>,
         Z4MeasureInstance>
static std::optional<tsp::ErrorMeasure> z4_measure_n_impact(
Itr         begin,
Itr         end,
int         itr,
int         population_size,
int         children_per_itr,
int         mutations_per_1000,
bool        verbose,
const char* out) noexcept {
  std::ofstream file {out};

  if (!file.is_open()) {
    return tsp::ErrorMeasure::FILE_ERROR;
  }

  file
  << "Ilosc miast;Nazwa;Koszt optymalny;Koszt obliczony;Ilosc iteracji;Rozmiar populacji;Maks ilosc dzieci na iteracje;Maks dzieci na pare;Maks wierzcholkow krzyzowania;Szansa na mutacje [%];Czas [us];Blad [%]\n";

  for (Itr it {begin}; it != end; ++it) {
    if (verbose) {
      fmt::print("Genetic (N)       [{:<20}] {:>5}: ",
                 it->name.stem().string(),
                 it->matrix.size());
    }

    std::array<tsp::Result, 3>  cache_runs {};
    std::array<tsp::Result, 10> runs {};

    for (int j {0}; j < 3; ++j) {
      auto result {measured_run(gen::run,
                                it->matrix,
                                it->graph_info,
                                it->optimal_cost,
                                itr,
                                population_size,
                                children_per_itr,
                                *it->calc_max_children_per_pair,
                                *it->calc_max_v_count_crossover,
                                mutations_per_1000)};
      if (error::handle(result) == tsp::State::ERROR) {
        return tsp::ErrorMeasure::ALGORITHM_ERROR;
      }
      cache_runs.at(j) = std::move(std::get<tsp::Result>(result));
    }

    if (verbose) {
      fmt::print("[");
    }

    for (int j {0}; j < 10; ++j) {
      auto result_ {measured_run(gen::run,
                                 it->matrix,
                                 it->graph_info,
                                 it->optimal_cost,
                                 itr,
                                 population_size,
                                 children_per_itr,
                                 *it->calc_max_children_per_pair,
                                 *it->calc_max_v_count_crossover,
                                 mutations_per_1000)};
      if (error::handle(result_) == tsp::State::ERROR) [[unlikely]] {
        return tsp::ErrorMeasure::ALGORITHM_ERROR;
      }

      runs.at(j) = std::move(std::get<tsp::Result>(result_));

      if (verbose) {
        fmt::print("-");
      }
    }

    if (verbose) {
      fmt::println("]");
    }

    const int         v_count {static_cast<int>(it->matrix.size())};
    const std::string instance_name {it->name.stem().string()};
    const int         optimal_cost {it->optimal_cost.value()};
    const int         itr_count {itr};
    const int         pop_size {population_size};
    const int         children_per_pair {*it->calc_max_children_per_pair};
    const int         v_count_crossover {*it->calc_max_v_count_crossover};
    const int         mutations {mutations_per_1000};

    for (const tsp::Result& run : runs) {
      const int         cost {run.solution.cost};
      const std::string time_us {
        fmt::format("{:.2f}", run.time.count() / 1000.)};
      const double error_percent {run.error_info->relative_percent};

      file << fmt::format("{};{};{};{};{};{};{};{};{};{};{};{:.2f}\n",
                          v_count,
                          instance_name,
                          optimal_cost,
                          cost,
                          itr_count,
                          pop_size,
                          children_per_itr,
                          children_per_pair,
                          v_count_crossover,
                          mutations,
                          time_us,
                          error_percent);
    }
  }

  return std::nullopt;
}

template<size_t count, typename Itr>
requires std::forward_iterator<Itr> &&
         std::is_same_v<
         std::remove_cvref_t<typename std::iterator_traits<Itr>::value_type>,
         tsp::Instance>
static std::optional<tsp::ErrorMeasure> z4_measure_all(
Itr         begin,
Itr         end,
int         default_itr,
int         default_population_size,
int         default_children_per_itr,
int         default_max_children_per_pair,
int         default_max_v_count_crossover,
int         default_mutations_per_1000,
int         min_children_per_itr,
int         max_children_per_itr,
int         step_children_per_itr,
int         min_population_size,
int         max_population_size,
int         step_population_size,
int         min_max_children_per_pair,
int         max_max_children_per_pair,
int         step_max_children_per_pair,
int         min_max_v_count_crossover,
int         max_max_v_count_crossover,
int         step_max_v_count_crossover,
int         min_mutations_per_1000,
int         max_mutations_per_1000,
int         step_mutations_per_1000,
int         min_itr,
int         max_itr,
int         step_itr,
bool        verbose,
const char* out_children_per_itr,
const char* out_population_size,
const char* out_max_children_per_pair,
const char* out_max_v_count_crossover,
const char* out_mutations_per_1000,
const char* out_itr,
const char* out_n) noexcept {
  std::array<Z4MeasureInstance, count> instances {};

  std::optional<tsp::ErrorMeasure> err {std::nullopt};

  int i {0};
  for (Itr itr {begin}; itr != end; ++itr) {
    instances.at(i).name                       = std::move(itr->input_file);
    instances.at(i).matrix                     = std::move(itr->matrix);
    instances.at(i).graph_info                 = std::move(itr->graph_info);
    instances.at(i).optimal_cost               = itr->optimal.cost;
    instances.at(i).calc_children_per_itr      = std::nullopt;
    instances.at(i).calc_population_size       = std::nullopt;
    instances.at(i).calc_max_children_per_pair = std::nullopt;
    instances.at(i).calc_max_v_count_crossover = std::nullopt;
    instances.at(i).calc_mutations_per_1000    = std::nullopt;

    ++i;
  }

  err = z4_measure_children_per_itr_impact(instances.begin(),
                                           instances.end(),
                                           default_itr,
                                           min_children_per_itr,
                                           max_children_per_itr,
                                           step_children_per_itr,
                                           default_population_size,
                                           default_max_children_per_pair,
                                           default_max_v_count_crossover,
                                           default_mutations_per_1000,
                                           verbose,
                                           out_children_per_itr);
  if (err.has_value()) {
    return err;
  }

  err = z4_measure_population_size_impact(instances.begin(),
                                          instances.end(),
                                          default_itr,
                                          min_population_size,
                                          max_population_size,
                                          step_population_size,
                                          default_max_children_per_pair,
                                          default_max_v_count_crossover,
                                          default_mutations_per_1000,
                                          verbose,
                                          out_population_size);
  if (err.has_value()) {
    return err;
  }

  err = z4_measure_max_children_per_pair_impact(instances.begin(),
                                                instances.end(),
                                                default_itr,
                                                min_max_children_per_pair,
                                                max_max_children_per_pair,
                                                step_max_children_per_pair,
                                                default_max_v_count_crossover,
                                                default_mutations_per_1000,
                                                verbose,
                                                out_max_children_per_pair);
  if (err.has_value()) {
    return err;
  }

  err = z4_measure_max_v_count_crossover_impact(instances.begin(),
                                                instances.end(),
                                                default_itr,
                                                min_max_v_count_crossover,
                                                max_max_v_count_crossover,
                                                step_max_v_count_crossover,
                                                default_mutations_per_1000,
                                                verbose,
                                                out_max_v_count_crossover);
  if (err.has_value()) {
    return err;
  }

  err = z4_measure_mutations_per_1000_impact(instances.begin(),
                                             instances.end(),
                                             default_itr,
                                             min_mutations_per_1000,
                                             max_mutations_per_1000,
                                             step_mutations_per_1000,
                                             verbose,
                                             out_mutations_per_1000);
  if (err.has_value()) {
    return err;
  }

  err = z4_measure_itr_impact(instances.begin(),
                              instances.end(),
                              min_itr,
                              max_itr,
                              step_itr,
                              verbose,
                              out_itr);
  if (err.has_value()) {
    return err;
  }

  err = z4_measure_n_impact(instances.begin(),
                            instances.end(),
                            default_itr,
                            default_population_size,
                            default_children_per_itr,
                            default_mutations_per_1000,
                            verbose,
                            out_n);

  return err;
}

static std::optional<tsp::ErrorMeasure> nearest_neighbour(
bool verbose) noexcept {
  if (verbose) {
    fmt::print("---\nMeasuring Nearest Neighbour\n");
  }

  std::optional<tsp::ErrorMeasure> err {std::nullopt};

  err =
  z1z2_measure_trivial_symmetric(nn::run, 20, verbose, "./measure_nn_s.csv");
  if (err.has_value()) {
    return err;
  }
  err =
  z1z2_measure_trivial_asymmetric(nn::run, 19, verbose, "./measure_nn_as.csv");
  if (err.has_value()) {
    return err;
  }

  if (verbose) {
    fmt::print("OK\n");
  } else {
    fmt::print("NN: DONE\n");
  }
  return std::nullopt;
}

static std::optional<tsp::ErrorMeasure> brute_force(bool verbose) noexcept {
  if (verbose) {
    fmt::print("---\nMeasuring Brute Force\n");
  }

  std::optional<tsp::ErrorMeasure> err {std::nullopt};

  err =
  z1z2_measure_trivial_symmetric(bf::run, 11, verbose, "./measure_bf_s.csv");
  if (err.has_value()) {
    return err;
  }
  err =
  z1z2_measure_trivial_asymmetric(bf::run, 12, verbose, "./measure_bf_as.csv");
  if (err.has_value()) {
    return err;
  }

  if (verbose) {
    fmt::print("OK\n");
  } else {
    fmt::print("BF: DONE\n");
  }
  return std::nullopt;
}

static std::optional<tsp::ErrorMeasure> random(bool verbose) noexcept {
  if (verbose) {
    fmt::print("---\nMeasuring Random\n");
  }

  std::optional<tsp::ErrorMeasure> err {std::nullopt};

  err = z1z2_measure_trivial_symmetric(random::run,
                                       20,
                                       verbose,
                                       "./measure_r_s.csv",
                                       4000);
  if (err.has_value()) {
    return err;
  }
  err = z1z2_measure_trivial_asymmetric(random::run,
                                        19,
                                        verbose,
                                        "./measure_r_as.csv",
                                        4000);
  if (err.has_value()) {
    return err;
  }

  if (verbose) {
    fmt::print("OK\n");
  } else {
    fmt::print("R: DONE\n");
  }
  return std::nullopt;
}

static std::optional<tsp::ErrorMeasure> bxb_least_cost(bool verbose) noexcept {
  if (verbose) {
    fmt::print("---\nMeasuring BxB Least Cost\n");
  }

  std::optional<tsp::ErrorMeasure> err {std::nullopt};

  err = z1z2_measure_trivial_symmetric(bxb::lc::run,
                                       16,
                                       verbose,
                                       "./measure_lc_s.csv");
  if (err.has_value()) {
    return err;
  }
  err = z1z2_measure_trivial_asymmetric(bxb::lc::run,
                                        18,
                                        verbose,
                                        "./measure_lc_as.csv");
  if (err.has_value()) {
    return err;
  }

  if (verbose) {
    fmt::print("OK\n");
  } else {
    fmt::print("LC: DONE\n");
  }
  return std::nullopt;
}

static std::optional<tsp::ErrorMeasure> bxb_bfs(bool verbose) noexcept {
  if (verbose) {
    fmt::print("---\nMeasuring BxB Breadth First Search\n");
  }

  std::optional<tsp::ErrorMeasure> err {std::nullopt};

  err = z1z2_measure_trivial_symmetric(bxb::bfs::run,
                                       14,
                                       verbose,
                                       "./measure_bb_s.csv");
  if (err.has_value()) {
    return err;
  }
  err = z1z2_measure_trivial_asymmetric(bxb::bfs::run,
                                        17,
                                        verbose,
                                        "./measure_bb_as.csv");
  if (err.has_value()) {
    return err;
  }

  if (verbose) {
    fmt::print("OK\n");
  } else {
    fmt::print("BB: DONE\n");
  }
  return std::nullopt;
}

static std::optional<tsp::ErrorMeasure> bxb_dfs(bool verbose) noexcept {
  if (verbose) {
    fmt::print("---\nMeasuring BxB Depth First Search\n");
  }

  std::optional<tsp::ErrorMeasure> err {std::nullopt};

  err = z1z2_measure_trivial_symmetric(bxb::dfs::run,
                                       19,
                                       verbose,
                                       "./measure_bd_s.csv");
  if (err.has_value()) {
    return err;
  }
  err = z1z2_measure_trivial_asymmetric(bxb::dfs::run,
                                        17,
                                        verbose,
                                        "./measure_bd_as.csv");
  if (err.has_value()) {
    return err;
  }

  if (verbose) {
    fmt::print("OK\n");
  } else {
    fmt::print("BD: DONE\n");
  }
  return std::nullopt;
}

static std::optional<tsp::ErrorMeasure> tabu_search(bool verbose) noexcept {
  if (verbose) {
    fmt::print("---\nMeasuring Tabu Search\n");
  }

  std::optional<tsp::ErrorMeasure> err {std::nullopt};

  {
    std::array<tsp::Instance, 11 - 5 + 1> symmetric_rand {};
    for (int i {5}; i <= 11; ++i) {
      auto instance_ {
        config::read(fmt::format("./data/rand_tsp/configs/{}_rand_s.ini", i))};
      if (error::handle(instance_) == tsp::State::ERROR) {
        return tsp::ErrorMeasure::FILE_ERROR;
      }
      symmetric_rand.at(i - 5) = std::move(std::get<tsp::Instance>(instance_));
    }

    err = z3_measure_all<symmetric_rand.size()>(symmetric_rand.begin(),
                                                symmetric_rand.end(),
                                                6,
                                                30,
                                                2,
                                                500,
                                                50,
                                                500,
                                                25,
                                                3000,
                                                verbose,
                                                "./measure_ts_tabu_rs.csv",
                                                "./measure_ts_itr_rs.csv",
                                                "./measure_ts_n_rs.csv");
    if (err.has_value()) {
      return err;
    }
  }

  {
    std::array<tsp::Instance, 12 - 5 + 1> asymmetric_rand {};
    for (int i {5}; i <= 12; ++i) {
      auto instance_ {config::read(
      fmt::format("./data/rand_atsp/configs/{}_rand_as.ini", i))};
      if (error::handle(instance_) == tsp::State::ERROR) {
        return tsp::ErrorMeasure::FILE_ERROR;
      }
      asymmetric_rand.at(i - 5) = std::move(std::get<tsp::Instance>(instance_));
    }

    err = z3_measure_all<asymmetric_rand.size()>(asymmetric_rand.begin(),
                                                 asymmetric_rand.end(),
                                                 6,
                                                 30,
                                                 2,
                                                 500,
                                                 50,
                                                 500,
                                                 25,
                                                 3000,
                                                 verbose,
                                                 "./measure_ts_tabu_ra.csv",
                                                 "./measure_ts_itr_ra.csv",
                                                 "./measure_ts_n_ra.csv");
    if (err.has_value()) {
      return err;
    }
  }

  {
    const std::array configs {
      "./data/tsplib_tsp/configs/127_bier127.ini",
      "./data/tsplib_tsp/configs/159_u159.ini",
      "./data/tsplib_tsp/configs/180_brg180.ini",
      "./data/tsplib_tsp/configs/225_tsp225.ini",
      "./data/tsplib_tsp/configs/264_pr264.ini",
      "./data/tsplib_tsp/configs/299_pr299.ini",
      "./data/tsplib_tsp/configs/318_linhp318.ini",
    };

    std::array<tsp::Instance, configs.size()> tsplib_symmetric {};

    int i {0};
    for (const auto& config : configs) {
      auto instance_ {config::read(config)};
      if (error::handle(instance_) == tsp::State::ERROR) {
        return tsp::ErrorMeasure::FILE_ERROR;
      }
      tsplib_symmetric.at(i) = std::move(std::get<tsp::Instance>(instance_));
      ++i;
    }

    err = z3_measure_all<tsplib_symmetric.size()>(tsplib_symmetric.begin(),
                                                  tsplib_symmetric.end(),
                                                  10,
                                                  40,
                                                  2,
                                                  1000,
                                                  100,
                                                  1000,
                                                  100,
                                                  3000,
                                                  verbose,
                                                  "./measure_ts_tabu_libs.csv",
                                                  "./measure_ts_itr_libs.csv",
                                                  "./measure_ts_n_libs.csv");
    if (err.has_value()) {
      return err;
    }
  }

  {
    const std::array configs {
      "./data/tsplib_atsp/configs/34_ftv33.ini",
      "./data/tsplib_atsp/configs/43_p43.ini",
      "./data/tsplib_atsp/configs/53_ft53.ini",
      "./data/tsplib_atsp/configs/65_ftv64.ini",
      "./data/tsplib_atsp/configs/71_ftv70.ini",
      "./data/tsplib_atsp/configs/100_kro124p.ini",
    };

    std::array<tsp::Instance, configs.size()> tsplib_asymmetric {};

    int i {0};
    for (const auto& config : configs) {
      auto instance_ {config::read(config)};
      if (error::handle(instance_) == tsp::State::ERROR) {
        return tsp::ErrorMeasure::FILE_ERROR;
      }
      tsplib_asymmetric.at(i) = std::move(std::get<tsp::Instance>(instance_));
      ++i;
    }

    err = z3_measure_all<tsplib_asymmetric.size()>(tsplib_asymmetric.begin(),
                                                   tsplib_asymmetric.end(),
                                                   10,
                                                   40,
                                                   2,
                                                   1000,
                                                   100,
                                                   1000,
                                                   100,
                                                   3000,
                                                   verbose,
                                                   "./measure_ts_tabu_liba.csv",
                                                   "./measure_ts_itr_liba.csv",
                                                   "./measure_ts_n_liba.csv");
    if (err.has_value()) {
      return err;
    }
  }

  if (verbose) {
    fmt::print("OK\n");
  } else {
    fmt::print("TS: DONE\n");
  }
  return std::nullopt;
}

static std::optional<tsp::ErrorMeasure> genetic(bool verbose) noexcept {
  if (verbose) {
    fmt::print("---\nMeasuring Genetic\n");
  }

  std::optional<tsp::ErrorMeasure> err {std::nullopt};

  {
    std::array<tsp::Instance, 11 - 5 + 1> symmetric_rand {};
    for (int i {5}; i <= 11; ++i) {
      auto instance_ {
        config::read(fmt::format("./data/rand_tsp/configs/{}_rand_s.ini", i))};
      if (error::handle(instance_) == tsp::State::ERROR) {
        return tsp::ErrorMeasure::FILE_ERROR;
      }
      symmetric_rand.at(i - 5) = std::move(std::get<tsp::Instance>(instance_));
    }

    err = z4_measure_all<symmetric_rand.size()>(
    symmetric_rand.begin(),
    symmetric_rand.end(),
    5000,
    25,
    50,
    3,
    3,
    100,
    20,
    100,
    10,
    20,
    100,
    10,
    2,
    10,
    1,
    2,
    10,
    1,
    50,
    550,
    100,
    1000,
    1'0000,
    1000,
    verbose,
    "./measure_g_children_per_itr_rs.csv",
    "./measure_g_population_size_rs.csv",
    "./measure_g_max_children_per_pair_rs.csv",
    "./measure_g_max_v_count_crossover_rs.csv",
    "./measure_g_mutations_per_1000_rs.csv",
    "./measure_g_itr_rs.csv",
    "./measure_g_n_rs.csv");
    if (err.has_value()) {
      return err;
    }
  }

  {
    std::array<tsp::Instance, 12 - 5 + 1> asymmetric_rand {};
    for (int i {5}; i <= 12; ++i) {
      auto instance_ {config::read(
      fmt::format("./data/rand_atsp/configs/{}_rand_as.ini", i))};
      if (error::handle(instance_) == tsp::State::ERROR) {
        return tsp::ErrorMeasure::FILE_ERROR;
      }
      asymmetric_rand.at(i - 5) = std::move(std::get<tsp::Instance>(instance_));
    }

    err = z4_measure_all<asymmetric_rand.size()>(
    asymmetric_rand.begin(),
    asymmetric_rand.end(),
    5000,
    25,
    50,
    3,
    3,
    100,
    20,
    100,
    10,
    20,
    100,
    10,
    2,
    10,
    1,
    2,
    10,
    1,
    50,
    550,
    100,
    1000,
    1'0000,
    1000,
    verbose,
    "./measure_g_children_per_itr_ra.csv",
    "./measure_g_population_size_ra.csv",
    "./measure_g_max_children_per_pair_ra.csv",
    "./measure_g_max_v_count_crossover_ra.csv",
    "./measure_g_mutations_per_1000_ra.csv",
    "./measure_g_itr_ra.csv",
    "./measure_g_n_ra.csv");
    if (err.has_value()) {
      return err;
    }
  }

  {
    const std::array configs {
      "./data/tsplib_tsp/configs/127_bier127.ini",
      "./data/tsplib_tsp/configs/159_u159.ini",
      "./data/tsplib_tsp/configs/180_brg180.ini",
      "./data/tsplib_tsp/configs/225_tsp225.ini",
      "./data/tsplib_tsp/configs/264_pr264.ini",
      "./data/tsplib_tsp/configs/299_pr299.ini",
      "./data/tsplib_tsp/configs/318_linhp318.ini",
    };

    std::array<tsp::Instance, configs.size()> tsplib_symmetric {};

    int i {0};
    for (const auto& config : configs) {
      auto instance_ {config::read(config)};
      if (error::handle(instance_) == tsp::State::ERROR) {
        return tsp::ErrorMeasure::FILE_ERROR;
      }
      tsplib_symmetric.at(i) = std::move(std::get<tsp::Instance>(instance_));
      ++i;
    }

    err = z4_measure_all<tsplib_symmetric.size()>(
    tsplib_symmetric.begin(),
    tsplib_symmetric.end(),
    5000,
    30,
    60,
    3,
    20,
    100,
    100,
    600,
    100,
    20,
    100,
    10,
    2,
    10,
    1,
    10,
    60,
    10,
    100,
    600,
    100,
    1000,
    8000,
    1000,
    verbose,
    "./measure_g_children_per_itr_libs.csv",
    "./measure_g_population_size_libs.csv",
    "./measure_g_max_children_per_pair_libs.csv",
    "./measure_g_max_v_count_crossover_libs.csv",
    "./measure_g_mutations_per_1000_libs.csv",
    "./measure_g_itr_libs.csv",
    "./measure_g_n_libs.csv");
    if (err.has_value()) {
      return err;
    }
  }

  {
    const std::array configs {
      "./data/tsplib_atsp/configs/34_ftv33.ini",
      "./data/tsplib_atsp/configs/43_p43.ini",
      "./data/tsplib_atsp/configs/53_ft53.ini",
      "./data/tsplib_atsp/configs/65_ftv64.ini",
      "./data/tsplib_atsp/configs/71_ftv70.ini",
      "./data/tsplib_atsp/configs/100_kro124p.ini",
    };

    std::array<tsp::Instance, configs.size()> tsplib_asymmetric {};

    int i {0};
    for (const auto& config : configs) {
      auto instance_ {config::read(config)};
      if (error::handle(instance_) == tsp::State::ERROR) {
        return tsp::ErrorMeasure::FILE_ERROR;
      }
      tsplib_asymmetric.at(i) = std::move(std::get<tsp::Instance>(instance_));
      ++i;
    }

    err = z4_measure_all<tsplib_asymmetric.size()>(
    tsplib_asymmetric.begin(),
    tsplib_asymmetric.end(),
    5000,
    30,
    60,
    3,
    20,
    100,
    100,
    600,
    100,
    20,
    100,
    10,
    2,
    10,
    1,
    5,
    30,
    5,
    100,
    600,
    100,
    1000,
    8000,
    1000,
    verbose,
    "./measure_g_children_per_itr_libs.csv",
    "./measure_g_population_size_libs.csv",
    "./measure_g_max_children_per_pair_libs.csv",
    "./measure_g_max_v_count_crossover_libs.csv",
    "./measure_g_mutations_per_1000_libs.csv",
    "./measure_g_itr_libs.csv",
    "./measure_g_n_libs.csv");
    if (err.has_value()) {
      return err;
    }
  }

  if (verbose) {
    fmt::print("OK\n");
  } else {
    fmt::print("G: DONE\n");
  }
  return std::nullopt;
}

std::variant<std::monostate, tsp::ErrorMeasure> execute_measurements(
const tsp::MeasuringRun& run) noexcept {
  std::optional<tsp::ErrorMeasure> err {std::nullopt};

  for (const auto& algo : run.algorithms) {
    switch (algo) {
      case tsp::Algorithm::NEAREST_NEIGHBOUR:
        err = measure::nearest_neighbour(run.verbose);
        break;
      case tsp::Algorithm::BRUTE_FORCE:
        err = measure::brute_force(run.verbose);
        break;
      case tsp::Algorithm::RANDOM:
        err = measure::random(run.verbose);
        break;
      case tsp::Algorithm::BXB_LEAST_COST:
        err = measure::bxb_least_cost(run.verbose);
        break;
      case tsp::Algorithm::BXB_BFS:
        err = measure::bxb_bfs(run.verbose);
        break;
      case tsp::Algorithm::BXB_DFS:
        err = measure::bxb_dfs(run.verbose);
        break;
      case tsp::Algorithm::TABU_SEARCH:
        err = measure::tabu_search(run.verbose);
        break;
      case tsp::Algorithm::GENETIC:
        err = measure::genetic(run.verbose);
        break;
      default:
        break;
    }

    if (err.has_value()) {
      return *err;
    }
  }

  return std::monostate {};
}

}    // namespace util::measure
