#include "gen.hpp"

#include "nn.hpp"

#include <list>
#include <numeric>
#include <random>
#include <set>
#include <stack>

//todo: implement

namespace gen::impl {

struct Chromosome {
  std::vector<int> vertices;
  std::vector<int> path;
  int              cost;

  constexpr std::weak_ordering operator<=>(
  const Chromosome& other) const noexcept {
    if (const auto result = cost <=> other.cost;
        result != std::strong_ordering::equal) {
      return result;
    }

    if (const auto result = path.at(1) <=> other.path.at(1);
        result != std::strong_ordering::equal) {
      return result;
    }

    return path.at(path.size() / 2) <=> other.path.at(other.path.size() / 2);
  }
};

using Population = std::set<Chromosome>;

[[nodiscard]] static std::variant<Population, tsp::ErrorAlgorithm>
init_population(const tsp::Matrix<int>&   matrix,
                const tsp::GraphInfo&     graph_info,
                const std::optional<int>& optimal_cost,
                auto&                     rand_src,
                int                       population_count) noexcept {
  const int v_count {static_cast<int>(matrix.size())};

  const auto nn_result {nn::run(matrix, graph_info, optimal_cost)};
  if (std::holds_alternative<tsp::ErrorAlgorithm>(nn_result)) [[unlikely]] {
    return std::get<tsp::ErrorAlgorithm>(nn_result);
  }

  const Chromosome nn_chromosome {[&nn_solution = std::get<tsp::Solution>(nn_result),
                             &v_count]() noexcept {
    std::vector vertices(v_count - 1, -1);
    for (int i {0}; i < v_count - 1; ++i) {
      vertices.at(nn_solution.path.at(i)) = i;
    }
    return {std::move(vertices), std::move(nn_solution.path), nn_solution.cost};
  }()};

  Population population {{nn_chromosome}};

  const std::uniform_int_distribution dist {1, v_count - 2};
  while (population.size() != population_count) [[likely]] {
    const int first_idx {dist(rand_src)};
    const int second_idx {dist(rand_src)};

    const auto first_v {nn_chromosome.path.at(first_v)};
    const auto second_v {nn_chromosome.path.at(second_v)};

    const int first_new_cost_left {matrix.at(first_v).at(second_v - 1)};
    const int first_new_cost_right {matrix.at(first_v).at(second_v + 1)};
    const int second_new_cost_left {matrix.at(second_v).at(first_v - 1)};
    const int second_new_cost_right {matrix.at(second_v).at(first_v + 1)};

    if (first_new_cost_left == -1 || first_new_cost_right == -1 ||
        second_new_cost_left == -1 || second_new_cost_right == -1)
    [[unlikely]] {
      continue;
    }

    const int first_old_cost {matrix.at(first_v).at(first_v - 1) +
                              matrix.at(first_v).at(first_v + 1)};
    const int second_old_cost {matrix.at(second_v).at(second_v - 1) +
                               matrix.at(second_v).at(second_v + 1)};

    const int cost_diff {first_new_cost_left + first_new_cost_right +
                         second_new_cost_left + second_new_cost_right -
                         first_old_cost - second_old_cost};

    population.emplace([&nn_chromosome,
                        &first_v,
                        &first_idx,
                        &second_v,
                        &second_idx,
                        &cost_diff]() noexcept {
      Chromosome chromosome {.vertices = nn_chromosome.vertices,
                             .path     = nn_chromosome.path,
                             .cost     = nn_chromosome.cost + cost_diff};

      std::swap(chromosome.path.at(first_idx),
                chromosome.path.at(second_idx));
      std::swap(chromosome.vertices.at(first_v),
                chromosome.vertices.at(second_v));

      return chromosome;
    }());
  }

  return population;
}

[[nodiscard]] static std::optional<Chromosome> crossover(
const tsp::Matrix<int>& matrix,
auto&                   rand_src,
const Chromosome&       parent_base,
const Chromosome&       parent2,
int                     max_v_count_crossover) noexcept {
  const int v_count {static_cast<int>(matrix.size())};

  Chromosome child {.vertices = parent_base.vertices,
                    .path     = parent_base.path,
                    .cost     = parent_base.cost};

  const std::uniform_int_distribution dist {};

  const int  first_idx {dist(rand_src, {1, v_count - 2})};
  const auto first_v {child.path.at(first_v)};

  const auto second_idx {parent2.vertices.at(first_v)};

  const auto count {
    dist(rand_src,
         {1,
          std::min(std::min(v_count - 1 - second_idx, v_count - 1 - first_idx),
                   max_v_count_crossover)})};

  for (int i {1}; i < count; ++i) {
    const int base_idx {first_idx + i};
    const int other_idx {second_idx + i};

    const int base_v {child.path.at(base_idx)};
    const int other_v {parent2.path.at(other_idx)};

    if (base_v == other_v) [[unlikely]] {
      continue;
    }

    if (child.vertices.at(other_v) < first_idx ||
        child.vertices.at(other_v) >= first_idx + count) {
      return std::nullopt;
    }

    const int new_cost_left {
      matrix.at(child.path.at(base_idx - 1)).at(other_v)};
    const int new_cost_right {
      matrix.at(other_v).at(child.path.at(base_idx + 1))};

    if (new_cost_left == -1 || new_cost_right == -1) [[unlikely]] {
      return std::nullopt;
    }

    const int old_cost {matrix.at(child.path.at(base_idx - 1)).at(base_v) +
                        matrix.at(base_v).at(child.path.at(base_idx + 1))};

    const int cost_diff {new_cost_left + new_cost_right - old_cost};

    child.path.at(base_idx)     = other_v;
    child.vertices.at(other_v)  = base_idx;
    child.cost                 += cost_diff;
  }

  return child;
}

static bool mutate(tsp::Matrix<int>& matrix,
                   auto&             rand_src,
                   Chromosome&       chromosome) noexcept {
  const int v_count {static_cast<int>(matrix.size())};

  const std::uniform_int_distribution dist {1, v_count - 2};

  const int swap_idx {dist(rand_src)};
  const int swap_v {chromosome.path.at(swap_idx)};
  const int first_v {chromosome.path.at(0)};

  const int new_cost_left_s {
    matrix.at(chromosome.path.at(v_count - 1)).at(swap_v)};
  const int new_cost_right_s {matrix.at(swap_v).at(chromosome.path.at(1))};

  const int new_cost_left_in {
    matrix.at(chromosome.path.at(swap_idx - 1)).at(first_v)};
  const int new_cost_right_in {
    matrix.at(first_v).at(chromosome.path.at(swap_idx + 1))};

  if (new_cost_left_s == -1 || new_cost_right_s == -1 ||
      new_cost_left_in == -1 || new_cost_right_in == -1) [[unlikely]] {
    return false;
  }

  const int old_cost {matrix.at(chromosome.path.at(swap_idx - 1))
                      .at(chromosome.path.at(swap_idx)) +
                      matrix.at(chromosome.path.at(swap_idx))
                      .at(chromosome.path.at(swap_idx + 1)) +
                      matrix.at(chromosome.path.at(v_count - 1)).at(first_v) +
                      matrix.at(first_v).at(chromosome.path.at(1))};

  const int cost_diff {new_cost_left_s + new_cost_right_s + new_cost_left_in +
                       new_cost_right_in - old_cost};

  std::swap(chromosome.path.at(swap_idx), chromosome.path.at(0));
  std::swap(chromosome.vertices.at(swap_v), chromosome.vertices.at(first_v));

  chromosome.cost += cost_diff;

  return true;
}

static void cut(Population& population, int target_count) noexcept {
  //todo
}

//todo: reproduce + algorithm

}    // namespace gen::impl

namespace gen {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>&   matrix,
const tsp::GraphInfo&     graph_info,
const std::optional<int>& optimal_cost,
int                       count_of_itr,
int                       population_size,
int                       count_of_children,
int                       max_children_per_pair,
int                       mutations_per_1000) noexcept {
  if (count_of_itr < 1 || population_size < 2 || count_of_children < 1 ||
      max_children_per_pair < 1 || mutations_per_1000 < 0 ||
      mutations_per_1000 > 1000) [[unlikely]] {
    return tsp::ErrorAlgorithm::INVALID_PARAM;
  }

  if (matrix.empty()) [[unlikely]] {
    return tsp::ErrorAlgorithm::NO_PATH;
  }

  if (matrix.size() == 1) [[unlikely]] {
    return tsp::Solution {.path = {0}, .cost = 0};
  }

  if (matrix.size() == 2) [[unlikely]] {
    return nn::run(matrix, graph_info, optimal_cost);
  }

  return impl::algorithm(matrix,
                         graph_info,
                         optimal_cost,
                         count_of_itr,
                         population_size,
                         count_of_children,
                         max_children_per_pair,
                         mutations_per_1000);
}

}    // namespace gen
