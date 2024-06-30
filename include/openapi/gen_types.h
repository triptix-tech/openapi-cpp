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
    case cista::hash("object"): return type::kObject;
    default: throw utl::fail("invalid type {}", s);
  }
}

std::string_view to_cpp(type const t) {
  switch (t) {
    case type::kInteger: return "int";
    case type::kNumber: return "double";
    case type::kString: return "std::string_view";
    case type::kBoolean: return "bool";
    case type::kArray: return "std::vector";
    case type::kObject: return "object";
    default: std::unreachable();
  }
}

struct indent {
  explicit indent(int indent, char separator = ',')
      : indent_{indent}, separator_{separator} {}

  void operator()(std::ostream& out) {
    if (!first_ && separator_ != '\0') {
      out << separator_;
    }
    first_ = false;

    if (indent_ != -1) {
      out << '\n';
      for (auto i = 0; i != indent_; ++i) {
        out << "  ";
      }
    }
  }

  bool first_{true};
  char separator_{};
  int indent_;
};

bool gen_enum(std::string_view name,
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
    return true;
  }
  return false;
}

std::string get_type(std::string_view name,
                     YAML::Node const& schema,
                     bool const required = true) {
  auto const ref = schema["$ref"];
  if (ref.IsDefined()) {
    auto const prefix = std::string_view{"#/components/schemas/"};
    auto const type = ref.as<std::string_view>().substr(prefix.size());
    return std::string{type};
  }

  auto const type = to_type(schema["type"].as<std::string_view>());
  auto const enumera = schema["enum"];
  auto const t = std::string{enumera.IsDefined() ? name : to_cpp(type)};
  auto const items = schema["items"];
  auto const has_default = schema["default"].IsDefined();
  auto const x = items.IsDefined() ? t + '<' + get_type(name, items) + '>' : t;
  return required || has_default ? x : std::string{"std::optional<"} + x + ">";
}

bool is_required(YAML::Node const& n) {
  auto const required = n["required"];
  return required.IsDefined() && required.as<bool>();
}

void gen_member(std::string_view name,
                bool required,
                YAML::Node const& schema,
                std::ostream& out) {
  out << "  " << get_type(name, schema, required) << " " << name << "_;\n";
}

void gen_value(std::string_view name,
               YAML::Node const& schema,
               YAML::Node const& default_value,
               std::ostream& out) {
  auto const type = to_type(schema["type"].as<std::string_view>());
  auto const enumera = schema["enum"];
  if (enumera) {
    out << name << "::" << default_value;
    return;
  }
  switch (type) {
    case type::kArray: {
      auto const item_schema = schema["items"];
      out << get_type(name, schema) << "{";
      auto ind = indent{-1, ','};
      for (auto const& v : default_value) {
        ind(out);
        gen_value(name, item_schema, v, out);
      }
      out << "}";
    } break;
    case type::kString: out << '"' << default_value << '"'; break;
    default: out << default_value;
  }
}

void gen_member_init(YAML::Node const& x, std::ostream& out) {
  auto const schema = x["schema"];
  auto const name = x["name"].as<std::string_view>();
  auto const type = get_type(name, schema);
  out << "  " << name << "_{::openapi::parse_param<" << type << ">(url, \""
      << name << "\"";
  auto const default_value = schema["default"];
  if (default_value.IsDefined()) {
    out << ", ";
    gen_value(name, schema, default_value, out);
  }
  out << ")}";
}

void write_params(YAML::Node const& n, std::ostream& out) {
  for (auto const& p : n["parameters"]) {
    auto const name = p["name"].as<std::string_view>();
    auto const items = p["schema"]["items"];
    if (items.IsDefined()) {
      gen_enum(name, items, out);
    } else {
      gen_enum(name, p["schema"], out);
    }
  }

  out << "struct " << n["operationId"] << "_params {\n";
  out << "  explicit " << n["operationId"]
      << "_params(boost::urls::url_view const& url)";

  auto const parameters = n["parameters"];
  if (parameters.IsDefined() && parameters.size() != 0) {
    out << " :";
    auto ind = indent{2};
    for (auto const& p : parameters) {
      ind(out);
      gen_member_init(p, out);
    }
  }
  out << "\n  {}\n\n";
  for (auto const& p : n["parameters"]) {
    auto const name = p["name"].as<std::string_view>();
    gen_member(name, is_required(p), p["schema"], out);
  }
  out << "};\n";
}

void write_types(YAML::Node const& n, std::ostream& out) {
  for (auto const& path : n["paths"]) {
    for (auto const& method : path.second) {
      write_params(method.second, out);
    }
  }

  auto const components = n["components"];
  if (components.IsDefined()) {
    for (auto const& c : components["schemas"]) {
      auto const& schema = c.second;
      auto const& name = c.first.as<std::string_view>();
      auto const type = to_type(schema["type"].as<std::string_view>());
      auto const required = schema["required"];
      auto const is_required = [&](std::string_view name) {
        if (!required.IsDefined()) {
          return false;
        }
        for (auto const& x : required) {
          if (x.as<std::string_view>() == name) {
            return true;
          }
        }
        return false;
      };

      if (gen_enum(name, schema, out)) {
        continue;
      }

      switch (type) {
        case type::kObject:
          out << "struct " << name << " {\n";
          for (auto const& p : schema["properties"]) {
            auto const member_name = p.first.as<std::string_view>();
            gen_member(member_name, is_required(member_name), p.second, out);
          }
          out << "};\n\n";
          break;

        case type::kArray:
          gen_enum(std::string{name} + "_enum", schema["items"], out);
          [[fallthrough]];

        default:
          out << "using " << name << " = " << get_type(name, schema) << ";\n\n";
          break;
      }
    }
  }
}

}  // namespace openapi