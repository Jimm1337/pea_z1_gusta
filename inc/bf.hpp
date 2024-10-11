#pragma once

#include "util.hpp"
#include <string_view>

namespace bf {

tsp::Solution run(std::string_view filename) noexcept;

}    // namespace bf
