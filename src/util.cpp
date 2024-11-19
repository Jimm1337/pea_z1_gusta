#include "util.hpp"

#if defined(ZADANIE1) && ZADANIE1 == 1
  #include "zadanie_1/bf.hpp"
  #include "zadanie_1/nn.hpp"
  #include "zadanie_1/random.hpp"
#endif

#if defined(ZADANIE2) && ZADANIE2 == 1
  #include "zadanie_2/bxb_bfs.hpp"
  #include "zadanie_2/bxb_dfs.hpp"
  #include "zadanie_2/bxb_lc.hpp"
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
  #include "zadanie_3/ts.hpp"
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
  #include "zadanie_4/gen.hpp"
#endif

#include <fmt/core.h>

#include <INIReader.h>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
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

#if defined(ZADANIE1) && ZADANIE1 == 1
  "[random]\n"
  "millis = <integer running time in ms>\n\n"
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
  "[tabu_search]\n"
  "itr = <integer max iterations>\n"
  "max_itr_no_improve = <integer iterations to halt with no improvement>\n"
  "tabu_itr = <integer iterations in tabu>\n\n"
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
  "[genetic]\n"
  "itr = <integer iterations>\n"
  "population_size = <integer population after selection>\n"
  "children_per_itr = <integer count of all children in itr>\n"
  "max_children_per_pair = <integer max children per pair>\n"
  "max_v_count_crossover = <integer max vertices to crossover>\n"
  "mutations_per_1000 = <integer ppt of chromosomes to mutate>\n\n"
#endif

  "- Example:\n\n"
  "[instance]\n"
  "input_path = C:/dev/pea_z1_gusta/data/test_6_as.txt\n"
  "symmetric = false\n"
  "full = true\n\n"
  "[optimal]\n"
  "path = 2 3 0 1 4 5\n"
  "cost = 150\n\n"

#if defined(ZADANIE1) && ZADANIE1 == 1
  "[random]\n"
  "millis = 1000\n\n"
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
  "[tabu_search]\n"
  "itr = 10000\n"
  "max_itr_no_improve = 50\n"
  "tabu_itr = 10\n\n"
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
  "[genetic]\n"
  "itr = 100000\n"
  "population_size = 50\n"
  "count_of_children = 50\n"
  "max_children_per_pair = 4\n"
  "max_v_count_crossover = 5\n"
  "mutations_per_1000 = 5\n\n"
#endif
  );
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
#if defined(ZADANIE1) && ZADANIE1 == 1
    .random = {.millis =
               static_cast<int>(reader.GetInteger("random", "millis", -1))},
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
    .tabu_search = {.itr = static_cast<int>(
                    reader.GetInteger("tabu_search", "itr", -1)),
               .max_itr_no_improve = static_cast<int>(
                    reader.GetInteger("tabu_search", "max_itr_no_improve", -1)),
               .tabu_itr = static_cast<int>(
                    reader.GetInteger("tabu_search", "tabu_itr", -1))},
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
    .genetic = {.itr =
                static_cast<int>(reader.GetInteger("genetic", "itr", -1)),
               .population_size = static_cast<int>(
                reader.GetInteger("genetic", "population_size", -1)),
               .children_per_itr = static_cast<int>(
                reader.GetInteger("genetic", "children_per_itr", -1)),
               .max_children_per_pair = static_cast<int>(
                reader.GetInteger("genetic", "max_children_per_pair", -1)),
               .max_v_count_crossover = static_cast<int>(
                reader.GetInteger("genetic", "max_v_count_crossover", -1)),
               .mutations_per_1000 = static_cast<int>(
                reader.GetInteger("genetic", "mutations_per_1000", -1))}
#endif
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
#if defined(ZADANIE1) && ZADANIE1 == 1
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
#endif

