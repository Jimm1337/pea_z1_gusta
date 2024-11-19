#pragma once

#include "util.hpp"

namespace util::measure {

std::variant<std::monostate, tsp::ErrorMeasure> execute_measurements(
const tsp::MeasuringRun& run) noexcept;

}    // namespace util::measure
