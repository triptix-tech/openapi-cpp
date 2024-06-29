#pragma once

#include <string_view>
#include <vector>

#include "utl/parser/arg_parser.h"

namespace openapi {

template <class T>
concept Primitive = std::is_same_v<T, int> || std::is_same_v<T, double> ||
                    std::is_same_v<T, bool>;

std::string_view get_param(boost::urls::url_view const& url,
                           std::string_view name) {
  auto const params = url.params();
  auto const it = params.find(name);
  utl::verify(it != params.end(), "parameter {} not found", name);
  return (*it).value;
}

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
T parse_param(boost::urls::url_view const& url, std::string_view name) {
  auto v = T{};
  parse(get_param(url, name), v);
  return v;
}

}  // namespace openapi