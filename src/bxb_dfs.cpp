#include "bxb.hpp"
#include "util.hpp"

#include <cstddef>
#include <limits>
#include <stack>
#include <vector>

namespace bxb::dfs::impl {

struct WorkingSolution {
  std::vector<bool> used_vertices;
  tsp::Solution     solution;
};

constexpr static std::vector<WorkingSolution> branch(
const tsp::Matrix<int>& matrix,
const WorkingSolution&  node,
int                     max_cost) noexcept {
  const size_t v_count {matrix.size()};

  std::vector<WorkingSolution> children {};

  for (int candidate {0}; candidate < v_count; ++candidate) {
    if (const int cost {matrix.at(node.solution.path.back()).at(candidate)};
        !node.used_vertices.at(candidate) && cost != -1 &&
        node.solution.cost + cost < max_cost) [[likely]] {
      children.emplace_back([&candidate, &node, &cost]() noexcept {
        WorkingSolution next {node};
        next.solution.path.emplace_back(candidate);
        next.solution.cost               += cost;
        next.used_vertices.at(candidate)  = true;
        return next;
      }());
    }
  }

  return children;
}

static void algorithm(const tsp::Matrix<int>& matrix,
                      tsp::Solution&          current_best,
                      int                     starting_vertex) {
  const size_t v_count {matrix.size()};

  std::stack<WorkingSolution> dfs_stack {
    {WorkingSolution {.used_vertices =
                      [&starting_vertex, &v_count]() noexcept {
                        std::vector root_used_v {std::vector(v_count, false)};
                        root_used_v.at(starting_vertex) = true;
                        return root_used_v;
                      }(),
                      .solution = {.path = {starting_vertex}, .cost = 0}}}};

  while (!dfs_stack.empty()) [[likely]] {
    WorkingSolution node {std::move(dfs_stack.top())};
    dfs_stack.pop();

    const int current_v {node.solution.path.back()};
    const int current_cost {node.solution.cost};

    // do not explore if already worse or equal
    if (current_cost >= current_best.cost) [[likely]] {
      continue;
    }

    // if leaf add return and compare with best, if no return path ignore
    if (node.solution.path.size() == v_count) [[unlikely]] {
      if (const int return_cost {matrix.at(current_v).at(starting_vertex)};
          return_cost != -1 && current_cost + return_cost < current_best.cost)
      [[likely]] {
        current_best       = std::move(node.solution);
        current_best.cost += return_cost;
        current_best.path.emplace_back(starting_vertex);
      }
    } else [[likely]] {
      // if not leaf add viable children to priority queue
      for (const auto& child : branch(matrix, node, current_best.cost)) {
        dfs_stack.emplace(child);
      }
    }
  }
}

}    // namespace bxb::dfs::impl

namespace bxb::dfs {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix,
const tsp::GraphInfo&   graph_info) noexcept {
  const size_t v_count {matrix.size()};

  if (v_count == 1) [[unlikely]] {    //edge case: 1 vertex
    return tsp::Solution {.path = {{0}}, .cost = 0};
  }

  tsp::Solution best {.path = {}, .cost = std::numeric_limits<int>::max()};

  if (graph_info.full_graph && graph_info.symmetric_graph) {
    impl::algorithm(matrix, best, 0);
  } else {
    for (int vertex {0}; vertex < v_count; ++vertex) {
      impl::algorithm(matrix, best, vertex);
    }
  }

  if (best.cost == std::numeric_limits<int>::max()) [[unlikely]] {
    return tsp::ErrorAlgorithm::NO_PATH;
  }

  return best;
}

}    // namespace bxb::dfs
