#pragma once

#include <stdexcept>

namespace openapi {

struct missing_param_exception : public std::exception {
  explicit missing_param_exception(std::string_view name) : param_{name} {}
  const char* what() const noexcept override { return "missing parameter"; }
  std::string_view param_;
};

}  // namespace openapi