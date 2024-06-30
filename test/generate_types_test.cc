#include "gtest/gtest.h"

#include <regex>

#include "yaml-cpp/yaml.h"

#include "cista/hash.h"

#include "boost/algorithm/string.hpp"
#include "boost/url/url_view.hpp"

#include "openapi/gen_types.h"
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
enum class mode {
  WALK,
  TRANSIT
};

void parse(std::string_view s, mode& x) {
  switch (cista::hash(s)) {
    case cista::hash("WALK"): x = mode::WALK; break;
    case cista::hash("TRANSIT"): x = mode::TRANSIT; break;
  }
}

struct sort_params {
  sort_params(boost::urls::url_view const& url) :
      mode_{::openapi::parse_param<std::vector<mode>>(url, "mode", std::vector<mode>{mode::WALK,mode::TRANSIT})},
      min_{::openapi::parse_param<int>(url, "min")}
  {}

  std::vector<mode> mode_;
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
enum class sort {
  asc,
  desc
};

void parse(std::string_view s, sort& x) {
  switch (cista::hash(s)) {
    case cista::hash("asc"): x = sort::asc; break;
    case cista::hash("desc"): x = sort::desc; break;
  }
}

struct sort_params {
  sort_params(boost::urls::url_view const& url) :
      sort_{::openapi::parse_param<sort>(url, "sort", sort::asc)},
      min_{::openapi::parse_param<int>(url, "min", 0)},
      needle_{::openapi::parse_param<std::string_view>(url, "needle", "needle")}
  {}

  sort sort_;
  int min_;
  std::string_view needle_;
};
)_";

  auto ss = std::stringstream{};
  ss << "\n";
  write_types(YAML::Load(kEnum), ss);
  EXPECT_EQ(kExpected, ss.str());
}

TEST(openapi, objects) {
  constexpr auto const kEnum = R"(
paths:
  /items:
    get:
      operationId: getItems
      responses:
        200:
          content:
            application/json:
              schema:
                type: array
                items:
                  $ref: '#/components/schemas/Item'
components:
  schemas:
    Status:
      type: string
      enum:
        - ON
        - OFF

    Pets:
      type: array
      items:
        type: string
        enum:
          - A
          - B

    Item:
      type: object
      required:
        - x
        - y
      properties:
        x:
          $ref: '#/components/schemas/Status'
        y:
          $ref: '#/components/schemas/Pets'
        z:
          type: integer
)";

  constexpr auto const kExpected = R"_(
)_";

  auto ss = std::stringstream{};
  ss << "\n";
  write_types(YAML::Load(kEnum), ss);
  EXPECT_EQ(kExpected, ss.str());
}
