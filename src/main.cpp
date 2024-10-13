#include "nn.hpp"
#include <fmt/core.h>

int main(int argc, const char** argv) {
  auto result = nn::run(DATA_DIR "tsp_20_1_as.txt");

  if (result.path.size() == 0) {
    return EXIT_FAILURE;
  }

  fmt::println("Path: ");
  for (const auto& node : result.path) {
    fmt::print("{} ", node);
  }
  fmt::println("\n");

  fmt::println("Cost: {}", result.cost);

  return EXIT_SUCCESS;
}
