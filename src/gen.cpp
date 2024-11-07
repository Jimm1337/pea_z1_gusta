#include "gen.hpp"

//todo: implement

namespace gen {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix,
int                     count_of_itr,
int                     population_size,
int                     count_of_children,
int                     crossovers_per_100,
int                     mutations_per_1000) noexcept {
  return tsp::ErrorAlgorithm::NO_PATH;
}

}    // namespace gen