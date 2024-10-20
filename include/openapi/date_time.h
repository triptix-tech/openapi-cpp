#pragma once

#include <chrono>

namespace openapi {

struct offset_time {
  offset_time() = default;

  template <typename Period>
  offset_time(
      std::chrono::time_point<std::chrono::system_clock, Period> const t,
      std::chrono::minutes const offset = std::chrono::minutes{0})
      : offset_{offset},
        time_{std::chrono::time_point_cast<std::chrono::minutes>(t)} {}

  operator std::chrono::sys_seconds() const { return time_; }
  std::chrono::sys_seconds operator*() const { return time_; }

  auto operator<=>(offset_time const&) const = default;

  std::chrono::minutes offset_{0U};
  std::chrono::sys_seconds time_{std::chrono::seconds{0U}};
};

using date_time_t = offset_time;

date_time_t now();

void parse(std::string_view, date_time_t&);

}  // namespace openapi