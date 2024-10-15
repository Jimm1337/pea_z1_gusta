#include "random.hpp"

#include "util.hpp"
#include <fmt/core.h>
#include <string_view>

#include <cstdint>
#include <limits>
#include <random>
#include <vector>

namespace random::impl {

struct WorkingSolution {
  tsp::Solution     solution;
  std::vector<bool> used_vertices;
};

struct RandomSource {
  std::mt19937_64                    engine;
  std::uniform_int_distribution<int> dist;
};

constexpr static int NUMBER_OF_RETRIES {1'0000};

void algorithm(const tsp::Matrix<int>& matrix,
               RandomSource&           random_source,
               tsp::Solution&          current_best) noexcept {
  const size_t v_count {matrix.size()};

  WorkingSolution work {
    {{}, 0},
    std::vector<bool>(v_count, false)
  };

  work.solution.path.emplace_back(random_source.dist(random_source.engine));
  work.used_vertices.at(work.solution.path.front()) = true;

  while (work.solution.path.size() != v_count) [[likely]] {
    const size_t starting_size {work.solution.path.size()};

    // optimization: trim already worse path
    if (work.solution.cost >= current_best.cost) {
      return;
    }

    // check if path exists
    bool has_path {false};
    for (int vertex {0}; vertex < v_count; ++vertex) {
      if (matrix.at(work.solution.path.back()).at(vertex) != -1) [[likely]] {
        has_path = true;
        break;
      }
    }

    if (!has_path) [[unlikely]] {
      return;
    }

    // while did not rand valid vertex generate, then add valid random vertex
    for (int i {0}; i < NUMBER_OF_RETRIES; ++i) {
      const int  random_v {random_source.dist(random_source.engine)};
      const bool used {work.used_vertices.at(random_v)};
      const int  cost {matrix.at(work.solution.path.back()).at(random_v)};

      if (!used && cost != -1) {
        work.solution.path.emplace_back(random_v);
        work.solution.cost              += cost;
        work.used_vertices.at(random_v)  = true;
        break;
      }
    }

    // no path found in max retries
    if (starting_size == work.solution.path.size()) [[unlikely]] {
      return;
    }
  }

  // edge case: no path from last to beggining
  if (matrix.at(work.solution.path.back()).at(work.solution.path.front()) == -1)
  [[unlikely]] {
    return;
  }

  // add cost to travel to beggining from last
  work.solution.cost +=
  matrix.at(work.solution.path.back()).at(work.solution.path.front());

  // if generated path would have better cost than current best close it and set new current best
  if (work.solution.cost < current_best.cost) [[unlikely]] {
    current_best = work.solution;
    current_best.path.emplace_back(current_best.path.front());
  }
}

}    // namespace random::impl

namespace random {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix,
int                     itr) noexcept {
  if (itr < 1) {
    return tsp::ErrorAlgorithm::INVALID_PARAM;
  }

  const size_t v_count {matrix.size()};

  if (v_count == 1) [[unlikely]] {    //edge case: 1 vertex
    return tsp::Solution {{{0}}, 0};
  }

  std::random_device device {};
  impl::RandomSource random_source {
    std::mt19937_64 {device()},
    std::uniform_int_distribution<int> {0, static_cast<int>(v_count - 1)}
  };

  tsp::Solution best {{}, std::numeric_limits<int>::max()};

  for (int iteration {0}; iteration < itr; ++iteration) {
    impl::algorithm(matrix, random_source, best);
  }

  if (best.cost == std::numeric_limits<int>::max()) [[unlikely]] {
    return tsp::ErrorAlgorithm::NO_PATH;
  }

  return best;
}

}    // namespace random
