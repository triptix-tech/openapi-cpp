#pragma once

#include <stdexcept>
#include <string>

namespace openapi {

struct bad_request_exception : public std::runtime_error {
  explicit bad_request_exception(std::string const& message)
      : std::runtime_error{message} {}
};

}  // namespace openapi