#if defined(ZADANIE2) && ZADANIE2 == 1
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
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
    case tsp::Algorithm::TABU_SEARCH:
      fmt::println("Algorithm (Tabu Search)");
      fmt::println("- Count of iterations: {}", params.tabu_search.itr);
      fmt::println("- Max iterations with no improvement: {}",
                   params.tabu_search.max_itr_no_improve);
      fmt::println("- Count of iterations in tabu: {}\n",
                   params.tabu_search.tabu_itr);
      break;
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
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
#endif

    default:
      fmt::println("[E] Something went wrong\n");
      break;
  }

  fmt::println("-- RESULTS --");
  fmt::println("Time: {:.2f} {}\n", count, unit);
  fmt::println("Cost: {}", solution.cost);
  if (matrix.size() <= 16) {
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

#if defined(ZADANIE1) && ZADANIE1 == 1
  " -bf: Use Brute Force algorithm\n"
  " -nn: Use Nearest Neighbour algorithm\n"
  " -r : Use Random algorithm\n"
#endif

#if defined(ZADANIE2) && ZADANIE2 == 1
  " -lc: Use Branch and Bound Least Cost algorithm\n"
  " -bb: Use Branch and Bound BFS algorithm\n"
  " -bd: Use Branch and Bound DFS algorithm\n"
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
  " -ts: Use Tabu Search algorithm\n"
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
  " -g : Use Genetic algorithm\n"
#endif

  "\nExample:\n"

#if defined(ZADANIE1) && ZADANIE1 == 1
  "pea_z1_gusta --config=C:/dev/pea_z1_gusta/configs/test_6.ini -r\n"
#endif

#if defined(ZADANIE2) && ZADANIE2 == 1
  "pea_z1_gusta --config=C:/dev/pea_z1_gusta/configs/test_6.ini -lc\n"
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
  "pea_z1_gusta --config=C:/dev/pea_z1_gusta/configs/test_6.ini -ts\n"
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
  "pea_z1_gusta --config=C:/dev/pea_z1_gusta/configs/test_6.ini -g\n"
#endif

  "pea_z1_gusta --measure --verbose "

#if defined(ZADANIE1) && ZADANIE1 == 1
  "-bf -nn -r "
#endif

#if defined(ZADANIE2) && ZADANIE2 == 1
  "-lc -bb -bd "
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
  "-ts "
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
  "-g "
#endif

  "\n");
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

#if defined(ZADANIE1) && ZADANIE1 == 1
  const bool algo_nn {std::ranges::find(arg_vec, "-nn") != arg_vec.end()};
  const bool algo_bf {std::ranges::find(arg_vec, "-bf") != arg_vec.end()};
  const bool algo_random {std::ranges::find(arg_vec, "-r") != arg_vec.end()};
#endif

#if defined(ZADANIE2) && ZADANIE2 == 1
  const bool algo_bxblc {std::ranges::find(arg_vec, "-lc") != arg_vec.end()};
  const bool algo_bxbbfs {std::ranges::find(arg_vec, "-bb") != arg_vec.end()};
  const bool algo_bxbdfs {std::ranges::find(arg_vec, "-bd") != arg_vec.end()};
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
  const bool algo_ts {std::ranges::find(arg_vec, "-ts") != arg_vec.end()};
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
  const bool algo_gen {std::ranges::find(arg_vec, "-g") != arg_vec.end()};
