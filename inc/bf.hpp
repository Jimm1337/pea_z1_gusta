#pragma once

#include "util.hpp"
#include <string_view>

namespace bf {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix) noexcept;

}    // namespace bf
