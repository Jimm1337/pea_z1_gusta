#include "global.hpp"
#include <string_view>

#include <atomic>
#include <string>
#include <vector>

namespace util::config {

struct Inputs {
  std::vector<std::string> filenames;
  std::vector<int32_t>     optimal_path_costs;
};

struct Data {
  Inputs                   inputs;
  std::vector<std::string> output_filenames;
  //todo: stop cond
};

}    // namespace util::config

namespace util::output {

}    // namespace util::output

namespace util::input {

// return: cost matrix from mierzwa format tsp
std::vector<std::vector<int>> tsp_mierzwa(std::string_view filename) noexcept;

}    // namespace util::input

namespace util::arg {

bool has_string(int argc, const char** argv, std::string_view str) noexcept;

}    // namespace util::arg

namespace util::display {

void busy_dots(std::atomic_bool& stop_token) noexcept;
void config(const config::Data& config) noexcept;

}    // namespace util::display
