#pragma once

#include "util.hpp"
#include <string_view>

#include <vector>

namespace nn {

// Closest node candidates
struct Candidate {
  int vertex;
  int cost;
};

tsp::Solution run(std::string_view filename) noexcept;

}    // namespace nn
