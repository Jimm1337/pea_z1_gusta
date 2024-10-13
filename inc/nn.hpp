#pragma once

#include "util.hpp"
#include <string_view>

namespace nn {

tsp::Solution run(std::string_view filename) noexcept;

}    // namespace nn
