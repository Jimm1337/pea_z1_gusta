#pragma once

#include "util.hpp"
#include <string_view>

#include <cstdint>

namespace random {

tsp::Solution run(std::string_view filename, size_t itr) noexcept;

}    // namespace random
