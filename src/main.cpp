#include "nn.hpp"
#include <fmt/core.h>

int main(int argc, const char** argv) {
  auto result = nn::run(DATA_DIR "tsp_15_s.txt");

  fmt::println("Path: ");
  for (const auto& node : result.path) {
    fmt::print("{} ", node);
  }
  fmt::println("\n");

  fmt::println("Cost: {}", result.cost);

  return 0;
}
