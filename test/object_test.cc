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
using namespace pet;

TEST(openapi, roundtrip) {
  auto const val = Item{
      .x_ = StatusEnum::OFF, .y_ = Pets{PetsEnum::A, PetsEnum::B}, .z_ = 0};
  auto const json = json::serialize(json::value_from(val));
  auto const v = json::parse(json);
  auto const vx = json::value_to<Item>(v);
  EXPECT_EQ(val, vx);
}