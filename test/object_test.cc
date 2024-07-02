#include "gtest/gtest.h"

#include "yaml-cpp/yaml.h"

#include "cista/hash.h"

#include "boost/json.hpp"
#include "boost/url.hpp"

#include "utl/verify.h"

#include "openapi/gen_types.h"
#include "openapi/json.h"

#include "pet-api/pet-api.h"

using namespace openapi;

TEST(openapi, roundtrip) {
  auto const val = Item{
      .x_ = Status_enum::OFF, .y_ = Pets{Pets_enum::A, Pets_enum::B}, .z_ = 0};
  auto const json = json::serialize(json::value_from(val));
  auto const v = json::parse(json);
  auto const vx = json::value_to<Item>(v);
  EXPECT_EQ(val, vx);
}