#include "gen.hpp"

#include "nn.hpp"

#include <list>
#include <numeric>
#include <random>
#include <set>
#include <stack>

//todo: implement

namespace gen::impl {
class WorkingSolution {
private:
  struct Allele {
    std::vector<size_t> v_indices;
    std::vector<int>    path;
    int                 cost;
  };

  Allele allele_first;
  Allele allele_last;
  int    first_last_v;
  int    first_last_v_cost;
  int    inter_allele_cost;

public:
  constexpr WorkingSolution(const tsp::Matrix<int>& matrix,
                            const tsp::Solution&    solution) noexcept:
    allele_first {
      .v_indices =
      std::vector(solution.path.size() - 1, std::numeric_limits<size_t>::max()),
      .path = std::vector(solution.path.begin() + 1,
                          solution.path.begin() + (solution.path.size() / 2)),
      .cost = 0},
    allele_last {
      .v_indices =
      std::vector(solution.path.size() - 1, std::numeric_limits<size_t>::max()),
      .path = std::vector(solution.path.begin() + (solution.path.size() / 2),
                          solution.path.end() - 1),
      .cost = 0},
    first_last_v {solution.path.at(0)} {
    for (int v {0}; v < allele_first.path.size(); ++v) {
      allele_first.v_indices.at(allele_first.path.at(v)) = v;

      if (v != 0) [[likely]] {
        allele_first.cost +=
        matrix.at(allele_first.path.at(v - 1)).at(allele_first.path.at(v));
      }
    }

    for (int v {0}; v < allele_last.path.size(); ++v) {
      allele_last.v_indices.at(allele_last.path.at(v)) = v;

      if (v != 0) [[likely]] {
        allele_last.cost +=
        matrix.at(allele_last.path.at(v - 1)).at(allele_last.path.at(v));
      }
    }

    inter_allele_cost =
    matrix.at(allele_first.path.back()).at(allele_last.path.front());
    first_last_v_cost =
    solution.cost - allele_first.cost - allele_last.cost - inter_allele_cost;
  }

  [[nodiscard]] constexpr std::weak_ordering operator<=>(
  const WorkingSolution& other) const noexcept {
    const auto cmp {cost() <=> other.cost()};
    if (cmp != std::strong_ordering::equal) {
      return cmp;
    }

    if (size() != other.size()) {
      return std::weak_ordering::equivalent;
    }

    for (int v {0}; v < size(); ++v) {
      if (at_c(v) < other.at_c(v)) {
        return std::weak_ordering::less;
      }
      if (at_c(v) > other.at_c(v)) {
        return std::weak_ordering::greater;
      }
    }

    return std::weak_ordering::equivalent;
  }

  [[nodiscard]] constexpr const int& at_c(size_t idx) const noexcept {
    if (idx == 0 ||
        idx == 1 + allele_first.path.size() + allele_last.path.size()) {
      return first_last_v;
    }
    if (idx < allele_first.path.size() + 1) {
      return allele_first.path.at(idx - 1);
    }
    return allele_last.path.at(idx - allele_first.path.size() - 1);
  }

  [[nodiscard]] constexpr size_t idx(int v) const noexcept {
    return v == first_last_v ? 0
         : allele_first.v_indices.at(v) != std::numeric_limits<size_t>::max()
         ? allele_first.v_indices.at(v) + 1
         : allele_last.v_indices.at(v) + allele_first.path.size() + 1;
  }

  [[nodiscard]] constexpr size_t size() const noexcept {
    return allele_first.path.size() + allele_last.path.size() + 2;
  }

  [[nodiscard]] constexpr const Allele& first_allele() const noexcept {
    return allele_first;
  }

  [[nodiscard]] constexpr const Allele& second_allele() const noexcept {
    return allele_last;
  }

  [[nodiscard]] constexpr const int& first_last_vertex() const noexcept {
    return first_last_v;
  }

  [[nodiscard]] constexpr int cost() const noexcept {
    return allele_first.cost + allele_last.cost + inter_allele_cost +
           first_last_v_cost;
  }

