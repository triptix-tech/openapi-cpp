#include "gtest/gtest.h"

#include "openapi/json.h"

using namespace openapi;

struct x {
  friend x tag_invoke(json::value_to_tag<x>, json::value const& jv) {
    x v;
    extract_member(jv.as_object(), v.a_, "a");
    extract_member(jv.as_object(), v.b_, "b");
    return v;
  }

  friend void tag_invoke(json::value_from_tag, json::value& jv, x const& v) {
    auto& o = (jv = json::object{}).as_object();
    write_member(o, v.a_, "a");
    write_member(o, v.b_, "b");
  }

  int a_{};
  std::optional<int> b_{};
};

TEST(json, optional_set) {
  auto const val = x{.a_ = 5, .b_ = 6};
  auto const json = json::serialize(json::value_from(val));
  auto const v = json::parse(json);
  auto const vx = json::value_to<x>(v);
  EXPECT_EQ(5, vx.a_);
  ASSERT_TRUE(vx.b_.has_value());
  EXPECT_EQ(6, *vx.b_);
}

TEST(json, optional_not_set) {
  auto const val = x{.a_ = 5};
  auto const json = json::serialize(json::value_from(val));
  auto const v = json::parse(json);
  auto const vx = json::value_to<x>(v);
  EXPECT_EQ(5, vx.a_);
  ASSERT_FALSE(vx.b_.has_value());
}

#include "cista/hash.h"

enum class Status { ON, OFF };

void parse(std::string_view s, Status& x) {
  switch (cista::hash(s)) {
    case cista::hash("ON"): x = Status::ON; break;
    case cista::hash("OFF"): x = Status::OFF; break;
  }
}

Status tag_invoke(json::value_to_tag<Status>, json::value const& jv) {
  auto x = Status{};
  parse(jv.as_string(), x);
  return x;
}

void tag_invoke(json::value_from_tag, json::value& jv, Status const v) {
  switch (v) {
    case Status::ON: jv = "ON"; break;
    case Status::OFF: jv = "OFF"; break;
  }
  std::unreachable();
}

enum class Pets_enum { A, B };

void parse(std::string_view s, Pets_enum& x) {
  switch (cista::hash(s)) {
    case cista::hash("A"): x = Pets_enum::A; break;
    case cista::hash("B"): x = Pets_enum::B; break;
  }
}

Pets_enum tag_invoke(json::value_to_tag<Pets_enum>, json::value const& jv) {
  auto x = Pets_enum{};
  parse(jv.as_string(), x);
  return x;
}

void tag_invoke(json::value_from_tag, json::value& jv, Pets_enum const v) {
  switch (v) {
    case Pets_enum::A: jv = "A"; break;
    case Pets_enum::B: jv = "B"; break;
  }
  std::unreachable();
}

using Pets = std::vector<Pets_enum>;

struct Item {
  friend Item tag_invoke(boost::json::value_to_tag<Item>,
                         boost::json::value const& jv) {
    auto v = Item{};
    extract_member(jv.as_object(), v.x, "x");
    extract_member(jv.as_object(), v.y, "y");
    extract_member(jv.as_object(), v.z, "z");
    return v;
  }

  friend void tag_invoke(json::value_from_tag, json::value& jv, Item const& v) {
    auto& o = (jv = json::object{}).as_object();
    write_member(o, v.x, "x");
    write_member(o, v.y, "y");
    write_member(o, v.z, "z");
  }

  Status x;
  Pets y;
  std::optional<int> z;
};