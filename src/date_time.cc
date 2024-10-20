#include "openapi/date_time.h"

#include "utl/verify.h"

#include "date/date.h"
#include "date/tz.h"

namespace openapi {

date_time_t now() {
  return std::chrono::time_point_cast<std::chrono::seconds>(
      std::chrono::system_clock::now());
}

void parse(std::string_view s, date_time_t& v) {
  auto lower = std::string{s};
  std::transform(begin(lower), end(lower), begin(lower),
                 [](auto c) { return std::tolower(c); });

  auto in = std::istringstream{lower};

  auto ot = offset_time{};
  in >> date::parse("%Ft%T%Ez", ot.time_, ot.offset_);
  if (!in.fail()) {
    v = ot;
    return;
  }

  in.clear();
  in.seekg(0);

  in >> date::parse("%Ft%H:%M%Ez", ot.time_, ot.offset_);
  if (!in.fail()) {
    v = ot;
    return;
  }

  in.clear();
  in.seekg(0);

  auto sys_time = std::chrono::sys_seconds{};
  in >> date::parse("%Ft%Tz", sys_time);
  if (!in.fail()) {
    v = sys_time;
    return;
  }

  in.clear();
  in.seekg(0);

  in >> date::parse("%Ft%H:%Mz", sys_time);
  if (!in.fail()) {
    v = sys_time;
    return;
  }

  throw utl::fail("failed to parse timestamp {}", lower);
}

}  // namespace openapi