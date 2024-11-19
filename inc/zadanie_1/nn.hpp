#pragma once

#include "util.hpp"

namespace nn {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix,
const tsp::GraphInfo&   graph_info,
const std::optional<int>&      optimal_cost) noexcept;

}    // namespace nn
