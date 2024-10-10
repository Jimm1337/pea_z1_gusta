#include "util.hpp"

#include <fmt/core.h>
#include <string_view>

#include <fstream>
#include <vector>

namespace util::input {

// return: cost matrix from mierzwa format tsp
std::vector<std::vector<int>> tsp_mierzwa(std::string_view filename) noexcept {
  size_t vertex_count {0};

  std::ifstream file {filename.data()};

  if (file.fail()) {
    fmt::println("[E] Error reading file: {}", filename.data());
    return {};
  }

  file >> vertex_count;

  std::vector<std::vector<int>> cost_matrix {};
  cost_matrix.resize(vertex_count);

  for (int i = 0; i < vertex_count; ++i) {
    cost_matrix.at(i).resize(vertex_count, -2);

    for (int j = 0; j < vertex_count; ++j) {
      int cost_read {-2};
      file >> cost_read;
      cost_matrix.at(i).at(j) = cost_read;
    }
  }

  return cost_matrix;
}

}    // namespace util::input
