#pragma once

#include "util.hpp"

namespace ts {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix,
const tsp::GraphInfo&   graph_info,
const std::optional<int>&      optimal_cost,
int                     itr_count,
int                     no_improve_stop_itr_count,
int                     tabu_itr_count) noexcept;

}    // namespace ts
