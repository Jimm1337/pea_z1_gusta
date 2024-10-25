#include "bxb.hpp"
#include "util.hpp"

#include <cstddef>
#include <limits>
#include <stack>
#include <utility>
#include <vector>

namespace bxb::impl {

struct WorkingSolution {
  std::vector<int> path;
  int              cost;
};

struct Adjacent {
  int vertex;
  int cost;
};

struct TraceResult {
  tsp::Matrix<int> matrix;
  int              cost;
};

constexpr static void mark_used(tsp::Matrix<int>& matrix,
                                int               last_v,
                                int               next_v) noexcept {
  // mark last_v as unconnected
  for (auto& cost : matrix.at(last_v)) {
    cost = -1;
  }

  // mark next_v as unreachable
  for (auto& costs_to_next : matrix) {
    costs_to_next.at(next_v) = -1;
  }

  // make return not possible
  matrix.at(next_v).at(last_v) = -1;
}

// return: reduction cost
[[nodiscard]] constexpr static int reduce(tsp::Matrix<int>& matrix) noexcept {
  const size_t v_count {matrix.size()};

  // reduce the matrix and find the reduction cost
  int reduction_cost {0};

  for (int col {0}; col < v_count; ++col) {
    int min_cost {std::numeric_limits<int>::max()};

    for (int i {0}; i < v_count; ++i) {
      if (const int current_cost {matrix.at(col).at(i)};
          current_cost != -1 && current_cost < min_cost) [[unlikely]] {
        min_cost = current_cost;
      }
    }

    if (min_cost != std::numeric_limits<int>::max()) {
      for (int i {0}; i < v_count; ++i) {
        if (matrix.at(col).at(i) != -1) {
          matrix.at(col).at(i) -= min_cost;
        }
      }

      reduction_cost += min_cost;
    }
  }

  for (int row {0}; row < v_count; ++row) {
    int min_cost {std::numeric_limits<int>::max()};

    for (int i {0}; i < v_count; ++i) {
      if (const int current_cost {matrix.at(i).at(row)};
          current_cost != -1 && current_cost < min_cost) [[unlikely]] {
        min_cost = current_cost;
      }
    }

    if (min_cost != std::numeric_limits<int>::max()) {
      for (int i {0}; i < v_count; ++i) {
        if (matrix.at(i).at(row) != -1) {
          matrix.at(i).at(row) -= min_cost;
        }
      }

      reduction_cost += min_cost;
    }
  }

  return reduction_cost;
}

[[nodiscard]] constexpr static std::vector<Adjacent> get_adjacent(
const tsp::Matrix<int>& matrix,
int                     vertex) noexcept {
  const size_t v_count {matrix.size()};

  std::vector<Adjacent> result {};
  result.reserve(matrix.size() - 1);

  for (int candidate {0}; candidate < v_count; ++candidate) {
    if (const int cost {matrix.at(vertex).at(candidate)}; cost != -1) {
      result.emplace_back(Adjacent {.vertex = candidate, .cost = cost});
    }
  }

  return result;
}

constexpr static std::vector<WorkingSolution> branch(
const WorkingSolution&  node,
const tsp::Matrix<int>& node_matrix,
int                     traced_cost) noexcept {
  std::vector<WorkingSolution> children {};

  // add all possible paths from current node to solution space
  for (const auto& adjacent : get_adjacent(node_matrix, node.path.back())) {
    children.emplace_back(
    WorkingSolution {[&adjacent, &node, &node_matrix, &traced_cost]() noexcept {
      tsp::Matrix<int> new_matrix {node_matrix};
      mark_used(new_matrix, node.path.back(), adjacent.vertex);
      const int reduction_cost {reduce(new_matrix)};

      return WorkingSolution {
        .path =
        [&adjacent, &node]() noexcept {
          std::vector new_path {node.path};
          new_path.emplace_back(adjacent.vertex);
          return new_path;
        }(),
        .cost = traced_cost + reduction_cost + adjacent.cost};
    }()});
  }

  return children;
}

constexpr static TraceResult trace_matrix(const WorkingSolution&  node,
                                          const tsp::Matrix<int>& root_matrix,
                                          int root_reduce_cost) noexcept {
  tsp::Matrix<int> matrix {root_matrix};
  int         cost {root_reduce_cost};

  for (int vertex {1}; vertex < node.path.size(); ++vertex) {
    const int this_node_cost {
      root_matrix.at(node.path.at(vertex - 1)).at(node.path.at(vertex))};

    mark_used(matrix, node.path.at(vertex - 1), node.path.at(vertex));

    cost += reduce(matrix);
    cost += this_node_cost;
  }

  return {.matrix = matrix, .cost = cost};
}

static void algorithm(const tsp::Matrix<int>& matrix,
                      int                     starting_vertex,
                      tsp::Solution&          current_best) noexcept {
  const size_t v_count {matrix.size()};

  const auto [root_matrix, root_reduce_cost] {[&matrix]() noexcept {
    tsp::Matrix<int> root_mtx {matrix};
    const int        reduce_cost {reduce(root_mtx)};
    return std::pair {root_mtx, reduce_cost};
  }()};

  // priority queue to always explore least cost node
  std::stack<WorkingSolution> bfs_queue{
    {WorkingSolution {.path = {starting_vertex}, .cost = root_reduce_cost}}};

  while (!bfs_queue.empty()) [[likely]] {
    WorkingSolution node {bfs_queue.top()};
    bfs_queue.pop();

    // do not explore if already worse or equal
    if (node.cost >= current_best.cost) [[likely]] {
      continue;
    }

    // trace reduced matrix
    const auto [node_matrix, traced_cost] {
      trace_matrix(node, root_matrix, root_reduce_cost)};

    // if leaf add return and compare with best, if no return path ignore
    if (node.path.size() == v_count) [[unlikely]] {
      if (const int return_cost {
            node_matrix.at(node.path.back()).at(node.path.front())};
          return_cost != -1) {
        node.cost += return_cost;
        node.path.emplace_back(node.path.front());

        if (node.cost < current_best.cost) [[unlikely]] {
          current_best = {.path = std::move(node.path), .cost = node.cost};
        }
      }
    } else [[likely]] {
      // if not leaf add viable children to priority queue
      for (const auto& child : branch(node, node_matrix, traced_cost)) {
        if (child.cost < current_best.cost) [[unlikely]] {
          bfs_queue.push(child);
        }
      }
    }
  }
}

}    // namespace bxb::impl

namespace bxb::dfs {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix) noexcept {
  const size_t v_count {matrix.size()};

  if (v_count == 1) [[unlikely]] {    //edge case: 1 vertex
    return tsp::Solution {.path = {{0}}, .cost = 0};
  }

  tsp::Solution best {.path = {}, .cost = std::numeric_limits<int>::max()};

  //repeat for each vertex to get the optimal solution
  for (int vertex {0}; vertex < v_count; ++vertex) {
    impl::algorithm(matrix, vertex, best);
  }

  if (best.cost == std::numeric_limits<int>::max()) [[unlikely]] {
    return tsp::ErrorAlgorithm::NO_PATH;
  }

  return best;
}

}    // namespace bxb::bfs
