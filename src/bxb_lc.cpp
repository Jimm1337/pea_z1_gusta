#include "bxb.hpp"
#include "util.hpp"

#include <cstddef>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

namespace bxb::impl {

struct WorkingSolution {
  tsp::Matrix<int> matrix;
  std::vector<int> path;
  int              cost;
};

struct Adjacent {
  int vertex;
  int cost;
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
const WorkingSolution& node) noexcept {
  std::vector<WorkingSolution> children {};
  children.reserve(node.matrix.size());

  // add all possible paths from current node to solution space
  for (const auto& adjacent : get_adjacent(node.matrix, node.path.back())) {
    children.emplace_back(WorkingSolution {[&adjacent, &node]() noexcept {
      tsp::Matrix<int> new_matrix {node.matrix};
      mark_used(new_matrix, node.path.back(), adjacent.vertex);
      const int reduction_cost {reduce(new_matrix)};

      return WorkingSolution {
        .matrix = std::move(new_matrix),
        .path =
        [&adjacent, &node]() noexcept {
          std::vector new_path {node.path};
          new_path.emplace_back(adjacent.vertex);
          return new_path;
        }(),
        .cost = node.cost + reduction_cost + adjacent.cost};
    }()});
  }

  return children;
}

static void algorithm(const tsp::Matrix<int>& matrix,
                               int starting_vertex,
                               tsp::Solution& current_best) noexcept {
  const size_t v_count {matrix.size()};

  // priority queue to always explore least cost node
  std::priority_queue<WorkingSolution,
                      std::vector<WorkingSolution>,
                      bool (*)(const WorkingSolution&,
                               const WorkingSolution&) noexcept>
  least_cost_heap {
    [](const WorkingSolution& lhs, const WorkingSolution& rhs) noexcept {
      return lhs.cost > rhs.cost;
    },
    {[&matrix, &starting_vertex]() noexcept {
      tsp::Matrix<int> root_matrix {matrix};
      const int        root_reduce_cost {reduce(root_matrix)};
      return WorkingSolution {.matrix = std::move(root_matrix),
                              .path   = {starting_vertex},
                              .cost   = root_reduce_cost};
    }()}};

  while (!least_cost_heap.empty()) [[likely]] {
    WorkingSolution node {least_cost_heap.top()};
    least_cost_heap.pop();

    // do not explore if already worse or equal
    if (node.cost >= current_best.cost) [[likely]] {
      continue;
    }

    // if leaf add return and compare with best, if no return path ignore
    if (node.path.size() >= v_count) [[unlikely]] {
      if (const int return_cost {
            node.matrix.at(node.path.back()).at(node.path.front())};
          return_cost != -1) {
        node.cost += return_cost;
        node.path.emplace_back(node.path.front());

        if (node.cost < current_best.cost) [[unlikely]] {
          current_best = {.path = node.path, .cost = node.cost};
        }
      }
    } else [[likely]] {
      // if not leaf add viable children to priority queue
      for (const auto& child : branch(node)) {
        if (child.cost < current_best.cost) [[unlikely]] {
          least_cost_heap.push(child);
        }
      }
    }
  }
}

}    // namespace bxb::impl

namespace bxb::lc {

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

}    // namespace bxb::lc
