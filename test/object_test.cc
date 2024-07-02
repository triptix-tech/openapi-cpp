#include "gtest/gtest.h"

#include "yaml-cpp/yaml.h"

#include "cista/hash.h"

#include "boost/json.hpp"
#include "boost/url.hpp"

#include "utl/verify.h"

#include "openapi/gen_types.h"
#include "openapi/json.h"

using namespace openapi;

struct getItems_params {
  explicit getItems_params(boost::urls::url_view const& url) {}
};
enum class Status_enum { ON, OFF };

inline Status_enum tag_invoke(boost::json::value_to_tag<Status_enum>,
                              boost::json::value const& jv) {
  auto x = Status_enum{};
  auto const sv = std::string_view{jv.as_string()};
  switch (cista::hash(sv)) {
    case cista::hash("ON"): x = Status_enum::ON; break;
    case cista::hash("OFF"): x = Status_enum::OFF; break;
    default: throw utl::fail("enum Status_enum: unknown value {}", sv);
  }
  return x;
}

inline void tag_invoke(json::value_from_tag,
                       json::value& jv,
                       Status_enum const v) {
  switch (v) {
    case Status_enum::ON: jv = "ON"; break;
    case Status_enum::OFF: jv = "OFF"; break;
    default: std::unreachable();
  }
}

enum class Pets_enum { A, B };

inline Pets_enum tag_invoke(boost::json::value_to_tag<Pets_enum>,
                            boost::json::value const& jv) {
  auto x = Pets_enum{};
  auto const sv = std::string_view{jv.as_string()};
  switch (cista::hash(sv)) {
    case cista::hash("A"): x = Pets_enum::A; break;
    case cista::hash("B"): x = Pets_enum::B; break;
    default: throw utl::fail("enum Pets_enum: unknown value {}", sv);
  }
  return x;
}

inline void tag_invoke(json::value_from_tag,
                       json::value& jv,
                       Pets_enum const v) {
  switch (v) {
    case Pets_enum::A: jv = "A"; break;
    case Pets_enum::B: jv = "B"; break;
  }
  std::unreachable();
}

using Pets = std::vector<Pets_enum>;

struct Item {
  friend bool operator==(Item const&, Item const&) = default;

  friend Item tag_invoke(boost::json::value_to_tag<Item>,
                         boost::json::value const& jv) {
    auto v = Item{};
    extract_member(jv.as_object(), v.x_, "x");
    extract_member(jv.as_object(), v.y_, "y");
    extract_member(jv.as_object(), v.z_, "z");
    return v;
  }

  friend void tag_invoke(boost::json::value_from_tag,
                         boost::json::value& jv,
                         Item const& v) {
    auto& o = (jv = json::object{}).as_object();
    write_member(o, v.x_, "x");
    write_member(o, v.y_, "y");
    write_member(o, v.z_, "z");
  }

  Status_enum x_;
  Pets y_;
  std::optional<int> z_;
};

TEST(openapi, roundtrip) {
  auto const val = Item{
      .x_ = Status_enum::OFF, .y_ = Pets{Pets_enum::A, Pets_enum::B}, .z_ = 0};
  auto const json = json::serialize(json::value_from(val));
  auto const v = json::parse(json);
  auto const vx = json::value_to<Item>(v);
  EXPECT_EQ(val, vx);
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
struct getItems_params {
  explicit getItems_params(boost::urls::url_view const& url)
  {}

};
enum class Status_enum {
  ON,
  OFF
};

inline Status_enum tag_invoke(boost::json::value_to_tag<Status_enum>, boost::json::value const& jv) {
  auto x = Status_enum{};
  auto const sv = std::string_view{jv.as_string()};
  switch (cista::hash(sv)) {
    case cista::hash("ON"): x = Status_enum::ON; break;
    case cista::hash("OFF"): x = Status_enum::OFF; break;
    default: throw utl::fail("enum Status_enum: unknown value {}", sv);
  }
  return x;
}

inline void tag_invoke(json::value_from_tag, json::value& jv, Status_enum const v) {
  switch (v) {
    case Status_enum::ON: jv = "ON"; break;
    case Status_enum::OFF: jv = "OFF"; break;
    default: std::unreachable();
  }
}

enum class Pets_enum {
  A,
  B
};

inline Pets_enum tag_invoke(boost::json::value_to_tag<Pets_enum>, boost::json::value const& jv) {
  auto x = Pets_enum{};
  auto const sv = std::string_view{jv.as_string()};
  switch (cista::hash(sv)) {
    case cista::hash("A"): x = Pets_enum::A; break;
    case cista::hash("B"): x = Pets_enum::B; break;
    default: throw utl::fail("enum Pets_enum: unknown value {}", sv);
  }
  return x;
}

inline void tag_invoke(json::value_from_tag, json::value& jv, Pets_enum const v) {
  switch (v) {
    case Pets_enum::A: jv = "A"; break;
    case Pets_enum::B: jv = "B"; break;
    default: std::unreachable();
  }
}

using Pets = std::vector<Pets_enum>;

struct Item {
  friend Item tag_invoke(boost::json::value_to_tag<Item>, boost::json::value const& jv) {
    auto v = Item{};
    extract_member(jv.as_object(), v.x_, "x");
    extract_member(jv.as_object(), v.y_, "y");
    extract_member(jv.as_object(), v.z_, "z");
    return v;
  }

  friend void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Item const& v) {
    auto& o = (jv = json::object{}).as_object();
    write_member(o, v.x_, "x");
    write_member(o, v.y_, "y");
    write_member(o, v.z_, "z");
  }

  Status_enum x_;
  Pets y_;
  std::optional<int> z_;
};

)_";

  auto ss = std::stringstream{};
  ss << "\n";
  write_types(YAML::Load(kEnum), ss);
  EXPECT_EQ(kExpected, ss.str());
}
