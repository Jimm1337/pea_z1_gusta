#pragma once

#include "util.hpp"

namespace random {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix,
int                     itr) noexcept;

}    // namespace random
