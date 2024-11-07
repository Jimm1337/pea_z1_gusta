#pragma once

#include "util.hpp"

namespace bxb::dfs {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix, const tsp::GraphInfo& graph_info) noexcept;

}    // namespace bxb::dfs

namespace bxb::bfs {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix, const tsp::GraphInfo& graph_info) noexcept;

}    // namespace bxb::bfs

namespace bxb::lc {

[[nodiscard]] std::variant<tsp::Solution, tsp::ErrorAlgorithm> run(
const tsp::Matrix<int>& matrix, const tsp::GraphInfo& graph_info) noexcept;

}    // namespace bxb::lc
