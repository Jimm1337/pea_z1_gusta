#include "util.hpp"
#include "measure.hpp"

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

#include <cstdlib>
#include <optional>
#include <variant>
#include <windows.h>

#pragma comment(lib, "Kernel32.lib")

int main(int argc, const char** argv) {
  const auto arg_result {util::arg::read(argc, argv)};
  if (util::error::handle(arg_result) == tsp::State::ERROR) {
    return EXIT_FAILURE;
  }
  const tsp::Arguments arg {std::get<tsp::Arguments>(arg_result)};

  // set priority for process and thread for more consistent results.
  if (SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS) == 0 ||
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST) == 0) {
    fmt::println(
    "[W] Could not set task priority to HIGH. Timing may be inconsistent.");
  }

  // measuring run
  if (std::holds_alternative<tsp::MeasuringRun>(arg)) {
    if (util::error::handle(util::measure::execute_measurements(
        std::get<tsp::MeasuringRun>(arg))) == tsp::State::ERROR) {
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }

  // single run
  const auto config_result {
    util::config::read(std::get<tsp::SingleRun>(arg).config_file)};
  if (util::error::handle(config_result) == tsp::State::ERROR) {
    return EXIT_FAILURE;
  }
  const tsp::Instance config {std::get<tsp::Instance>(config_result)};

  const std::optional optimal_cost {config.optimal.cost != -1
                                    ? std::optional {config.optimal.cost}
                                    : std::nullopt};

  const auto timed_result {[&arg, &config, &optimal_cost]() noexcept {
    switch (std::get<tsp::SingleRun>(arg).algorithm) {
#if defined(ZADANIE1) && ZADANIE1 == 1
      case tsp::Algorithm::BRUTE_FORCE:
        return util::measured_run(bf::run,
                                  config.matrix,
                                  config.graph_info,
                                  optimal_cost);
      case tsp::Algorithm::NEAREST_NEIGHBOUR:
        return util::measured_run(nn::run,
                                  config.matrix,
                                  config.graph_info,
                                  optimal_cost);
      case tsp::Algorithm::RANDOM:
        return util::measured_run(random::run,
                                  config.matrix,
                                  config.graph_info,
                                  optimal_cost,
                                  config.params.random.millis);
#endif

#if defined(ZADANIE2) && ZADANIE2 == 1
      case tsp::Algorithm::BXB_LEAST_COST:
        return util::measured_run(bxb::lc::run,
                                  config.matrix,
                                  config.graph_info,
                                  optimal_cost);
      case tsp::Algorithm::BXB_BFS:
        return util::measured_run(bxb::bfs::run,
                                  config.matrix,
                                  config.graph_info,
                                  optimal_cost);
      case tsp::Algorithm::BXB_DFS:
        return util::measured_run(bxb::dfs::run,
                                  config.matrix,
                                  config.graph_info,
                                  optimal_cost);
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
      case tsp::Algorithm::TABU_SEARCH:
        return util::measured_run(ts::run,
                                  config.matrix,
                                  config.graph_info,
                                  optimal_cost,
                                  config.params.tabu_search.itr,
                                  config.params.tabu_search.max_itr_no_improve,
                                  config.params.tabu_search.tabu_itr);
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
      case tsp::Algorithm::GENETIC:
        return util::measured_run(gen::run,
                                  config.matrix,
                                  config.graph_info,
                                  optimal_cost,
                                  config.params.genetic.itr,
                                  config.params.genetic.population_size,
                                  config.params.genetic.children_per_itr,
                                  config.params.genetic.max_children_per_pair,
                                  config.params.genetic.max_v_count_crossover,
                                  config.params.genetic.mutations_per_1000);
#endif
      default:
        std::exit(1);
    }
  }()};
  if (util::error::handle(timed_result) == tsp::State::ERROR) {
    return EXIT_FAILURE;
  }
  const tsp::Result timed {std::get<tsp::Result>(timed_result)};

  util::output::report(std::get<tsp::SingleRun>(arg), config, timed);

  return EXIT_SUCCESS;
}
