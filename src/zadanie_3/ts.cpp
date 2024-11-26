#include "zadanie_3/ts.hpp"

#include "zadanie_1/nn.hpp"
#include "util.hpp"

#include <vector>
#include <variant>
#include <optional>
#include <utility>
#include <limits>
#include <cstddef>

namespace ts::impl {

struct WorkingSolution {
  std::vector<size_t> v_indices; // vertex -> index in path (first/last is 0)
  tsp::Solution       solution;
};

struct SwapCandidate {
  int  first_v;
  int  second_v;
  int  delta_cost;
  bool tabu;
  bool aspiration_criterion;
};

static std::variant<WorkingSolution, tsp::ErrorAlgorithm> get_first_solution(
const tsp::Matrix<int>& matrix,
const tsp::GraphInfo&   graph_info,
std::optional<int>      optimal_cost) noexcept {
  const size_t v_count {matrix.size()};

  // use nn
  const auto upper_bound_result {nn::run(matrix, graph_info, optimal_cost)};
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

constexpr static void mark_tabu(tsp::Matrix<int>& tabu_matrix,
                                int               first_v,
                                int               second_v,
                                int               count_of_tabu_itr) noexcept {
  tabu_matrix.at(first_v).at(second_v) = count_of_tabu_itr;
  tabu_matrix.at(second_v).at(first_v) = count_of_tabu_itr;
}

constexpr static bool is_tabu(const tsp::Matrix<int>& tabu_matrix,
                              int                     first_v,
                              int                     second_v) noexcept {
  return tabu_matrix.at(first_v).at(second_v) > 0 ||
         tabu_matrix.at(second_v).at(first_v) > 0;
}

constexpr static void update_tabu(tsp::Matrix<int>& tabu_matrix) noexcept {
  for (auto& each : tabu_matrix) {
    for (auto& value : each) {
      --value;
    }
  }
}

constexpr static int get_delta_cost(const tsp::Matrix<int>& matrix,
                                    const WorkingSolution&  current_solution,
                                    int                     first_v,
                                    int                     second_v) noexcept {
  // O(1) get delta cost for swap -> get new and old costs for 4 edges, account for edge cases (adjacent/first/last)

  const size_t first_v_idx {current_solution.v_indices.at(first_v)};
  const int    first_prev {first_v_idx != 0
                           ? current_solution.solution.path.at(first_v_idx - 1)
                           : current_solution.solution.path.at(
                          current_solution.solution.path.size() - 2)};
  const int    first_next {current_solution.solution.path.at(first_v_idx + 1)};

  const size_t second_v_idx {current_solution.v_indices.at(second_v)};
  const int second_prev {current_solution.solution.path.at(second_v_idx - 1)};
  const int second_next {current_solution.solution.path.at(second_v_idx + 1)};

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
    return std::numeric_limits<int>::max();
  }

  const int new_cost {first_prev_new_cost + first_next_new_cost +
                      second_prev_new_cost + second_next_new_cost};

  const int old_cost {first_prev_old_cost + first_next_old_cost +
                      second_prev_old_cost + second_next_old_cost};

  return new_cost - old_cost;
}

constexpr static SwapCandidate get_best_candidate(
const tsp::Matrix<int>& matrix,
const tsp::Matrix<int>& tabu_matrix,
const WorkingSolution&  solution,
int                     current_best_cost) noexcept {
  const size_t path_size {solution.solution.path.size()};

  SwapCandidate best_candidate {.first_v    = 0,
                                .second_v   = 1,
                                .delta_cost = std::numeric_limits<int>::max(),
                                .tabu       = false,
                                .aspiration_criterion = false};

  for (int first_v_idx {0}; first_v_idx < path_size - 2; ++first_v_idx) {
    for (int second_v_idx {first_v_idx + 1}; second_v_idx < path_size - 1;
         ++second_v_idx) {
      const int first_v {solution.solution.path.at(first_v_idx)};
      const int second_v {solution.solution.path.at(second_v_idx)};

      const int delta_cost {
        get_delta_cost(matrix, solution, first_v, second_v)};

      // if would break solution, skip
      if (delta_cost == std::numeric_limits<int>::max()) [[unlikely]] {
        continue;
      }

      const bool tabu {is_tabu(tabu_matrix, first_v, second_v)};
      const bool aspiration_criterion {solution.solution.cost + delta_cost <
                                       current_best_cost};

      if ((!tabu || aspiration_criterion) &&
          delta_cost < best_candidate.delta_cost) {
        best_candidate =
        SwapCandidate {.first_v              = first_v,
                       .second_v             = second_v,
                       .delta_cost           = delta_cost,
                       .tabu                 = tabu,
                       .aspiration_criterion = aspiration_criterion};
      }
    }
  }

  return best_candidate;
}

constexpr static void perform_swap(WorkingSolution&     solution,
                                   const SwapCandidate& swap) noexcept {
  const size_t first_v_idx {solution.v_indices.at(swap.first_v)};
  const size_t second_v_idx {solution.v_indices.at(swap.second_v)};

  std::swap(solution.solution.path.at(first_v_idx),
            solution.solution.path.at(second_v_idx));
  std::swap(solution.v_indices.at(swap.first_v),
            solution.v_indices.at(swap.second_v));

  if (first_v_idx == 0) {
    solution.solution.path.back() = swap.second_v;
  }

  solution.solution.cost += swap.delta_cost;
}

// memory -> O(n^2 + 3 * n) >> tabu_matrix + 2 solutins -> O(n^2 + n)
// time -> O(n^2 + 2 * n^2 * itr_count) >> nn + tabu -> O(n^2 + n^2 * itr_count)
template<typename WorkingSolutionType, typename TabuMatrixType>
requires std::is_same_v<std::remove_cvref_t<WorkingSolutionType>,
                        WorkingSolution> &&
         std::is_same_v<std::remove_cvref_t<TabuMatrixType>, tsp::Matrix<int>>
static tsp::Solution algorithm(const tsp::Matrix<int>&   matrix,
                               const std::optional<int>& optimal_cost,
                               TabuMatrixType&&          tabu_matrix_in,
                               WorkingSolutionType&&     starting_solution,
                               int                       itr_count,
                               int no_improve_stop_itr_count,
                               int tabu_itr_count) noexcept {
  tsp::Matrix<int> tabu_matrix {std::forward<TabuMatrixType>(tabu_matrix_in)};
  WorkingSolution  work {std::forward<WorkingSolutionType>(starting_solution)}; //time O(n^2) - nn
  tsp::Solution    best {work.solution};
  int              no_improve_itr_count {0};

  // finish if optimal solution is nn solution
  if (optimal_cost.has_value() && best.cost == *optimal_cost) {
    return best;
  }

  for (int itr {0}; itr < itr_count; ++itr) {
    const SwapCandidate best_candidate {
      get_best_candidate(matrix, tabu_matrix, work, best.cost)}; //time O(n^2) - check all swaps

    const bool can_swap {
      !best_candidate.tabu || best_candidate.aspiration_criterion ||
      best_candidate.delta_cost != std::numeric_limits<int>::max()};
    const bool is_improvement {best_candidate.delta_cost < 0};

    if (!can_swap || !is_improvement) [[unlikely]] {
      ++no_improve_itr_count;
    }

    if (no_improve_itr_count == no_improve_stop_itr_count) [[unlikely]] {
      return best;
    }

    if (can_swap) [[likely]] {
      perform_swap(work, best_candidate);
      mark_tabu(tabu_matrix,
                best_candidate.first_v,
                best_candidate.second_v,
                tabu_itr_count);

      if (is_improvement) {
        no_improve_itr_count = 0;

        if (work.solution.cost < best.cost) {
          best = work.solution;

          // finish if optimal solution is found
          if (optimal_cost.has_value() && best.cost == *optimal_cost) {
            return best;
          }
        }
      }
    }

    update_tabu(tabu_matrix); //time O(n^2) - update tabu
  }

  return best;
}

}    // namespace ts::impl

namespace ts {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>&   matrix,
const tsp::GraphInfo&     graph_info,
const std::optional<int>& optimal_cost,
int                       itr_count,
int                       no_improve_stop_itr_count,
int                       tabu_itr_count) noexcept {
  const size_t v_count {matrix.size()};

  // param check
  if (itr_count < 1 || no_improve_stop_itr_count < 1 || tabu_itr_count < 1)
  [[unlikely]] {
    return tsp::ErrorAlgorithm::INVALID_PARAM;
  }

  if (v_count == 1) [[unlikely]] {    //edge case: 1 vertex
    return tsp::Solution {.path = {{0}}, .cost = 0};
  }

  // check if path exists
  auto first_solution_result {
    impl::get_first_solution(matrix, graph_info, optimal_cost)};
  if (std::holds_alternative<tsp::ErrorAlgorithm>(first_solution_result)) {
    return std::get<tsp::ErrorAlgorithm>(first_solution_result);
  }

  return impl::algorithm(matrix,
                         optimal_cost,
                         std::vector(v_count, std::vector(v_count, 0)),
                         std::move(std::get<impl::WorkingSolution>(first_solution_result)),
                         itr_count,
                         no_improve_stop_itr_count,
                         tabu_itr_count);
}

}    // namespace ts
