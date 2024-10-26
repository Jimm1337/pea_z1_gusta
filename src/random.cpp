#include "random.hpp"

#include "util.hpp"
#include <fmt/core.h>

#include <limits>
#include <random>
#include <vector>

namespace random::impl {

struct WorkingSolution {
  std::vector<bool> used_vertices;
  tsp::Solution     solution;
};

struct RandomSource {
  std::mt19937_64                    engine;
  std::uniform_int_distribution<int> dist;
};

constexpr static int NUMBER_OF_RETRIES {1'0000};

constexpr static void algorithm(const tsp::Matrix<int>& matrix,
                                RandomSource&           random_source,
                                tsp::Solution&          current_best) noexcept {
  const size_t v_count {matrix.size()};

  WorkingSolution work {
    .used_vertices = std::vector(v_count, false),
    .solution      = {.path = {}, .cost = 0}
  };

  work.solution.path.emplace_back(random_source.dist(random_source.engine));
  work.used_vertices.at(work.solution.path.front()) = true;

  while (work.solution.path.size() != v_count) [[likely]] {
    const size_t starting_size {work.solution.path.size()};

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

      if (const int cost {matrix.at(work.solution.path.back()).at(random_v)};
          !used && cost != -1) {
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

  // edge case: no path from last to beginning
  if (matrix.at(work.solution.path.back()).at(work.solution.path.front()) == -1)
  [[unlikely]] {
    return;
  }

  // add cost to travel to beginning from last
  work.solution.cost +=
  matrix.at(work.solution.path.back()).at(work.solution.path.front());

  // if generated path would have better cost than current best close it and set new current best
  if (work.solution.cost < current_best.cost) [[unlikely]] {
    current_best = std::move(work.solution);
    current_best.path.emplace_back(current_best.path.front());
  }
}

}    // namespace random::impl

namespace random {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix,
const int               time_ms) noexcept {
  const auto start {std::chrono::high_resolution_clock::now()};

  if (time_ms < 1) {
    return tsp::ErrorAlgorithm::INVALID_PARAM;
  }

  const size_t v_count {matrix.size()};

  if (v_count == 1) [[unlikely]] {    //edge case: 1 vertex
    return tsp::Solution {.path = {{0}}, .cost = 0};
  }

  std::random_device device {};
  impl::RandomSource random_source {
    .engine = std::mt19937_64 {device()},
    .dist   = std::uniform_int_distribution {0, static_cast<int>(v_count - 1)}
  };

  tsp::Solution best {.path = {}, .cost = std::numeric_limits<int>::max()};

  // run for specified time
  while (true) {
    const tsp::Time elapsed{std::chrono::high_resolution_clock::now() - start};
    if (elapsed.count() > time_ms) {
      break;
    }

    impl::algorithm(matrix, random_source, best);
  }

  if (best.cost == std::numeric_limits<int>::max()) [[unlikely]] {
    return tsp::ErrorAlgorithm::NO_PATH;
  }

  return best;
}

}    // namespace random