  constexpr bool try_swap_v(const tsp::Matrix<int>& matrix,
                            int                     v1,
                            int                     v2) noexcept {
    if (v1 == v2) {
      return true;
    }

    const size_t first_v_idx {idx(v1)};
    const int    first_prev {first_v_idx != 0 ? at(first_v_idx - 1)
                                              : at(size() - 2)};
    const int    first_next {at(first_v_idx + 1)};

    const size_t second_v_idx {idx(v2)};
    const int    second_prev {second_v_idx != 0 ? at(second_v_idx - 1)
                                                : at(size() - 2)};
    const int    second_next {at(second_v_idx + 1)};

    const int first_prev_old_cost {matrix.at(first_prev).at(v1)};
    const int first_next_old_cost {matrix.at(v1).at(first_next)};
    const int second_prev_old_cost {matrix.at(second_prev).at(v2)};
    const int second_next_old_cost {matrix.at(v2).at(second_next)};

    const int first_prev_new_cost {
      first_prev != v2 ? matrix.at(first_prev).at(v2) : matrix.at(v1).at(v2)};
    const int first_next_new_cost {
      v2 != first_next ? matrix.at(v2).at(first_next) : first_next_old_cost};
    const int second_prev_new_cost {
      second_prev != v1 ? matrix.at(second_prev).at(v1) : matrix.at(v2).at(v1)};
    const int second_next_new_cost {
      v1 != second_next ? matrix.at(v1).at(second_next) : second_next_old_cost};

    if (first_prev_new_cost == -1 || first_next_new_cost == -1 ||
        second_prev_new_cost == -1 || second_next_new_cost == -1) [[unlikely]] {
      return false;
    }

    const int v1_allele =
    first_v_idx == 0                                                      ? 0
    : allele_first.v_indices.at(v1) != std::numeric_limits<size_t>::max() ? 1
                                                                          : 2;
    const int v2_allele =
    second_v_idx == 0                                                     ? 0
    : allele_first.v_indices.at(v2) != std::numeric_limits<size_t>::max() ? 1
                                                                          : 2;

    if (v1_allele == 0) {
      if (v2_allele == 1) {
        if (v2 != allele_first.path.front()) {
          allele_first.cost += -second_prev_old_cost + first_prev_new_cost;
        }
        if (v2 != allele_first.path.back()) {
          allele_first.cost += -second_next_old_cost + first_next_new_cost;
        } else {
          inter_allele_cost = first_next_new_cost;
        }
        allele_first.v_indices.at(v1) = allele_first.v_indices.at(v2);
        allele_first.v_indices.at(v2) = std::numeric_limits<size_t>::max();
      } else {
        if (v2 != allele_last.path.front()) {
          allele_last.cost += -second_prev_old_cost + first_prev_new_cost;
        }
        if (v2 != allele_last.path.back()) {
          allele_last.cost += -second_next_old_cost + first_next_new_cost;
        }
        allele_last.v_indices.at(v1) = allele_last.v_indices.at(v2);
        allele_last.v_indices.at(v2) = std::numeric_limits<size_t>::max();
      }
      first_last_v_cost += -first_next_old_cost + -first_prev_old_cost +
                           second_next_new_cost + second_prev_new_cost;
    } else if (v2_allele == 0) {
      if (v1_allele == 1) {
        if (v1 != allele_first.path.front()) {
          allele_first.cost += -first_prev_old_cost + second_prev_new_cost;
        }
        if (v1 != allele_first.path.back()) {
          allele_first.cost += -first_next_old_cost + second_next_new_cost;
        } else {
          inter_allele_cost = second_next_new_cost;
        }
        allele_first.v_indices.at(v2) = allele_first.v_indices.at(v1);
        allele_first.v_indices.at(v1) = std::numeric_limits<size_t>::max();
      } else {
        if (v1 != allele_last.path.front()) {
          allele_last.cost += -first_prev_old_cost + second_prev_new_cost;
        }
        if (v1 != allele_last.path.back()) {
          allele_last.cost += -first_next_old_cost + second_next_new_cost;
        }
        allele_last.v_indices.at(v2) = allele_last.v_indices.at(v1);
        allele_last.v_indices.at(v1) = std::numeric_limits<size_t>::max();
      }
      first_last_v_cost += -second_next_old_cost + -second_prev_old_cost +
                           first_next_new_cost + first_prev_new_cost;
    } else if (v1_allele == v2_allele) {
      if (v1_allele == 1) {
        if (v1 != allele_first.path.front()) {
          allele_first.cost += -first_prev_old_cost + second_prev_new_cost;
        } else {
          first_last_v_cost += -first_prev_old_cost + second_prev_new_cost;
        }
        if (v2 != allele_first.path.front()) {
          allele_first.cost += -second_prev_old_cost + first_prev_new_cost;
        } else {
          first_last_v_cost += -second_prev_old_cost + first_prev_new_cost;
        }
        if (v1 != allele_first.path.back()) {
          allele_first.cost += -first_next_old_cost + second_next_new_cost;
        } else {
          inter_allele_cost = second_next_new_cost;
        }
        if (v2 != allele_first.path.back()) {
          allele_first.cost += -second_next_old_cost + first_next_new_cost;
        } else {
          inter_allele_cost = first_next_new_cost;
        }
        std::swap(allele_first.v_indices.at(v1), allele_first.v_indices.at(v2));
      } else {
        if (v1 != allele_last.path.front()) {
          allele_last.cost += -first_prev_old_cost + second_prev_new_cost;
        } else {
          inter_allele_cost = second_prev_new_cost;
        }
        if (v2 != allele_last.path.front()) {
          allele_last.cost += -second_prev_old_cost + first_prev_new_cost;
        } else {
          inter_allele_cost = first_prev_new_cost;
        }
        if (v1 != allele_last.path.back()) {
          allele_last.cost += -first_next_old_cost + second_next_new_cost;
        } else {
          first_last_v_cost += -first_next_old_cost + second_next_new_cost;
        }
        if (v2 != allele_last.path.back()) {
          allele_last.cost += -second_next_old_cost + first_next_new_cost;
        } else {
          first_last_v_cost += -second_next_old_cost + first_next_new_cost;
        }
        std::swap(allele_last.v_indices.at(v1), allele_last.v_indices.at(v2));
      }
    } else {
      if (v1_allele == 1) {
        if (v1 != allele_first.path.front()) {
          allele_first.cost += -first_prev_old_cost + second_prev_new_cost;
        } else {
          first_last_v_cost += -first_prev_old_cost + second_prev_new_cost;
        }
        if (v1 != allele_first.path.back()) {
          allele_first.cost += -first_next_old_cost + second_next_new_cost;
        } else {
          inter_allele_cost = second_next_new_cost;
        }
        if (v2 != allele_last.path.front()) {
          allele_last.cost += -second_prev_old_cost + first_prev_new_cost;
        } else {
          inter_allele_cost = first_prev_new_cost;
        }
        if (v2 != allele_last.path.back()) {
          allele_last.cost += -second_next_old_cost + first_next_new_cost;
        } else {
          first_last_v_cost += -second_next_old_cost + first_next_new_cost;
        }
        allele_first.v_indices.at(v2) = allele_first.v_indices.at(v1);
        allele_first.v_indices.at(v1) = std::numeric_limits<size_t>::max();
        allele_last.v_indices.at(v1)  = allele_last.v_indices.at(v2);
        allele_last.v_indices.at(v2)  = std::numeric_limits<size_t>::max();
      } else {
        if (v1 != allele_last.path.front()) {
          allele_last.cost += -first_prev_old_cost + second_prev_new_cost;
        } else {
          inter_allele_cost = second_prev_new_cost;
        }
        if (v1 != allele_last.path.back()) {
          allele_last.cost += -first_next_old_cost + second_next_new_cost;
        } else {
          first_last_v_cost += -first_next_old_cost + second_next_new_cost;
        }
        if (v2 != allele_first.path.front()) {
          allele_first.cost += -second_prev_old_cost + first_prev_new_cost;
        } else {
          first_last_v_cost += -second_prev_old_cost + first_prev_new_cost;
        }
        if (v2 != allele_first.path.back()) {
          allele_first.cost += -second_next_old_cost + first_next_new_cost;
        } else {
          inter_allele_cost = first_next_new_cost;
        }
        allele_first.v_indices.at(v1) = allele_first.v_indices.at(v2);
        allele_first.v_indices.at(v2) = std::numeric_limits<size_t>::max();
        allele_last.v_indices.at(v2)  = allele_last.v_indices.at(v1);
        allele_last.v_indices.at(v1)  = std::numeric_limits<size_t>::max();
      }
    }

    std::swap(at(idx(v1)), at(idx(v2)));

    return true;
  }

