#include "gtest/gtest.h"

#include <regex>

#include "yaml-cpp/yaml.h"

#include "cista/hash.h"

#include "boost/algorithm/string.hpp"
#include "boost/url/url_view.hpp"

#include "openapi/gen_types.h"
#include "openapi/json.h"
#include "openapi/parse.h"

using namespace openapi;

enum class mode { WALK, TRANSIT };

void parse(std::string_view s, mode& x) {
  switch (cista::hash(s)) {
    case cista::hash("WALK"): x = mode::WALK; break;
    case cista::hash("TRANSIT"): x = mode::TRANSIT; break;
  }
}

struct sort_params {
  sort_params(boost::urls::url_view const& url)
      : mode_{::openapi::parse_param<std::vector<mode>>(
            url.params(),
            "mode",
            std::vector<mode>{mode::WALK, mode::TRANSIT})},
        min_{::openapi::parse_param<std::int64_t>(url.params(), "min")} {}

  std::vector<mode> mode_;
  std::int64_t min_;
};

TEST(openapi, parse_int_vector) {
  auto v = std::vector<std::int64_t>{};
  parse("1,2,3", v);
  EXPECT_EQ((std::vector<std::int64_t>{1, 2, 3}), v);
}

TEST(openapi, parse_enum_vector) {
  auto const v = parse_param<std::optional<std::vector<mode>>>(
      boost::urls::url_view{"/?mode=TRANSIT,WALK"}.params(), "mode");
  EXPECT_EQ((std::vector{mode::TRANSIT, mode::WALK}), v);
}

TEST(openapi, parse_expect_optional) {
  auto const v = parse_param<std::optional<std::vector<mode>>>(
      boost::urls::url_view{"/"}.params(), "mode");
  EXPECT_FALSE(v.has_value());
}

TEST(openapi, parse_expect_throw) {
  EXPECT_THROW(parse_param<std::vector<mode>>(
                   boost::urls::url_view{"/"}.params(), "mode"),
               std::runtime_error);
}

TEST(openapi, parse_expect_default_value) {
  auto const v = parse_param<std::vector<mode>>(
      boost::urls::url_view{"/"}.params(), "mode",
      std::vector<mode>{mode::TRANSIT, mode::WALK});
  EXPECT_EQ((std::vector{mode::TRANSIT, mode::WALK}), v);
}