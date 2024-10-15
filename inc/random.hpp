#pragma once

#include "util.hpp"
#include <string_view>

#include <cstdint>

namespace random {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix,
int                     itr) noexcept;

}    // namespace random
