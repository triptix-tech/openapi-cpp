#include "gtest/gtest.h"

#include "yaml-cpp/yaml.h"

#include "cista/hash.h"

#include "boost/json.hpp"
#include "boost/url.hpp"

#include "utl/verify.h"

#include "openapi/gen_types.h"
#include "openapi/json.h"
#include "openapi/parse.h"

#include "pet-api/pet-api.h"

using namespace openapi;
using namespace pet;

TEST(openapi, roundtrip) {
  auto const val = Item{
      .x_ = StatusEnum::OFF, .y_ = Pets{PetsEnum::A, PetsEnum::B}, .z_ = 0};
  auto const json = json::serialize(json::value_from(val));
  auto const v = json::parse(json);
  auto const vx = json::value_to<Item>(v);
  EXPECT_EQ(val, vx);
}

TEST(openapi, param_enum) {
  auto const v = parse_param<std::optional<StatusEnum>>(
      boost::urls::url_view{"/items?status=OFF"}.params(), "status");
  ASSERT_TRUE(v.has_value());
  EXPECT_EQ(StatusEnum::OFF, *v);
}

TEST(openapi, param_enum_unknown_value) {
  try {
    parse_param<std::optional<StatusEnum>>(
        boost::urls::url_view{"/items?status=OF"}.params(), "status");
    FAIL() << "expected bad_request_exception";
  } catch (bad_request_exception const& e) {
    EXPECT_EQ(R"(parameter "status": invalid value "OF" for enum StatusEnum, )"
              R"(expected one of: ON, OFF)",
              std::string_view{e.what()});
  }
}

// Enums are also converted from JSON for responses and internal decoding,
// where an unknown value is not the client's fault.
TEST(openapi, json_enum_unknown_value_is_no_bad_request) {
  auto const jv = json::parse(R"({"x": "OF", "y": ["A"]})");
  try {
    json::value_to<Item>(jv);
    FAIL() << "expected exception";
  } catch (bad_request_exception const&) {
    FAIL() << "enum error from JSON must not be a bad request";
  } catch (std::exception const& e) {
    EXPECT_EQ(
        R"(invalid value "OF" for enum StatusEnum, expected one of: ON, OFF)",
        std::string_view{e.what()});
  }
}
