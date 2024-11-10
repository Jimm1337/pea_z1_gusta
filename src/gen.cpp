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
    std::vector<bool> used;
    std::vector<int>  path;
    int               cost;
  };

  std::vector<size_t> v_indices;
  Allele              allele_first;
  Allele              allele_last;
  int                 first_last_v;
  int                 first_last_v_cost;
  int                 inter_allele_cost;

public:
  constexpr WorkingSolution(const tsp::Matrix<int>& matrix,
                            const tsp::Solution&    solution) noexcept:
    v_indices {std::vector(solution.path.size() - 1,
                           std::numeric_limits<size_t>::max())},
    first_last_v {solution.path.at(0)},
    allele_first {
      .used = std::vector(solution.path.size() - 1, false),
      .path = std::vector(solution.path.begin() + 1,
                          solution.path.begin() + (solution.path.size() / 2)),
      .cost = 0},
    allele_last {
      .used = std::vector(solution.path.size() - 1, false),
      .path = std::vector(solution.path.begin() + (solution.path.size() / 2),
                          solution.path.end() - 1),
      .cost = 0} {
    v_indices.at(first_last_v) = 0;

    for (int v {0}; v < allele_first.path.size(); ++v) {
      v_indices.at(allele_first.path.at(v))         = v + 1;
      allele_first.used.at(allele_first.path.at(v)) = true;

      if (v != 0) [[likely]] {
        allele_first.cost +=
        matrix.at(allele_first.path.at(v - 1)).at(allele_first.path.at(v));
      }
    }

    for (int v {0}; v < allele_last.path.size(); ++v) {
      v_indices.at(allele_last.path.at(v)) = v + allele_first.path.size() + 1;
      allele_last.used.at(allele_last.path.at(v)) = true;

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

  [[nodiscard]] constexpr const size_t& idx(int v) const noexcept {
    return v_indices.at(v);
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

  [[nodiscard]] constexpr const int& begin_end() const noexcept {
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

    const int v1_allele = first_v_idx == 0         ? 0
                        : allele_first.used.at(v1) ? 1
                                                   : 2;
    const int v2_allele = second_v_idx == 0        ? 0
                        : allele_first.used.at(v2) ? 1
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
        allele_first.used.at(v1)               = true;
        allele_first.used.at(v2)               = false;
      } else {
        if (v2 != allele_last.path.front()) {
          allele_last.cost += -second_prev_old_cost + first_prev_new_cost;
        }
        if (v2 != allele_last.path.back()) {
          allele_last.cost += -second_next_old_cost + first_next_new_cost;
        }
        allele_last.used.at(v1)                                          = true;
        allele_last.used.at(v2) = false;
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
        allele_first.used.at(v2)              = true;
        allele_first.used.at(v1)              = false;
      } else {
        if (v1 != allele_last.path.front()) {
          allele_last.cost += -first_prev_old_cost + second_prev_new_cost;
        }
        if (v1 != allele_last.path.back()) {
          allele_last.cost += -first_next_old_cost + second_next_new_cost;
        }
        allele_last.used.at(v2)                                         = true;
        allele_last.used.at(v1)                                         = false;
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
        allele_first.used.at(v1) = false;
        allele_first.used.at(v2) = true;
        allele_last.used.at(v2)  = false;
        allele_last.used.at(v1)  = true;
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
        allele_first.used.at(v2) = false;
        allele_first.used.at(v1) = true;
        allele_last.used.at(v1)  = false;
        allele_last.used.at(v2)  = true;
      }
    }

    std::swap(at(idx(v1)), at(idx(v2)));
    std::swap(v_indices.at(v1), v_indices.at(v2));

    return true;
  }

private:
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

static std::variant<WorkingSolution, tsp::ErrorAlgorithm> get_first_solution(
const tsp::Matrix<int>& matrix,
const tsp::GraphInfo&   graph_info) noexcept {
  const auto upper_bound_result {nn::run(matrix, graph_info)};
  if (std::holds_alternative<tsp::ErrorAlgorithm>(upper_bound_result)) {
    return std::get<tsp::ErrorAlgorithm>(upper_bound_result);
  }

  return WorkingSolution {matrix, std::get<tsp::Solution>(upper_bound_result)};
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

static Population select_and_reproduce(tsp::Matrix<int>& matrix,
                                       Population&       population,
                                       std::mt19937_64&  rand_src,
                                       int               count_of_children,
                                       int crossovers_per_100) noexcept {
  const size_t population_size {population.size()};

  //todo: reproduce, cut up to pop size
}

}    // namespace gen::impl

namespace gen {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix,
const tsp::GraphInfo&   graph_info,
int                     count_of_itr,
int                     population_size,
int                     count_of_children,
int                     mutations_per_1000) noexcept {
  return tsp::ErrorAlgorithm::NO_PATH;
}

}    // namespace gen
