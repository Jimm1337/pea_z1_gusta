#include "bxb.hpp"
#include "util.hpp"
#include "nn.hpp"

#include <cstddef>
#include <limits>
#include <queue>
#include <vector>

namespace bxb::bfs::impl {

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

  std::queue<WorkingSolution> bfs_queue {
    {WorkingSolution {.used_vertices =
                      [&starting_vertex, &v_count]() noexcept {
                        std::vector root_used_v {std::vector(v_count, false)};
                        root_used_v.at(starting_vertex) = true;
                        return root_used_v;
                      }(),
                      .solution = {.path = {starting_vertex}, .cost = 0}}}};

  while (!bfs_queue.empty()) [[likely]] {
    WorkingSolution node {std::move(bfs_queue.front())};
    bfs_queue.pop();

    const int current_v {node.solution.path.back()};
    const int current_cost {node.solution.cost};

    // do not explore if already worse or equal
    if (current_cost >= current_best.cost) [[unlikely]] {
      continue;
    }

    // if leaf add return and compare with best, if no return path ignore
    if (node.solution.path.size() == v_count) [[unlikely]] {
      if (const int return_cost {matrix.at(current_v).at(starting_vertex)};
          return_cost != -1 && current_cost + return_cost < current_best.cost)
      [[unlikely]] {
        current_best       = std::move(node.solution);
        current_best.cost += return_cost;
        current_best.path.emplace_back(starting_vertex);
      }
    } else [[likely]] {
      // if not leaf add viable children to priority queue
      for (const auto& child : branch(matrix, node, current_best.cost)) {
        bfs_queue.emplace(child);
      }
    }
  }
}

}    // namespace bxb::bfs::impl

namespace bxb::bfs {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix,
const tsp::GraphInfo&   graph_info) noexcept {
  const size_t v_count {matrix.size()};

  if (v_count == 1) [[unlikely]] {    //edge case: 1 vertex
    return tsp::Solution {.path = {{0}}, .cost = 0};
  }

  // upper bound = nn solution
  const auto upper_bound_result{nn::run(matrix, graph_info)};
  if (std::holds_alternative<tsp::ErrorAlgorithm>(upper_bound_result)) {
    return upper_bound_result;
  }
  tsp::Solution best {std::get<tsp::Solution>(upper_bound_result)};

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

}    // namespace bxb::bfs
