#include "gen.hpp"

#include "nn.hpp"

#include <list>
#include <numeric>
#include <random>
#include <set>
#include <stack>

//todo: implement

namespace gen::impl {
struct WorkingSolution {
  std::vector<size_t> v_indices;
  tsp::Solution       solution;

  [[nodiscard]] constexpr std::weak_ordering operator<=>(
  const WorkingSolution& other) const noexcept {
    const auto cmp{solution.cost <=> other.solution.cost};
    if (cmp != std::strong_ordering::equal) {
      return cmp;
    }

    if (solution.path.size() != other.solution.path.size()) {
      return std::weak_ordering::equivalent;
    }

    for (int v {0}; v < solution.path.size(); ++v) {
      if (solution.path.at(v) < other.solution.path.at(v)) {
        return std::weak_ordering::less;
      }
      if (solution.path.at(v) > other.solution.path.at(v)) {
        return std::weak_ordering::greater;
      }
    }
    return std::weak_ordering::equivalent;
  }
};

using Population = std::set<WorkingSolution, std::greater<>>;

static std::variant<WorkingSolution, tsp::ErrorAlgorithm> get_first_solution(
const tsp::Matrix<int>& matrix,
const tsp::GraphInfo&   graph_info) noexcept {
  const size_t v_count {matrix.size()};

  const auto upper_bound_result {nn::run(matrix, graph_info)};
  if (std::holds_alternative<tsp::ErrorAlgorithm>(upper_bound_result)) {
    return std::get<tsp::ErrorAlgorithm>(upper_bound_result);
  }
  tsp::Solution upper_bound {std::get<tsp::Solution>(upper_bound_result)};
  std::vector   indices {
    std::vector(v_count, std::numeric_limits<size_t>::max())};
  for (size_t v {0}; v < upper_bound.path.size() - 1; ++v) {
    indices.at(upper_bound.path.at(v)) = v;
  }

  return WorkingSolution {.v_indices = std::move(indices),
                          .solution  = std::move(upper_bound)};
}

constexpr static bool perform_swap(const tsp::Matrix<int>& matrix,
                                   WorkingSolution&        solution,
                                   int                     first_v,
                                   int                     second_v) noexcept {
  if (first_v == second_v) {
    return true;
  }

  const size_t first_v_idx {solution.v_indices.at(first_v)};
  const int    first_prev {
    first_v_idx != 0
       ? solution.solution.path.at(first_v_idx - 1)
       : solution.solution.path.at(solution.solution.path.size() - 2)};
  const int first_next {solution.solution.path.at(first_v_idx + 1)};

  const size_t second_v_idx {solution.v_indices.at(second_v)};
  const int    second_prev {solution.solution.path.at(second_v_idx - 1)};
  const int    second_next {solution.solution.path.at(second_v_idx + 1)};

  const int first_prev_old_cost {matrix.at(first_prev).at(first_v)};
  const int first_next_old_cost {matrix.at(first_v).at(first_next)};
  const int second_prev_old_cost {matrix.at(second_prev).at(second_v)};
  const int second_next_old_cost {matrix.at(second_v).at(second_next)};

  const int first_prev_new_cost {first_prev != second_v
                                 ? matrix.at(first_prev).at(second_v)
                                 : matrix.at(first_v).at(second_v)};
  const int first_next_new_cost {second_v != first_next
                                 ? matrix.at(second_v).at(first_next)
                                 : first_next_old_cost};
  const int second_prev_new_cost {second_prev != first_v
                                  ? matrix.at(second_prev).at(first_v)
                                  : matrix.at(second_v).at(first_v)};
  const int second_next_new_cost {first_v != second_next
                                  ? matrix.at(first_v).at(second_next)
                                  : second_next_old_cost};

  if (first_prev_new_cost == -1 || first_next_new_cost == -1 ||
      second_prev_new_cost == -1 || second_next_new_cost == -1) [[unlikely]] {
    return false;
  }

  const int new_cost {first_prev_new_cost + first_next_new_cost +
                      second_prev_new_cost + second_next_new_cost};

  const int old_cost {first_prev_old_cost + first_next_old_cost +
                      second_prev_old_cost + second_next_old_cost};

  std::swap(solution.solution.path.at(first_v_idx),
            solution.solution.path.at(second_v_idx));
  std::swap(solution.v_indices.at(first_v), solution.v_indices.at(second_v));

  if (first_v_idx == 0) {
    solution.solution.path.back() = second_v;
  }

  solution.solution.cost += new_cost - old_cost;
  return true;
}

[[nodiscard]] static std::variant<Population, tsp::ErrorAlgorithm>
init_population(const tsp::Matrix<int>& matrix,
                const tsp::GraphInfo&   graph_info,
                std::mt19937_64&        rand_src,
                int                     population_size) noexcept {
  const size_t v_count {matrix.size()};
  const int    max_retries {static_cast<int>(v_count) * population_size * 100};
  int          retries {0};

  const auto first_sol_result {get_first_solution(matrix, graph_info)};
  if (std::holds_alternative<tsp::ErrorAlgorithm>(first_sol_result)) {
    return std::get<tsp::ErrorAlgorithm>(first_sol_result);
  }

  Population new_population {};

  std::uniform_int_distribution distribution {};

  while (new_population.size() != population_size) {
    WorkingSolution next {std::get<WorkingSolution>(first_sol_result)};

    for (int i {
           static_cast<int>(next.v_indices.end() - next.v_indices.begin() - 1)};
         i > 0;
         --i) {
      while (!perform_swap(
      matrix,
      next,
      next.solution.path.at(
      *(next.v_indices.begin() +
        distribution(rand_src,
                     std::uniform_int_distribution<>::param_type(0, i)))),
      next.solution.path.at(*(next.v_indices.begin() + i)))) {
        if (++retries > max_retries) [[unlikely]] {
          return tsp::ErrorAlgorithm::INVALID_PARAM;
        }
      }
    }

    new_population.emplace(std::move(next));
  }

  return new_population;
}

static void mutate(const tsp::Matrix<int>& matrix,
                   Population&             population,
                   std::mt19937_64&        rand_src,
                   int                     mutations_per_1000) noexcept {
  const size_t v_count {matrix.size()};
  const int    max_retries {static_cast<int>(v_count * 10)};

  std::uniform_int_distribution<> chance_distribution {0, 1000};
  std::uniform_int_distribution<> swap_distribution {};

  for (int i {0}; i < population.size(); ++i) {
    if (chance_distribution(rand_src) < mutations_per_1000) {
      int retries {0};
      int second_v {
        swap_distribution(rand_src,
                          std::uniform_int_distribution<>::param_type {
                            1,
                            static_cast<int>(v_count - 1)})};
      int first_v {swap_distribution(
      rand_src,
      std::uniform_int_distribution<>::param_type {0, second_v - 1})};

      const WorkingSolution old_node {
        population
        .extract(
        std::ranges::next(population.begin(), i, std::prev(population.end())))
        .value()};
      WorkingSolution node {old_node};

      while (!perform_swap(matrix, node, first_v, second_v)) [[unlikely]] {
        if (++retries > max_retries) [[unlikely]] {
          break;
        }

        second_v =
        swap_distribution(rand_src,
                          std::uniform_int_distribution<>::param_type {
                            1,
                            static_cast<int>(v_count - 1)});
        first_v = swap_distribution(
        rand_src,
        std::uniform_int_distribution<>::param_type {0, second_v - 1});
      }

      if (population.contains(node)) [[unlikely]] {
        population.emplace(old_node);
      } else {
        population.emplace(node);
      }
    }
  }
}

// todo: selection func
template<typename PopulationType>
requires std::is_same_v<std::remove_cvref_t<PopulationType>, Population>
static Population select_and_reproduce(tsp::Matrix<int>& matrix,
                                       PopulationType&&  pop,
                                       std::mt19937_64&  rand_src,
                                       int               count_of_children,
                                       int crossovers_per_100) noexcept {
  Population   population {std::forward<PopulationType>(population)};
  const size_t population_size {population.size()};
  const size_t max_parents {population_size / count_of_children};

  //todo: cut a up to max parents, reproduce, cut up to pop size
}

// todo: crossover (use 1-point then 2-point then ... (percent chance per child))

// todo: evaluation (discard invalid solutions)

}    // namespace gen::impl

namespace gen {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix,
const tsp::GraphInfo&   graph_info,
int                     count_of_itr,
int                     population_size,
int                     count_of_children,
int                     crossovers_per_100,
int                     mutations_per_1000) noexcept {
  return tsp::ErrorAlgorithm::NO_PATH;
}

}    // namespace gen
