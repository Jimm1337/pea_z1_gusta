#include "bxb.hpp"
#include "util.hpp"
#include <type_traits>

#include <cstddef>
#include <functional>
#include <limits>
#include <list>
#include <optional>
#include <queue>
#include <vector>

namespace bxb::impl {

struct WorkingSolution {
  tsp::Matrix<int> matrix;
  std::vector<int> path;
  int              cost;
};

class SolutionSpace {
public:
  class Node {
  public:
    friend class SolutionSpace;

  private:
    Node(const WorkingSolution&                             val,
         const std::optional<std::reference_wrapper<Node>>& parent) noexcept:
      parent {parent},
      value {val} {
    }

    std::list<Node>                             children;
    std::optional<std::reference_wrapper<Node>> parent;

  public:
    WorkingSolution value;

    [[nodiscard]] constexpr std::optional<std::reference_wrapper<Node>>
    get_parent() noexcept {
      return parent;
    }

    [[nodiscard]] constexpr const std::list<Node>& get_children() noexcept {
      return children;
    }

    // return: new node ref
    template<typename Val>
    requires std::is_same_v<std::remove_cvref_t<Val>, WorkingSolution>
    Node& chain(Val&& value) noexcept {
      return children.emplace_back(Node {std::forward<Val>(value), *this});
    }
  };

  Node root;

  explicit SolutionSpace(const WorkingSolution& root_value) noexcept:
    root {root_value, std::nullopt} {
  }

  // bfs bound
  void bound(int threshhold) noexcept {
    std::queue<Node*> bfs_queue{{&root}};

    while (!bfs_queue.empty()) {
      Node* node {bfs_queue.front()};
      bfs_queue.pop();

      node->children.remove_if([&threshhold](const Node& candidate) noexcept {
        return candidate.value.cost > threshhold;
      });

      for (Node& child : node->children) {
        bfs_queue.emplace(&child);
      }
    }
  }
};

struct ReductionResult {
  int reduction_cost;
  int next_v_cost;
};

[[nodiscard]] static constexpr ReductionResult reduce(tsp::Matrix<int>& matrix,
                                                      int               last_v,
                                                      int next_v) noexcept {
  const size_t v_count {matrix.size()};

  // save cost from previous
  const int next_v_cost {[&last_v, &next_v, &matrix]() noexcept {
    return matrix.at(last_v).at(next_v);
  }()};

  // reduce the matrix and find the reduction cost
  int reduction_cost {0};

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

  return {.reduction_cost = reduction_cost, .next_v_cost = next_v_cost};
}

}    // namespace bxb::impl
