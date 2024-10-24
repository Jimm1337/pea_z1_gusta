#include "bxb.hpp"
#include "util.hpp"

#include <cstddef>
#include <limits>
#include <list>
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

struct Node {
  std::list<Node> children;
  WorkingSolution value;
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
          current_cost != -1 && current_cost < min_cost) {
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
          current_cost != -1 && current_cost < min_cost) {
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

static void branch(Node& node) noexcept {
  // add all possible not explored paths from current node to solution space
  for (const auto& adjacent :
       get_adjacent(node.value.matrix, node.value.path.back())) {
    node.children.emplace_back(
    Node {.children = {},
          .value    = {[&adjacent, &node]() noexcept {
            tsp::Matrix<int> new_matrix {node.value.matrix};
            mark_used(new_matrix, node.value.path.back(), adjacent.vertex);
            const int reduction_cost {reduce(new_matrix)};

            return WorkingSolution {
                 .matrix = std::move(new_matrix),
                 .path =
              [&adjacent, &node]() noexcept {
                std::vector new_path {node.value.path};
                new_path.emplace_back(adjacent.vertex);
                return new_path;
              }(),
                 .cost = node.value.cost + reduction_cost + adjacent.cost};
          }()}});
  }
}

static tsp::Solution algorithm(const tsp::Matrix<int>& matrix,
                               int starting_vertex) noexcept {
  const size_t v_count {matrix.size()};

  // reduce initial matrix
  auto [root_matrix, root_reduce_cost] {[&matrix]() noexcept {
    std::pair result {matrix, -1};
    result.second = reduce(result.first);
    return result;
  }()};

  // init solution space
  Node root {
    .children = {},
    .value    = {.matrix = root_matrix,
                 .path   = {starting_vertex},
                 .cost   = root_reduce_cost}
  };

  // remember current best for bounding
  tsp::Solution current_best {.path = {},
                              .cost = std::numeric_limits<int>::max()};

  // priority queue to always explore least cost node
  std::
  priority_queue<Node*, std::vector<Node*>, bool (*)(Node*, Node*) noexcept>
  least_cost_heap {[](Node* lhs, Node* rhs) noexcept {
                     return lhs->value.cost > rhs->value.cost;
                   },
                   {&root}};

  while (!least_cost_heap.empty()) [[likely]] {
    Node& node {*least_cost_heap.top()};
    least_cost_heap.pop();

    // do not explore if already worse or equal
    if (node.value.cost >= current_best.cost) [[likely]] {
      continue;
    }

    branch(node);

    // if leaf add return and compare with best, if no return path ignore
    if (node.value.path.size() >= v_count) [[unlikely]] {
      if (const int return_cost {node.value.matrix.at(node.value.path.back())
                                 .at(node.value.path.front())};
          return_cost != -1) {
        node.value.cost += return_cost;
        node.value.path.emplace_back(node.value.path.front());

        if (node.value.cost < current_best.cost) [[unlikely]] {
          current_best = {.path = node.value.path, .cost = node.value.cost};
        }
      }
    }

    // add viable children to priority queue
    for (auto& child : node.children) {
      if (child.value.cost < current_best.cost) [[unlikely]] {
        least_cost_heap.push(&child);
      }
    }
  }

  return current_best;
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
    const tsp::Solution result {impl::algorithm(matrix, vertex)};
    if (result.cost < best.cost) {
      best = result;
    }
  }

  if (best.cost == std::numeric_limits<int>::max()) [[unlikely]] {
    return tsp::ErrorAlgorithm::NO_PATH;
  }

  return best;
}

}    // namespace bxb::lc