  [[nodiscard]] static constexpr std::optional<WorkingSolution> try_recombine(
  const tsp::Matrix<int>& matrix,
  const Allele&           first,
  const Allele&           last,
  int                     first_last_vertex) noexcept {
    if (first.v_indices.at(first_last_vertex) !=
        std::numeric_limits<size_t>::max() ||
        last.v_indices.at(first_last_vertex) !=
        std::numeric_limits<size_t>::max()) {
      return std::nullopt;
    }
    for (int v {0}; v < first.v_indices.size(); ++v) {
      if (first.v_indices.at(v) != std::numeric_limits<size_t>::max() &&
          last.v_indices.at(v) != std::numeric_limits<size_t>::max()) {
        return std::nullopt;
      }
    }
    if (matrix.at(first_last_vertex).at(first.path.front()) == -1 ||
        matrix.at(last.path.back()).at(first_last_vertex) == -1 ||
        matrix.at(first.path.back()).at(last.path.front()) == -1) {
      return std::nullopt;
    }

    return WorkingSolution {matrix, first, last, first_last_vertex};
  }

  [[nodiscard]] constexpr tsp::Solution to_solution() const noexcept {
    const size_t size {allele_first.path.size() + allele_last.path.size() + 2};

    tsp::Solution solution {.path =
                            [&size]() noexcept {
                              std::vector<int> path {};
                              path.reserve(size);
                              return path;
                            }(),
                            .cost = cost()};

    solution.path.emplace_back(first_last_v);
    const auto next_itr {
      std::ranges::copy(allele_first.path, std::back_inserter(solution.path))
      .out};
    std::ranges::copy(allele_last.path, next_itr);
    solution.path.emplace_back(first_last_v);

    return solution;
  }

private:
  constexpr WorkingSolution(const tsp::Matrix<int>& matrix,
                            const Allele&           first,
                            const Allele&           last,
                            int                     first_last_vertex) noexcept:
    allele_first {first},
    allele_last {last},
    first_last_v {first_last_vertex},
    first_last_v_cost {matrix.at(first_last_vertex).at(first.path.front()) +
                       matrix.at(last.path.back()).at(first_last_vertex)},
    inter_allele_cost {matrix.at(first.path.back()).at(last.path.front())} {
  }

