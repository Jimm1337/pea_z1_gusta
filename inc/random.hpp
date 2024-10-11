#pragma once

#include "util.hpp"
#include <string_view>

namespace random {

tsp::Solution run(std::string_view filename) noexcept;

}    // namespace random
