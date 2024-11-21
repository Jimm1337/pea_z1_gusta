#include "measure.hpp"

#if (defined(ZADANIE1) && ZADANIE1 == 1)
  #include "zadanie_1/bf.hpp"
  #include "zadanie_1/nn.hpp"
  #include "zadanie_1/random.hpp"
#endif

#if (defined(ZADANIE2) && ZADANIE2 == 1)
  #include "zadanie_2/bxb_bfs.hpp"
  #include "zadanie_2/bxb_dfs.hpp"
  #include "zadanie_2/bxb_lc.hpp"
#endif

#if (defined(ZADANIE3) && ZADANIE3 == 1)
  #include "zadanie_3/ts.hpp"
#endif

#if (defined(ZADANIE4) && ZADANIE4 == 1)
  #include "zadanie_4/gen.hpp"
#endif

#include <fstream>

namespace util::measure {

#if (defined(ZADANIE1) && ZADANIE1 == 1) || (defined(ZADANIE2) && ZADANIE2 == 1)
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

  file_s
  << "Ilosc wierzcholkow;Nazwa;Koszt optymalny;Koszt obliczony;Czas [us]\n";

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
    const int         v_count {static_cast<int>(instance.matrix.size())};
    for (const tsp::Result& run : runs) {
      const int         cost {run.solution.cost};
      const std::string time_us {
        fmt::format("{:.2f}", run.time.count() * 1000.)};

      file_s << fmt::format("{};{};{};{};{}\n",
                            v_count,
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

  file_as
  << "Ilosc wierzcholkow;Nazwa;Koszt optymalny;Koszt obliczony;Czas [us]\n";

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
    const int         v_count {static_cast<int>(instance.matrix.size())};
    for (const tsp::Result& run : runs) {
      const int         cost {run.solution.cost};
      const std::string time_us {
        fmt::format("{:.2f}", run.time.count() * 1000.)};

      file_as << fmt::format("{};{};{};{};{}\n",
                             v_count,
                             instance_name,
                             optimal_cost,
                             cost,
                             time_us);
    }
  }

  return std::nullopt;
}
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
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
          fmt::format("{:.2f}", run.time.count() * 1000.)};

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
          fmt::format("{:.2f}", run.time.count() * 1000.)};

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
        fmt::format("{:.2f}", run.time.count() * 1000.)};

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
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
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
          fmt::format("{:.2f}", run.time.count() * 1000.)};
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
          fmt::format("{:.2f}", run.time.count() * 1000.)};
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
          fmt::format("{:.2f}", run.time.count() * 1000.)};
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
          fmt::format("{:.2f}", run.time.count() * 1000.)};
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
          fmt::format("{:.2f}", run.time.count() * 1000.)};
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
          fmt::format("{:.2f}", run.time.count() * 1000.)};
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
        fmt::format("{:.2f}", run.time.count() * 1000.)};
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
#endif

#if defined(ZADANIE1) && ZADANIE1 == 1
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
#endif

#if defined(ZADANIE2) && ZADANIE2 == 1
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
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
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
      "./data/tsplib_tsp/configs/225_tsp225.ini",
      "./data/tsplib_tsp/configs/264_pr264.ini",
      "./data/tsplib_tsp/configs/318_lin318.ini",
      "./data/tsplib_tsp/configs/431_gr431.ini",
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
      "./data/tsplib_atsp/configs/48_ry48p.ini",
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
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
static std::optional<tsp::ErrorMeasure> genetic(bool verbose) noexcept {
  if (verbose) {
    fmt::print("---\nMeasuring Genetic\n");
  }

  std::optional<tsp::ErrorMeasure> err {std::nullopt};

  {
    std::array<tsp::Instance, 11 - 7 + 1> symmetric_rand {};
    for (int i {7}; i <= 11; ++i) {
      auto instance_ {
        config::read(fmt::format("./data/rand_tsp/configs/{}_rand_s.ini", i))};
      if (error::handle(instance_) == tsp::State::ERROR) {
        return tsp::ErrorMeasure::FILE_ERROR;
      }
      symmetric_rand.at(i - 7) = std::move(std::get<tsp::Instance>(instance_));
    }

    err = z4_measure_all<symmetric_rand.size()>(
    symmetric_rand.begin(),
    symmetric_rand.end(),
    5000,
    10,
    20,
    3,
    3,
    100,
    5,
    25,
    5,
    5,
    10,
    1,
    2,
    8,
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
    std::array<tsp::Instance, 12 - 7 + 1> asymmetric_rand {};
    for (int i {7}; i <= 12; ++i) {
      auto instance_ {config::read(
      fmt::format("./data/rand_atsp/configs/{}_rand_as.ini", i))};
      if (error::handle(instance_) == tsp::State::ERROR) {
        return tsp::ErrorMeasure::FILE_ERROR;
      }
      asymmetric_rand.at(i - 7) = std::move(std::get<tsp::Instance>(instance_));
    }

    err = z4_measure_all<asymmetric_rand.size()>(
    asymmetric_rand.begin(),
    asymmetric_rand.end(),
    5000,
    10,
    20,
    3,
    3,
    100,
    5,
    25,
    5,
    5,
    10,
    1,
    2,
    8,
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
      "./data/tsplib_tsp/configs/225_tsp225.ini",
      "./data/tsplib_tsp/configs/264_pr264.ini",
      "./data/tsplib_tsp/configs/318_lin318.ini",
      "./data/tsplib_tsp/configs/431_gr431.ini",
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
      "./data/tsplib_atsp/configs/48_ry48p.ini",
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
    "./measure_g_children_per_itr_liba.csv",
    "./measure_g_population_size_liba.csv",
    "./measure_g_max_children_per_pair_liba.csv",
    "./measure_g_max_v_count_crossover_liba.csv",
    "./measure_g_mutations_per_1000_liba.csv",
    "./measure_g_itr_liba.csv",
    "./measure_g_n_liba.csv");
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
#endif

std::variant<std::monostate, tsp::ErrorMeasure> execute_measurements(
const tsp::MeasuringRun& run) noexcept {
  std::optional<tsp::ErrorMeasure> err {std::nullopt};

  for (const auto& algo : run.algorithms) {
    switch (algo) {
#if defined(ZADANIE1) && ZADANIE1 == 1
      case tsp::Algorithm::NEAREST_NEIGHBOUR:
        err = measure::nearest_neighbour(run.verbose);
        break;
      case tsp::Algorithm::BRUTE_FORCE:
        err = measure::brute_force(run.verbose);
        break;
      case tsp::Algorithm::RANDOM:
        err = measure::random(run.verbose);
        break;
#endif

#if defined(ZADANIE2) && ZADANIE2 == 1
      case tsp::Algorithm::BXB_LEAST_COST:
        err = measure::bxb_least_cost(run.verbose);
        break;
      case tsp::Algorithm::BXB_BFS:
        err = measure::bxb_bfs(run.verbose);
        break;
      case tsp::Algorithm::BXB_DFS:
        err = measure::bxb_dfs(run.verbose);
        break;
#endif

#if defined(ZADANIE3) && ZADANIE3 == 1
      case tsp::Algorithm::TABU_SEARCH:
        err = measure::tabu_search(run.verbose);
        break;
#endif

#if defined(ZADANIE4) && ZADANIE4 == 1
      case tsp::Algorithm::GENETIC:
        err = measure::genetic(run.verbose);
        break;
#endif

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
