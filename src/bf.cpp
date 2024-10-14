#include "bf.hpp"

#include <fmt/core.h>

#include <algorithm>
#include <queue>
#include <vector>

namespace bf::impl {

struct Candidate {
  int vertex;
  int cost;
};

struct WorkingSolution {
  tsp::Solution     solution;
  std::vector<bool> used_vertices;
};

void algorithm(const tsp::Matrix<int>& matrix,
               tsp::Solution&          current_best,
               int                     starting_vertex) {
  const size_t v_count = matrix.size();

  std::queue<WorkingSolution> queue {};
  queue.push({
    {{starting_vertex}, 0},
    [&starting_vertex, &v_count] {
      std::vector<bool> used {};
      used.resize(v_count, false);
      used.at(starting_vertex) = true;
      return used;                  }
    ()
  });

  while (!queue.empty()) {
    WorkingSolution current = queue.front();
    queue.pop();

    const int current_v    = current.solution.path.back();
    const int current_cost = current.solution.cost;

    if (current.solution.path.size() == v_count) {
      const int return_cost = matrix.at(current_v).at(starting_vertex);

      if (return_cost != -1 && current_cost + return_cost < current_best.cost) {
        current_best       = current.solution;
        current_best.cost += return_cost;
        current_best.path.emplace_back(starting_vertex);
      }
      continue;
    }

    if (current_cost >= current_best.cost) {
      continue;
    }

    std::vector<Candidate> options {};
    for (int vertex {0}; vertex < v_count; ++vertex) {
      const bool used = current.used_vertices.at(vertex);
      const int  cost = matrix.at(current_v).at(vertex);

      if (!used && cost != -1) {
        options.emplace_back(Candidate {vertex, cost});
      }
    }

    for (const auto& option : options) {
      queue.push([&current, &option] {
        WorkingSolution next_itr {current};
        next_itr.solution.path.emplace_back(option.vertex);
        next_itr.solution.cost                   += option.cost;
        next_itr.used_vertices.at(option.vertex)  = true;
        return next_itr;
      }());
    }
  }
}

}    // namespace bf::impl

namespace bf {

tsp::Solution run(std::string_view filename) noexcept {
  const tsp::Matrix<int> matrix {util::input::tsp_mierzwa(filename)};

  const size_t v_count {matrix.size()};

  if (v_count == 0) {    //error: bad input
    fmt::println("[E] Invalid input");
    return {};
  } else if (v_count == 1) {    //edge case: 1 vertex
    return {{{0}}, 0};
  }

  tsp::Solution best {{}, std::numeric_limits<int>::max()};

  for (int vertex {0}; vertex < v_count; ++vertex) {
    impl::algorithm(matrix, best, vertex);
  }

  return best;
}

}    // namespace bf