#endif

  // do measuring run
  if (std::ranges::find(arg_vec, "--measure") != arg_vec.end()) {
    tsp::MeasuringRun run {};
    run.algorithms.fill(tsp::Algorithm::INVALID);

    if (std::ranges::find(arg_vec, "--verbose") != arg_vec.end()) {
      run.verbose = true;
    }

#if defined(ZADANIE1) && ZADANIE1 == 1
    if (algo_nn) {
      run.algorithms.at(0) = tsp::Algorithm::NEAREST_NEIGHBOUR;
    }

    if (algo_bf) {
      run.algorithms.at(1) = tsp::Algorithm::BRUTE_FORCE;
    }

    if (algo_random) {
      run.algorithms.at(2) = tsp::Algorithm::RANDOM;
    }
#endif

#if defined(ZADANIE2) && ZADANIE2 == 1
    if (algo_bxblc) {
      run.algorithms.at(3) = tsp::Algorithm::BXB_LEAST_COST;
    }

    if (algo_bxbbfs) {
      run.algorithms.at(4) = tsp::Algorithm::BXB_BFS;
    }

    if (algo_bxbdfs) {
      run.algorithms.at(5) = tsp::Algorithm::BXB_DFS;
    }
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
    if (algo_ts) {
      run.algorithms.at(6) = tsp::Algorithm::TABU_SEARCH;
    }
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
    if (algo_gen) {
      run.algorithms.at(7) = tsp::Algorithm::GENETIC;
    }
#endif

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

  const int algo_count {[
#if defined(ZADANIE1) && ZADANIE1 == 1
                        &algo_bf,
                        &algo_random,
                        &algo_nn,
#endif

#if defined(ZADANIE2) && ZADANIE2 == 1
                        &algo_bxblc,
                        &algo_bxbbfs,
                        &algo_bxbdfs,
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
                        &algo_ts,
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
                        &algo_gen,
#endif
                        count {0}]() mutable noexcept {
#if defined(ZADANIE1) && ZADANIE1 == 1
    if (algo_bf) {
      ++count;
    }
    if (algo_nn) {
      ++count;
    }
    if (algo_random) {
      ++count;
    }
#endif

#if defined(ZADANIE2) && ZADANIE2 == 1
    if (algo_bxblc) {
      ++count;
    }
    if (algo_bxbbfs) {
      ++count;
    }
    if (algo_bxbdfs) {
      ++count;
    }
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
    if (algo_ts) {
      ++count;
    }
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
    if (algo_gen) {
      ++count;
    }
#endif

    return count;
  }()};
  if (algo_count > 1) [[unlikely]] {
    return tsp::ErrorArg::MULTIPLE_ARG;
  }
  if (algo_count == 0) [[unlikely]] {
    return tsp::ErrorArg::NO_ARG;
  }

#if defined(ZADANIE1) && ZADANIE1 == 1
  if (algo_nn) {
    return tsp::SingleRun {
      .algorithm   = tsp::Algorithm::NEAREST_NEIGHBOUR,
      .config_file = std::filesystem::absolute(config_path)};
  }

  if (algo_bf) {
    return tsp::SingleRun {
      .algorithm   = tsp::Algorithm::BRUTE_FORCE,
      .config_file = std::filesystem::absolute(config_path)};
  }

  if (algo_random) {
    return tsp::SingleRun {
      .algorithm   = tsp::Algorithm::RANDOM,
      .config_file = std::filesystem::absolute(config_path)};
  }
#endif

#if defined(ZADANIE2) && ZADANIE2 == 1
  if (algo_bxblc) {
    return tsp::SingleRun {
      .algorithm   = tsp::Algorithm::BXB_LEAST_COST,
      .config_file = std::filesystem::absolute(config_path)};
  }

  if (algo_bxbbfs) {
    return tsp::SingleRun {
      .algorithm   = tsp::Algorithm::BXB_BFS,
      .config_file = std::filesystem::absolute(config_path)};
  }

  if (algo_bxbdfs) {
    return tsp::SingleRun {
      .algorithm   = tsp::Algorithm::BXB_DFS,
      .config_file = std::filesystem::absolute(config_path)};
  }
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
  if (algo_ts) {
    return tsp::SingleRun {
      .algorithm   = tsp::Algorithm::TABU_SEARCH,
      .config_file = std::filesystem::absolute(config_path)};
  }
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
  if (algo_gen) {
    return tsp::SingleRun {
      .algorithm   = tsp::Algorithm::GENETIC,
      .config_file = std::filesystem::absolute(config_path)};
  }
#endif

  return tsp::SingleRun {.algorithm   = tsp::Algorithm::INVALID,
                         .config_file = std::filesystem::absolute(config_path)};
}

}    // namespace util::arg

