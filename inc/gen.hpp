#pragma once

#include "util.hpp"

namespace gen {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix,
int                     count_of_itr,
int                     population_size,
int                     count_of_children,
int                     crossovers_per_100,
int                     mutations_per_1000) noexcept;

}    // namespace gen
