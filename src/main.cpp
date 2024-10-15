#include "bf.hpp"
#include "nn.hpp"
#include "random.hpp"
#include "util.hpp"
#include <fmt/core.h>

int main(int argc, const char** argv) {
  const auto arg_result {util::arg::read(argc, argv)};
  if (util::error::handle(arg_result) == tsp::State::ERROR) {
    return EXIT_FAILURE;
  }
  const tsp::Arguments arg {std::get<tsp::Arguments>(arg_result)};

  const auto config_result {util::config::read(arg.config_path)};
  if (util::error::handle(config_result) == tsp::State::ERROR) {
    return EXIT_FAILURE;
  }
  const tsp::Instance config {std::get<tsp::Instance>(config_result)};

  const auto timed_result {[&arg, &config]() noexcept {
    switch (arg.algorithm) {
      case tsp::Algorithm::BRUTE_FORCE:
        return util::measured_run(bf::run, config.matrix);
      case tsp::Algorithm::NEAREST_NEIGHBOUR:
        return util::measured_run(nn::run, config.matrix);
      case tsp::Algorithm::RANDOM:
        return util::measured_run(random::run, config.matrix, config.param);
      default:
        std::terminate();
    }
  }()};
  if (util::error::handle(timed_result) == tsp::State::ERROR) {
    return EXIT_FAILURE;
  }
  const tsp::Result timed {std::get<tsp::Result>(timed_result)};

  util::output::report(arg, config, timed);
}
