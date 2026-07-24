#pragma once

#include <sstream>
#include <string_view>
#include <vector>

#include "boost/url/params_view.hpp"
#include "boost/url/segments_view.hpp"

#include "utl/parser/arg_parser.h"
#include "utl/verify.h"

#include "openapi/bad_request_exception.h"
#include "openapi/date_time.h"

namespace openapi {

template <typename T>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};

template <typename T>
constexpr inline auto const is_optional_v = is_optional<T>::value;

template <class T>
concept Primitive =
    std::is_same_v<T, std::int64_t> || std::is_same_v<T, double> ||
    std::is_same_v<T, bool> || std::is_same_v<T, std::string>;

template <Primitive T>
void parse(std::string_view s, T& v) {
  auto cs = utl::cstr{s};
  utl::parse_arg(cs, v);
}

template <typename T>
void parse(std::string_view s, std::vector<T>& v) {
  utl::for_each_token(
      s, ',', [&](auto&& token) { parse(token.view(), v.emplace_back()); });
}

template <typename T>
void parse(std::string_view s, std::optional<T>& v) {
  auto x = T{};
  parse(s, x);
  v = x;
}

template <typename T>
T parse_param(boost::urls::params_view const& params,
              std::string_view name,
              std::optional<T> const& default_value = std::nullopt,
              bool const allow_missing = false) {
  auto const it = params.find(name);
  if (it != params.end()) {
    auto v = T{};
    parse((*it).value, v);
    return v;
  } else {
    if constexpr (!is_optional_v<T>) {
      if (!default_value.has_value() && !allow_missing) {
        throw bad_request_exception{fmt::format("missing parameter: {}", name)};
      }
    }
  }
  return default_value.has_value() ? T{*default_value} : T{};
}

template <typename T>
T parse_segment(boost::urls::segments_view const& segs,
                std::string_view name,
                std::size_t idx) {
  if (idx < segs.size()) {
    auto it = segs.begin();
    std::advance(it, idx);
    auto v = T{};
    parse(*it, v);
    return v;
  }
  throw bad_request_exception{
      fmt::format("missing segment parameter: {}", name)};
}

}  // namespace openapi
