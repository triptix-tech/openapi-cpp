#pragma once

#include <ostream>
#include <string_view>

#include "utl/verify.h"

#include "yaml-cpp/yaml.h"

#include "cista/hash.h"

#include "utl/to_vec.h"

namespace openapi {

enum class type { kBoolean, kInteger, kNumber, kString, kArray, kObject };

type to_type(std::string_view s) {
  switch (cista::hash(s)) {
    case cista::hash("integer"): return type::kInteger;
    case cista::hash("number"): return type::kNumber;
    case cista::hash("string"): return type::kString;
    case cista::hash("array"): return type::kArray;
    case cista::hash("boolean"): return type::kBoolean;
    default: std::unreachable();
  }
}

std::string_view to_cpp(type const t) {
  switch (t) {
    case type::kInteger: return "int";
    case type::kNumber: return "double";
    case type::kString: return "std::string";
    case type::kBoolean: return "bool";
    case type::kArray: return "std::vector";
    default: std::unreachable();
  }
}

struct indent {
  indent(unsigned indent) : indent_{indent} {}

  void operator()(std::ostream& out) {
    if (!first_) {
      out << ',';
      first_ = false;
    }

    out << '\n';

    for (auto i = 0U; i != indent_; ++i) {
      out << "  ";
    }
  }

  bool first_{true};
  unsigned indent_;
};

void gen_member(YAML::Node const& x, std::ostream& out) {
  auto const schema = x["schema"];
  auto const name = x["name"].as<std::string_view>();
  auto const type = to_type(x["type"].as<std::string_view>());

  auto const enumera = x["enum"];
  if (enumera.IsDefined()) {
    auto ind = indent{1};
    out << "  enum class " << name << " {\n";
    for (auto const& e : enumera) {
      ind(out);
      out << e;
    }
  }

  switch (type) {
    case type::kArray:
    case type::kObject: throw utl::fail("not supported");
    default: out << "  " << to_cpp(type) << " " << name << "_;";
  }
}

void write_params(YAML::Node const& n, std::ostream& out) {
  for (auto const& p : n["parameters"]) {
    gen_member(p, out);
  }
}

}  // namespace openapi