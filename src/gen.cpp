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
};

using Population =
std::set<Chromosome,
         decltype([](const Chromosome& lhs,
                     const Chromosome& rhs) noexcept -> std::weak_ordering {
           if (lhs.cost != rhs.cost) {
             return lhs.cost <=> rhs.cost;
           }

           for (int i {0}; i < lhs.path.size(); ++i) {
             if (lhs.path.at(i) < rhs.path.at(i)) {
               return std::weak_ordering::less;
             }

             if (lhs.path.at(i) > rhs.path.at(i)) {
               return std::weak_ordering::greater;
             }
           }

           return std::weak_ordering::equivalent;
         })>;

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

  Chromosome nn_chromosome {[&nn_solution = std::get<tsp::Solution>(nn_result),
                             &v_count]() noexcept {
    std::vector vertices(v_count - 1, -1);
    for (int i {0}; i < v_count - 1; ++i) {
      vertices.at(nn_solution.path.at(i)) = i;
    }
    return {std::move(vertices), std::move(nn_solution.path), nn_solution.cost};
  }()};

  Population population {{nn_chromosome}};

  std::uniform_int_distribution dist {0, v_count - 1};
  while (population.size() != population_count) [[likely]] {
    const int first_v {dist(rand_src)};
    const int second_v {dist(rand_src)};

    if (first_v == second_v || nn_chromosome.vertices.at(first_v) == 0 ||
        nn_chromosome.vertices.at(second_v) == 0 ||
        nn_chromosome.vertices.at(first_v) == v_count - 1 ||
        nn_chromosome.vertices.at(second_v) == v_count - 1) [[unlikely]] {
      continue;
    }

    const auto first_idx {nn_chromosome.vertices.at(first_v)};
    const auto second_idx {nn_chromosome.vertices.at(second_v)};

    const int first_old_cost {matrix.at(first_v).at(first_v - 1) +
                              matrix.at(first_v).at(first_v + 1)};
    const int second_old_cost {matrix.at(second_v).at(second_v - 1) +
                               matrix.at(second_v).at(second_v + 1)};

    const int first_new_cost_left {matrix.at(first_v).at(second_v - 1)};
    const int first_new_cost_right {matrix.at(first_v).at(second_v + 1)};
    const int second_new_cost_left {matrix.at(second_v).at(first_v - 1)};
    const int second_new_cost_right {matrix.at(second_v).at(first_v + 1)};

    if (first_new_cost_left == -1 || first_new_cost_right == -1 ||
        second_new_cost_left == -1 || second_new_cost_right == -1)
    [[unlikely]] {
      continue;
    }

    const int cost_diff {first_new_cost_left + first_new_cost_right +
                         second_new_cost_left + second_new_cost_right -
                         first_old_cost - second_old_cost};

    std::swap(nn_chromosome.path.at(first_idx),
              nn_chromosome.path.at(second_idx));
    std::swap(nn_chromosome.vertices.at(first_v),
              nn_chromosome.vertices.at(second_v));

    population.emplace(nn_chromosome.vertices,
                       nn_chromosome.path,
                       nn_chromosome.cost + cost_diff);

    std::swap(nn_chromosome.path.at(first_idx),
              nn_chromosome.path.at(second_idx));
    std::swap(nn_chromosome.vertices.at(first_v),
              nn_chromosome.vertices.at(second_v));
  }

  return population;
}

[[nodiscard]] static std::optional<Chromosome> crossover(
const tsp::Matrix<int>& matrix,
const Chromosome&       parent_base,
const Chromosome&       parent2,
auto&                   rand_src,
int                     max_v_count_crossover) noexcept {
  const int v_count {static_cast<int>(parent_base.path.size() - 1)};

  Chromosome child {.vertices = parent_base.vertices,
                    .path     = parent_base.path,
                    .cost     = parent_base.cost};

  std::uniform_int_distribution dist {};

  const int first_v {dist(rand_src, {0, v_count - 1})};

  const auto first_idx {child.vertices.at(first_v)};
  const auto second_idx {parent2.vertices.at(first_v)};

  if (first_idx == 0 || first_idx == v_count - 1) [[unlikely]] {
    return std::nullopt;
  }

  const auto count {
    dist(rand_src,
         {1,
          std::min(std::min(v_count - 1 - second_idx, v_count - 1 - first_idx),
                   max_v_count_crossover)})};

  for (int i {1}; i < count; ++i) {
    const int old_cost {matrix.at(child.path.at(first_idx + i - 1))
                        .at(child.path.at(first_idx + i)) +
                        matrix.at(child.path.at(first_idx + i))
                        .at(child.path.at(first_idx + i + 1))};

    const int new_cost_left {matrix.at(child.path.at(first_idx + i - 1))
                             .at(parent2.path.at(second_idx + i))};
    const int new_cost_right {matrix.at(parent2.path.at(second_idx + i))
                              .at(child.path.at(first_idx + i + 1))};

    if (new_cost_left == -1 || new_cost_right == -1) [[unlikely]] {
      return std::nullopt;
    }

    const int cost_diff {new_cost_left + new_cost_right - old_cost};

    child.path.at(first_idx + i) = parent2.path.at(second_idx + i);
    child.cost += cost_diff;
  }
}

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