  [[nodiscard]] constexpr int& at(size_t idx) noexcept {
    if (idx == 0 ||
        idx == 1 + allele_first.path.size() + allele_last.path.size()) {
      return first_last_v;
    }
    if (idx < allele_first.path.size() + 1) {
      return allele_first.path.at(idx - 1);
    }
    return allele_last.path.at(idx - allele_first.path.size() - 1);
  }
};

using Population = std::set<WorkingSolution, std::greater<>>;

[[nodiscard]] static std::variant<Population, tsp::ErrorAlgorithm>
init_population(const tsp::Matrix<int>& matrix,
                const tsp::GraphInfo&   graph_info,
                std::mt19937_64&        rand_src,
                int                     population_size) noexcept {
  const size_t v_count {matrix.size()};
  const int    max_retries {static_cast<int>(v_count) * population_size * 100};
  int          retries {0};

  const auto first_sol_result {nn::run(matrix, graph_info)};
  if (std::holds_alternative<tsp::ErrorAlgorithm>(first_sol_result)) {
    return std::get<tsp::ErrorAlgorithm>(first_sol_result);
  }

  Population new_population {};

  std::uniform_int_distribution distribution {};

  while (new_population.size() != population_size) {
    WorkingSolution next {matrix, std::get<tsp::Solution>(first_sol_result)};

    for (int i {static_cast<int>(next.size() - 1)}; i > 0; --i) {
      while (
      !next.try_swap_v(matrix,
                       next.at_c(distribution(
                       rand_src,
                       std::uniform_int_distribution<>::param_type(0, i))),
                       next.at_c(i))) {
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

      while (!node.try_swap_v(matrix, first_v, second_v)) [[unlikely]] {
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
      } else [[likely]] {
        population.emplace(node);
      }
    }
  }
}

static void reproduce(const tsp::Matrix<int>& matrix,
                      Population&             population,
                      int                     count_of_children,
                      int                     max_children_per_pair) noexcept {
  const size_t     population_size {population.size()};
  const Population reproducing {population};

  auto first_p_it {reproducing.begin()};
  auto second_p_it {std::next(first_p_it)};

  while (population.size() != population_size + count_of_children &&
         second_p_it != reproducing.end()) {
    std::vector potential_children {
      {WorkingSolution::try_recombine(matrix,
       first_p_it->first_allele(),
       second_p_it->second_allele(),
       first_p_it->first_last_vertex()),
       WorkingSolution::try_recombine(matrix,
       first_p_it->first_allele(),
       second_p_it->second_allele(),
       second_p_it->first_last_vertex()),
       WorkingSolution::try_recombine(matrix,
       first_p_it->first_allele(),
       second_p_it->second_allele(),
       second_p_it->first_last_vertex()),
       WorkingSolution::try_recombine(matrix,
       first_p_it->second_allele(),
       second_p_it->first_allele(),
       second_p_it->first_last_vertex()),
       WorkingSolution::try_recombine(matrix,
       second_p_it->first_allele(),
       first_p_it->second_allele(),
       first_p_it->first_last_vertex()),
       WorkingSolution::try_recombine(matrix,
       second_p_it->first_allele(),
       first_p_it->second_allele(),
       second_p_it->first_last_vertex()),
       WorkingSolution::try_recombine(matrix,
       second_p_it->second_allele(),
       first_p_it->first_allele(),
       first_p_it->first_last_vertex()),
       WorkingSolution::try_recombine(matrix,
       second_p_it->second_allele(),
       first_p_it->first_allele(),
       second_p_it->first_last_vertex())}
    };

    if (max_children_per_pair < potential_children.size()) {
      std::ranges::sort(potential_children,
                        std::greater {},
                        [](const auto& potential_child) noexcept {
                          return potential_child.has_value()
                               ? potential_child->cost()
                               : std::numeric_limits<int>::max();
                        });
    }

    for (int child {0};
         child < std::min(max_children_per_pair,
                          static_cast<int>(potential_children.size()));
         ++child) {
      if (potential_children.at(child).has_value()) {
        population.emplace(std::move(*potential_children.at(child)));
      }
    }

    ++first_p_it;
    ++second_p_it;
  }
}

static void cut(Population& population, int target_size) noexcept {
  population.erase(
  std::ranges::next(population.begin(), target_size, population.end()),
  population.end());
}

static std::variant<tsp::Solution, tsp::ErrorAlgorithm> algorithm(
const tsp::Matrix<int>& matrix,
const tsp::GraphInfo&   graph_info,
int                     count_of_itr,
int                     population_size,
int                     count_of_children,
int                     max_children_per_pair,
int                     mutations_per_1000) noexcept {
  std::mt19937_64 rand_src {std::random_device {}()};

  auto population_result {
    init_population(matrix, graph_info, rand_src, population_size)};
  if (std::holds_alternative<tsp::ErrorAlgorithm>(population_result))
  [[unlikely]] {
    return std::get<tsp::ErrorAlgorithm>(population_result);
  }
  Population population {std::move(std::get<Population>(population_result))};

  for (int itr {0}; itr < count_of_itr; ++itr) {
    reproduce(matrix, population, count_of_children, max_children_per_pair);
    mutate(matrix, population, rand_src, mutations_per_1000);
    cut(population, population_size);
  }

  return population.begin()->to_solution();
}

}    // namespace gen::impl

namespace gen {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix,
const tsp::GraphInfo&   graph_info,
int                     count_of_itr,
int                     population_size,
int                     count_of_children,
int                     max_children_per_pair,
int                     mutations_per_1000) noexcept {
  if (count_of_itr < 1 || population_size < 2 || count_of_children < 1 ||
      max_children_per_pair < 1 || mutations_per_1000 < 0 ||
      mutations_per_1000 > 1000) [[unlikely]] {
    return tsp::ErrorAlgorithm::INVALID_PARAM;
  }

  if (matrix.empty()) [[unlikely]] {
    return tsp::ErrorAlgorithm::NO_PATH;
  }

  if (matrix.size() == 1) [[unlikely]] {
    return tsp::Solution{.path = {0}, .cost = 0};
  }

  if (matrix.size() == 2) [[unlikely]] {
    return nn::run(matrix, graph_info);
  }

  return impl::algorithm(matrix,
                         graph_info,
                         count_of_itr,
                         population_size,
                         count_of_children,
                         max_children_per_pair,
                         mutations_per_1000);
}

}    // namespace gen
