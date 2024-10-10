#include "nn.hpp"

#include "util.hpp"
#include <fmt/core.h>
#include <string_view>

#include <algorithm>
#include <vector>

namespace nn::impl {

std::vector<Candidate> find_nearest(
const std::vector<int>&  costs,
const std::vector<bool>& used_vertices) noexcept {
  std::vector<Candidate> candidates {};
  int                    current_min_cost {std::numeric_limits<int>::max()};

  // find minimal cost for connected nodes that are not used already
  for (int vertex_candidate {0}; vertex_candidate < costs.size();
       ++vertex_candidate) {
    const int candidate_cost {costs.at(vertex_candidate)};

    if (!used_vertices.at(vertex_candidate) &&
        candidate_cost < current_min_cost) {
      current_min_cost = candidate_cost;
    }
  }

  // add all nodes that cost minmimal cost to candidates
  for (int adj_vertex {0}; adj_vertex < costs.size(); ++adj_vertex) {
    if (!used_vertices.at(adj_vertex) &&
        costs.at(adj_vertex) == current_min_cost) {
      candidates.emplace_back(Candidate {adj_vertex, current_min_cost});
    }
  }

  return candidates;
}

void branch(const std::vector<std::vector<int>>& matrix,
            std::vector<bool>                    used,
            std::vector<Solution>&               solutions,
            int                                  current_vertex,
            size_t                               branch_number = 0) noexcept {
  used.at(current_vertex) = true;

  //base case
  if (std::ranges::all_of(used, [](const auto& elem) {
        return elem;
      })) {
    return;
  }

  std::vector<Candidate> nearest {
    find_nearest(matrix.at(current_vertex), used)};

  // case: no path
  if (nearest.size() == 0) {
    //trim (no complete path)
    solutions.at(branch_number).cost = std::numeric_limits<int>::max();
    return;
  }

  // first closest node (same solution/branch)
  if (nearest.size() >= 1) {
    const Candidate next {nearest.front()};

    solutions.at(branch_number).path.emplace_back(next.vertex);
    solutions.at(branch_number).cost += next.cost;
    branch(matrix, used, solutions, next.vertex, branch_number);
  }

  // next closest nodes (if exist) (create new branches)
  if (nearest.size() > 1) {
    const size_t branching_point {branch_number};

    for (int candidate {1}; candidate < nearest.size(); ++candidate) {
      branch_number = solutions.size();
      solutions.emplace_back(solutions.at(branching_point));

      const Candidate next {nearest.at(candidate)};

      solutions.at(branch_number).path.emplace_back(next.vertex);
      solutions.at(branch_number).cost += next.cost;

      branch(matrix, used, solutions, next.vertex, branch_number);
    }
  }
}

}    // namespace nn::impl

namespace nn {

Solution run(std::string_view filename) noexcept {
  const std::vector<std::vector<int>> matrix {
    util::input::tsp_mierzwa(filename)};

  const size_t v_count {matrix.size()};

  if (v_count == 0) {    //error: bad input
    fmt::println("[E] Invalid input");
    return {};
  } else if (v_count == 1) {    //edge case: 1 vertex
    return {{{0}}, 0};
  }

  // todo: case with all edges same?

  std::vector<Solution> solutions {};

  for (int vertex = 0; vertex < v_count; ++vertex) {
    solutions.emplace_back(Solution {{vertex}, 0});

    std::vector<bool> used_vertices {};
    used_vertices.resize(v_count);

    impl::branch(matrix,
                 std::move(used_vertices),
                 solutions,
                 vertex,
                 solutions.size() - 1);
  }

  // add return to start
  for (auto& solution : solutions) {
    const int first_node     = solution.path.front();
    const int last_node      = solution.path.back();
    const int cost_to_return = matrix.at(last_node).at(first_node);

    solution.path.emplace_back(first_node);
    solution.cost += cost_to_return;
  }

  // return one of mininimal cost paths
  return *std::ranges::min_element(
  solutions,
  [](const Solution& first, const Solution& second) noexcept {
    return first.cost < second.cost;
  });
}

}    // namespace nn
