#include <string_view>

#include <vector>

namespace nn {

// TSP problem solution
struct Solution {
  std::vector<int> path;
  int              cost;
};

// Closest node candidates
struct Candidate {
  int vertex;
  int cost;
};

Solution run(std::string_view filename) noexcept;

}    // namespace nn
