#pragma once

#include <string_view>
#include <vector>

#include "utl/parser/arg_parser.h"

namespace openapi {

template <typename T>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};

template <typename T>
constexpr auto const is_optional_v = is_optional<T>::value;

template <class T>
concept Primitive = std::is_same_v<T, std::int64_t> ||
                    std::is_same_v<T, double> || std::is_same_v<T, bool>;

template <Primitive T>
void parse(std::string_view s, T& v) {
  auto cs = utl::cstr{s};
  utl::parse_arg(cs, v);
}

template <typename T>
void parse(std::string_view s, std::vector<T>& v) {
  utl::for_each_token(s, ',',
                      [&](auto&& s) { parse(s.view(), v.emplace_back()); });
}

template <typename T>
void parse(std::string_view s, std::optional<T>& v) {
  auto x = T{};
  parse(s, x);
  v = x;
}

template <typename T>
T parse_param(boost::urls::url_view const& url,
              std::string_view name,
              std::optional<T> const& default_value = std::nullopt) {
  auto const params = url.params();
  auto const it = params.find(name);
  if (it != params.end()) {
    auto v = T{};
    parse((*it).value, v);
    return v;
  } else {
    if constexpr (!is_optional_v<T>) {
      if (!default_value.has_value()) {
        throw utl::fail("required parameter {} not set", name);
      }
    }
  }
  return default_value.has_value() ? T{*default_value} : T{};
}

}  // namespace openapi