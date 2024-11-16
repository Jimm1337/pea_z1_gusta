#include "bxb.hpp"
#include "nn.hpp"
#include "util.hpp"
#include <initializer_list>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace bxb::lc::impl {

template<typename T, typename Cmp = std::less<T>>
class Heap {
  std::vector<T> data;
  Cmp            cmp;

public:
  constexpr Heap(const Cmp&               cmp  = Cmp {},
                 std::initializer_list<T> init = {}) noexcept:
    data {init},
    cmp {cmp} {
    std::ranges::make_heap(data, cmp);
  }

  template<typename Ty>
  constexpr void push(Ty&& value) noexcept {
    data.emplace_back(std::forward<Ty>(value));
    std::ranges::push_heap(data, cmp);
  }

  constexpr T pop() noexcept {
    std::ranges::pop_heap(data, cmp);
    const T top {std::move(data.back())};
    data.pop_back();
    return top;
  }

  [[nodiscard]] constexpr bool empty() const noexcept {
    return data.empty();
  }
};

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
        node.solution.cost + cost < max_cost) {
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

  Heap<WorkingSolution,
       bool (*)(const WorkingSolution&, const WorkingSolution&) noexcept>
  least_cost_heap {
    [](const WorkingSolution& lhs, const WorkingSolution& rhs) noexcept {
      return lhs.solution.cost > rhs.solution.cost;
    },
    {WorkingSolution {.used_vertices =
                      [&starting_vertex, &v_count]() noexcept {
                        std::vector root_used_v {std::vector(v_count, false)};
                        root_used_v.at(starting_vertex) = true;
                        return root_used_v;
                      }(),
                      .solution = {.path = {starting_vertex}, .cost = 0}}}};

  while (!least_cost_heap.empty()) [[likely]] {
    WorkingSolution node {least_cost_heap.pop()};

    const int current_v {node.solution.path.back()};
    const int current_cost {node.solution.cost};

    // do not explore if already worse or equal
    if (current_cost >= current_best.cost) [[likely]] {
      continue;
    }

    // if leaf add return and compare with best, if no return path ignore
    if (node.solution.path.size() == v_count) [[unlikely]] {
      if (const int return_cost {matrix.at(current_v).at(starting_vertex)};
          return_cost != -1 && current_cost + return_cost < current_best.cost) {
        current_best       = std::move(node.solution);
        current_best.cost += return_cost;
        current_best.path.emplace_back(starting_vertex);
      }
    } else [[likely]] {
      // if not leaf add viable children to priority queue
      for (auto&& child : branch(matrix, node, current_best.cost)) {
        least_cost_heap.push(std::move(child));
      }
    }
  }
}

}    // namespace bxb::lc::impl

namespace bxb::lc {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>&   matrix,
const tsp::GraphInfo&     graph_info,
const std::optional<int>& optimal_cost) noexcept {
  const size_t v_count {matrix.size()};

  if (v_count == 1) [[unlikely]] {    //edge case: 1 vertex
    return tsp::Solution {.path = {{0}}, .cost = 0};
  }

  // upper bound = nn solution
  auto upper_bound_result {nn::run(matrix, graph_info, optimal_cost)};
  if (std::holds_alternative<tsp::ErrorAlgorithm>(upper_bound_result)) {
    return upper_bound_result;
  }
  tsp::Solution best {std::move(std::get<tsp::Solution>(upper_bound_result))};

  if (graph_info.full_graph && graph_info.symmetric_graph) {
    impl::algorithm(matrix, best, 0);
  } else {
    for (int vertex {0}; vertex < v_count; ++vertex) {
      impl::algorithm(matrix, best, vertex);
      if (optimal_cost.has_value() && best.cost == *optimal_cost) {
        break;
      }
    }
  }

  if (best.cost == std::numeric_limits<int>::max()) [[unlikely]] {
    return tsp::ErrorAlgorithm::NO_PATH;
  }

  return best;
}

}    // namespace bxb::lc
