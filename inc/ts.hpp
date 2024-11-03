#pragma once

#include "util.hpp"

namespace ts {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix, int itr_count, int no_improve_stop_itr_count, int tabu_itr_count) noexcept;

}