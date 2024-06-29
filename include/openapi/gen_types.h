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
    case type::kString: return "std::string_view";
    case type::kBoolean: return "bool";
    case type::kArray: return "std::vector";
    default: std::unreachable();
  }
}

struct indent {
  explicit indent(unsigned indent, char separator = ',')
      : indent_{indent}, separator_{separator} {}

  void operator()(std::ostream& out) {
    if (!first_ && separator_ != '\0') {
      out << separator_;
    }
    first_ = false;

    out << '\n';

    for (auto i = 0U; i != indent_; ++i) {
      out << "  ";
    }
  }

  bool first_{true};
  char separator_{};
  unsigned indent_;
};

void gen_types(std::string_view name,
               YAML::Node const& schema,
               std::ostream& out) {
  auto const enumera = schema["enum"];
  auto const type = to_type(schema["type"].as<std::string_view>());
  if (enumera.IsDefined()) {
    {
      out << "enum class " << name << " {";
      auto ind = indent{1};
      for (auto const& e : enumera) {
        ind(out);
        out << e;
      }
      out << "\n};\n\n";
    }
    {
      out << "void parse(" << to_cpp(type) << " s" << ", " << name
          << "& x) {\n";
      out << "  switch (cista::hash(s)) {";
      auto ind = indent{2, 0};
      for (auto const& e : enumera) {
        ind(out);
        out << "case cista::hash(\"" << e << "\"): x = " << name << "::" << e
            << "; break;";
      }
      out << "\n  }\n";
      out << "}\n\n";
    }
  }
}

std::string get_type(std::string_view name, YAML::Node const& schema) {
  auto const type = to_type(schema["type"].as<std::string_view>());
  auto const enumera = schema["enum"];
  auto const t = std::string{enumera.IsDefined() ? name : to_cpp(type)};
  auto const items = schema["items"];
  if (items.IsDefined()) {
    return t + '<' + get_type(name, items) + '>';
  }
  return t;
}

void gen_member(YAML::Node const& x, std::ostream& out) {
  auto const schema = x["schema"];
  auto const name = x["name"].as<std::string_view>();
  auto const type = to_type(schema["type"].as<std::string_view>());

  auto const enumera = schema["enum"];

  switch (type) {
    case type::kObject: throw utl::fail("not supported");
    case type::kArray:
    default: out << "  " << get_type(name, schema) << " " << name << "_;\n";
  }
}

void gen_member_init(YAML::Node const& x, std::ostream& out) {
  auto const schema = x["schema"];
  auto const name = x["name"].as<std::string_view>();
  out << "  " << name << "_{::openapi::parse_param<" << get_type(name, schema)
      << ">(url, \"" << name << "\")}";
}

void write_params(YAML::Node const& n, std::ostream& out) {
  for (auto const& p : n["parameters"]) {
    auto const name = p["name"].as<std::string_view>();
    auto const items = p["schema"]["items"];
    if (items.IsDefined()) {
      gen_types(name, items, out);
    } else {
      gen_types(name, p["schema"], out);
    }
  }

  out << "struct " << n["operationId"] << "_params {\n";
  out << "  " << n["operationId"]
      << "_params(boost::urls::url_view const& url) :";
  auto ind = indent{2};
  for (auto const& p : n["parameters"]) {
    ind(out);
    gen_member_init(p, out);
  }
  out << "\n  {}\n\n";
  for (auto const& p : n["parameters"]) {
    gen_member(p, out);
  }
  out << "};\n";
}

}  // namespace openapi