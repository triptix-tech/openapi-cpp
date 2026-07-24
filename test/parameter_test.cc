#include "gtest/gtest.h"

#include "boost/json.hpp"
#include "boost/url.hpp"

#include "openapi/bad_request_exception.h"
#include "openapi/gen_types.h"

#include "pet-api/pet-api.h"

using namespace openapi;
using namespace pet;

TEST(openapi, valid_query) {
  auto url = boost::urls::url_view{"/items/test/42?param3=10"};
  auto p = pet::getItems_params{url.params(), url.segments()};
  EXPECT_EQ("test", p.param1_);
  EXPECT_EQ(42, p.param2_);
  EXPECT_EQ(10, p.param3_);
}

TEST(openapi, path_params_always_required) {
  auto url = boost::urls::url_view{"/items?param3=10"};
  EXPECT_THROW(pet::getItems_params(url.params(), url.segments()),
               openapi::bad_request_exception);
}
