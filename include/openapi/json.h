#pragma once

#include "boost/json.hpp"

#include "utl/verify.h"

#include "openapi/date_time.h"

namespace openapi {

namespace json = boost::json;

date_time_t tag_invoke(json::value_to_tag<date_time_t>, json::value const&);
void tag_invoke(json::value_from_tag, json::value&, date_time_t const);

template <class T>
void extract_member(json::object const& o, T& t, json::string_view key) {
  auto const it = o.find(key);
  if (it == o.end()) {
    [[unlikely]];
    throw utl::fail("key {} not found in {}", key, json::serialize(o));
  }
  t = json::value_to<T>(it->value());
}

template <class T>
void extract_member(json::object const& o,
                    std::optional<T>& t,
                    json::string_view key) {
  auto const it = o.find(key);
  if (it != o.end()) {
    t = json::value_to<T>(it->value());
  }
}

template <class T>
void write_member(json::object& o,
                  std::optional<T> const& t,
                  json::string_view key) {
  if (t.has_value()) {
    o.emplace(key, json::value_from(t));
  }
}

template <class T>
void write_member(json::object& o, T const& t, json::string_view key) {
  o.emplace(key, json::value_from(t));
}

template <typename T>
concept Enum = std::is_scoped_enum_v<T>;

template <Enum T>
void parse(std::string_view s, T& x) {
  auto const v = json::value{s};
  x = json::value_to<T>(v);
}

inline void parse(std::string_view s, std::string_view& x) { x = s; }

}  // namespace openapi