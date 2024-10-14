#include "bf.hpp"
#include "nn.hpp"
#include <fmt/core.h>

int main(int argc, const char** argv) {
  auto result_nn = nn::run(DATA_DIR "test_6_as.txt");
  auto result_bf = bf::run(DATA_DIR "test_6_as.txt");

  if (result_nn.path.size() == 0) {
    return EXIT_FAILURE;
  }

  if (result_bf.path.size() == 0) {
    return EXIT_FAILURE;
  }

  fmt::println("NN Path: ");
  for (const auto& node : result_nn.path) {
    fmt::print("{} ", node);
  }
  fmt::println("\n");

  fmt::println("Cost: {}\n", result_nn.cost);

  fmt::println("BF Path: ");
  for (const auto& node : result_bf.path) {
    fmt::print("{} ", node);
  }
  fmt::println("\n");

  fmt::println("Cost: {}\n", result_bf.cost);

  return EXIT_SUCCESS;
}
