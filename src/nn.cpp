#include "nn.hpp"

#include "util.hpp"
#include <fmt/core.h>
#include <string_view>

#include <cstdint>
#include <limits>
#include <queue>
#include <vector>

namespace nn::impl {

struct WorkingSolution {
  tsp::Solution     solution;
  std::vector<bool> used_vertices;
};

void algorithm(const tsp::Matrix<int>& matrix,
               tsp::Solution&          current_best,
               int                     starting_vertex) noexcept {
  const size_t v_count = matrix.size();

  // potential paths to be processed
  std::queue<WorkingSolution> queue {};
  queue.push({
    {{starting_vertex}, 0},
    [&starting_vertex, &v_count]() noexcept {
      std::vector<bool> used {};
      used.resize(v_count, false);
      used.at(starting_vertex) = true;
      return used;                  }
    ()
  });

  while (!queue.empty()) [[likely]] {
    WorkingSolution current = queue.front();
    queue.pop();

    const int current_v    = current.solution.path.back();
    const int current_cost = current.solution.cost;

    // if all vertices added check if closed path is better than the best
    if (current.solution.path.size() == v_count) [[unlikely]] {
      const int return_cost = matrix.at(current_v).at(starting_vertex);

      if (return_cost != -1 && current_cost + return_cost < current_best.cost)
      [[unlikely]] {
        current_best       = current.solution;
        current_best.cost += return_cost;
        current_best.path.emplace_back(starting_vertex);
      }
      continue;
    }

    // optimization: trim already worse solutions
    if (current_cost >= current_best.cost) {
      continue;
    }

    // explore all adjacent vertices which cost minimal cost to travel to
    std::vector<int> nearest {};
    int              min_cost {std::numeric_limits<int>::max()};
    for (int vertex {0}; vertex < v_count; ++vertex) {
      const bool used = current.used_vertices.at(vertex);
      const int  cost = matrix.at(current_v).at(vertex);

      if (!used && cost != -1 && cost <= min_cost) [[unlikely]] {
        if (cost < min_cost) [[unlikely]] {
          nearest.clear();
          min_cost = cost;
        }
        nearest.emplace_back(vertex);
      }
    }

    // add next paths to be processed for nearest neighbour
    for (const auto& option : nearest) {
      queue.push([&current, &option, &min_cost]() noexcept {
        WorkingSolution next_itr {current};
        next_itr.solution.path.emplace_back(option);
        next_itr.solution.cost            += min_cost;
        next_itr.used_vertices.at(option)  = true;
        return next_itr;
      }());
    }
  }
}

}    // namespace nn::impl

namespace nn {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix) noexcept {
  const size_t v_count {matrix.size()};

  if (v_count == 1) [[unlikely]] {    //edge case: 1 vertex
    return tsp::Solution {{{0}}, 0};
  }

  tsp::Solution best {{}, std::numeric_limits<int>::max()};

  for (int vertex {0}; vertex < v_count; ++vertex) {
    impl::algorithm(matrix, best, vertex);
  }

  if (best.cost == std::numeric_limits<int>::max()) [[unlikely]] {
    return tsp::ErrorAlgorithm::NO_PATH;
  }

  return best;
}

}    // namespace nn
