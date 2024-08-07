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
            url, "mode", std::vector<mode>{mode::WALK, mode::TRANSIT})},
        min_{::openapi::parse_param<int>(url, "min")} {}

  std::vector<mode> mode_;
  int min_;
};

TEST(openapi, parse_int_vector) {
  auto v = std::vector<int>{};
  parse("1,2,3", v);
  EXPECT_EQ((std::vector{1, 2, 3}), v);
}

TEST(openapi, parse_enum_vector) {
  auto const v = parse_param<std::optional<std::vector<mode>>>(
      "/?mode=TRANSIT,WALK", "mode");
  EXPECT_EQ((std::vector{mode::TRANSIT, mode::WALK}), v);
}

TEST(openapi, parse_expect_optional) {
  auto const v = parse_param<std::optional<std::vector<mode>>>("/", "mode");
  EXPECT_FALSE(v.has_value());
}

TEST(openapi, parse_expect_throw) {
  EXPECT_THROW(parse_param<std::vector<mode>>("/", "mode"), std::runtime_error);
}

TEST(openapi, parse_expect_default_value) {
  auto const v = parse_param<std::vector<mode>>(
      "/", "mode", std::vector<mode>{mode::TRANSIT, mode::WALK});
  EXPECT_EQ((std::vector{mode::TRANSIT, mode::WALK}), v);
}

TEST(openapi, array_param) {
  constexpr auto const kEnumArray = R"(
paths:
  /items:
    get:
      operationId: sort
      parameters:
        - name: mode
          in: query
          required: false
          schema:
            default: [WALK, TRANSIT]
            type: array
            items:
              type: string
              minItems: 1
              enum:
                - WALK
                - TRANSIT
          explode: false

        - in: query
          name: min
          required: true
          schema:
            type: integer

)";

  constexpr auto const kExpected = R"(
enum class mode_enum {
  WALK,
  TRANSIT
};

inline mode_enum tag_invoke(boost::json::value_to_tag<mode_enum>, boost::json::value const& jv) {
  auto x = mode_enum{};
  auto const sv = std::string_view{jv.as_string()};
  switch (cista::hash(sv)) {
    case cista::hash("WALK"): x = mode_enum::WALK; break;
    case cista::hash("TRANSIT"): x = mode_enum::TRANSIT; break;
    default: throw utl::fail("enum mode_enum: unknown value {}", sv);
  }
  return x;
}

inline void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, mode_enum const v) {
  switch (v) {
    case mode_enum::WALK: jv = "WALK"; break;
    case mode_enum::TRANSIT: jv = "TRANSIT"; break;
    default: std::unreachable();
  }
}

struct sort_params {
  explicit sort_params(boost::urls::url_view const& url) :
      mode_{::openapi::parse_param<std::vector<mode_enum>>(url, "mode", std::vector<mode_enum>{mode::WALK,mode::TRANSIT})},
      min_{::openapi::parse_param<int>(url, "min")}
  {}

  std::vector<mode_enum> mode_;
  int min_;
};
)";

  auto ss = std::stringstream{};
  ss << "\n";
  write_types(YAML::Load(kEnumArray), ss);
  EXPECT_EQ(kExpected, ss.str());
}

TEST(openapi, enum_param) {
  constexpr auto const kEnum = R"(
paths:
  /items:
    get:
      operationId: sort
      parameters:
        - in: query
          name: sort
          description: Sort order
          schema:
            type: string
            enum: [asc, desc]
            default: asc

        - in: query
          name: min
          schema:
            type: integer
            default: 0

        - in: query
          name: needle
          schema:
            type: string
            default: "needle"
)";

  constexpr auto const kExpected = R"_(
enum class sort_enum {
  asc,
  desc
};

inline sort_enum tag_invoke(boost::json::value_to_tag<sort_enum>, boost::json::value const& jv) {
  auto x = sort_enum{};
  auto const sv = std::string_view{jv.as_string()};
  switch (cista::hash(sv)) {
    case cista::hash("asc"): x = sort_enum::asc; break;
    case cista::hash("desc"): x = sort_enum::desc; break;
    default: throw utl::fail("enum sort_enum: unknown value {}", sv);
  }
  return x;
}

inline void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, sort_enum const v) {
  switch (v) {
    case sort_enum::asc: jv = "asc"; break;
    case sort_enum::desc: jv = "desc"; break;
    default: std::unreachable();
  }
}

struct sort_params {
  explicit sort_params(boost::urls::url_view const& url) :
      sort_{::openapi::parse_param<sort_enum>(url, "sort", sort::asc)},
      min_{::openapi::parse_param<int>(url, "min", 0)},
      needle_{::openapi::parse_param<std::string_view>(url, "needle", "needle")}
  {}

  sort_enum sort_;
  int min_;
  std::string_view needle_;
};
)_";

  auto ss = std::stringstream{};
  ss << "\n";
  write_types(YAML::Load(kEnum), ss);
  EXPECT_EQ(kExpected, ss.str());
}
