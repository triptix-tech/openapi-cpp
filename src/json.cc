#include "openapi/json.h"

#include "fmt/chrono.h"

namespace openapi {

date_time_t tag_invoke(json::value_to_tag<date_time_t>, json::value const& jv) {
  auto d = date_time_t{};
  parse(jv.as_string(), d);
  return d;
}

void tag_invoke(json::value_from_tag, json::value& jv, date_time_t const v) {
  auto const offset_total_minutes = v.offset_.count();
  auto const offset_hours = offset_total_minutes / 60;
  auto const offset_minutes = std::abs(offset_total_minutes % 60);
  jv = v.offset_.count() == 0
           ? fmt::format("{:%FT%T}Z", v.time_)
           : fmt::format("{:%FT%T}{:+03d}:{:02d}", v.time_ + v.offset_,
                         offset_hours, offset_minutes);
}

}  // namespace openapi