#include "../cmake-build-debug/_deps/fmt-src/include/fmt/args.h"
#include "bxb.hpp"
#include "util.hpp"
#include <type_traits>

#include <cassert>
#include <cstddef>
#include <execution>
#include <functional>
#include <limits>
#include <list>
#include <optional>
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
  assert(last_v < matrix.size());
  assert(next_v < matrix.size());

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
[[nodiscard]] static constexpr int reduce(tsp::Matrix<int>& matrix) noexcept {
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

[[nodiscard]] static constexpr std::vector<Adjacent> get_adjacent(
const tsp::Matrix<int>& matrix,
int                     vertex) noexcept {
  assert(vartex < matrix.size());

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

class SolutionSpace {
public:
  class Node {
  public:
    friend class SolutionSpace;

  private:
    template<typename Val>
    requires std::is_same_v<std::remove_cvref_t<Val>, WorkingSolution>
    Node(Val&&                                              val,
         const std::optional<std::reference_wrapper<Node>>& parent) noexcept:
      parent {parent},
      value {std::forward<Val>(val)} {
    }

    std::list<Node>                             children;
    std::optional<std::reference_wrapper<Node>> parent;
    bool                                        no_path_flag {false};

  public:
    WorkingSolution value;

    [[nodiscard]] constexpr bool is_leaf() const noexcept {
      return value.path.size() == value.matrix.size();
    }

    void branch() noexcept {
      const std::vector adj_vertices {
        get_adjacent(value.matrix, value.path.back())};

      // check if there is a path
      if (adj_vertices.empty()) {
        if (!is_leaf()) [[unlikely]] {
          no_path_flag = true;
        }

        return;
      }

      // add all possible paths from current node to solution space
      for (const auto& adjacent : adj_vertices) {
        children.emplace_back(
        Node {WorkingSolution {[&adjacent, this]() noexcept {
                tsp::Matrix<int> new_matrix {value.matrix};
                mark_used(new_matrix, value.path.back(), adjacent.vertex);
                const int reduction_cost {reduce(new_matrix)};

                return WorkingSolution {
                  .matrix = std::move(new_matrix),
                  .path =
                  [&adjacent, this]() noexcept {
                    std::vector new_path {value.path};
                    new_path.emplace_back(adjacent.vertex);
                    return new_path;
                  }(),
                  .cost = value.cost + reduction_cost + adjacent.cost};
              }()},
              *this});
      }
    }
  };

  Node root;

  explicit SolutionSpace(const WorkingSolution& root_value) noexcept:
    root {root_value, std::nullopt} {
  }

  // bfs bound
  void bound(int threshhold) noexcept {
    assert(threshhold >= 0);

    std::queue<std::reference_wrapper<Node>> bfs_queue {{root}};

    while (!bfs_queue.empty()) [[likely]] {
      Node& node {bfs_queue.front().get()};
      bfs_queue.pop();

      node.children.remove_if([&threshhold](const Node& candidate) noexcept {
        return candidate.value.cost > threshhold || candidate.no_path_flag ||
               candidate.is_leaf();
      });

      for (Node& child : node.children) {
        bfs_queue.emplace(child);
      }
    }
  }

  [[nodiscard]] static constexpr std::optional<std::reference_wrapper<Node>>
  next(std::reference_wrapper<Node> node) noexcept {
    //todo

    std::optional<std::reference_wrapper<Node>> smallest {std::nullopt};

    while (!smallest.has_value()) {
      while (node.get().children.empty()) [[unlikely]] {
        node.get().no_path_flag = true;

        if (node.get().parent.has_value()) [[likely]] {
          node = *node.get().parent;
        } else [[unlikely]] {
          return node.get().parent;
        }
      }

      std::list<Node>& children {node.get().children};

      for (auto& child : children) {
        if (!child.no_path_flag &&
            (!smallest.has_value() ||
             child.value.cost < smallest->get().value.cost)) {
          smallest = child;
        }
      }

      if (!smallest.has_value()) [[unlikely]] {
        node = *node.get().parent;
      }
    }

    return smallest;
  }
};

static void algorithm(const tsp::Matrix<int>& matrix,
                      int                     starting_vertex) noexcept {
  // reduce initial matrix
  auto [root_matrix, root_reduce_cost] {[&matrix]() noexcept {
    std::pair result {matrix, -1};
    result.second = reduce(result.first);
    return result;
  }()};

  // init solution space
  SolutionSpace space {
    {.matrix = std::move(root_matrix),
     .path   = {starting_vertex},
     .cost   = root_reduce_cost}
  };

  tsp::Solution current_best {.path = {},
                              .cost = std::numeric_limits<int>::max()};

  // start from root
  for (std::optional<std::reference_wrapper<SolutionSpace::Node>> node_ref {
         space.root};
       node_ref.has_value();
       node_ref = space.next(*node_ref)) {
    SolutionSpace::Node& node {node_ref->get()};

    node.branch();

    if (node.is_leaf()) [[unlikely]] {
      if (node.value.cost < current_best.cost) [[unlikely]] {
        current_best = {.path = node.value.path, .cost = node.value.cost};
        space.bound(current_best.cost);
      }
    }
  }
}

}    // namespace bxb::impl
