#pragma once

#include "util.hpp"

namespace gen {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix,
const tsp::GraphInfo&   graph_info,
const std::optional<int>&      optimal_cost,
int                     count_of_itr,
int                     population_size,
int                     count_of_children,
int                     max_children_per_pair,
int                     max_v_count_crossover,
int                     mutations_per_1000) noexcept;

}    // namespace gen
