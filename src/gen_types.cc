#include "openapi/gen_types.h"

#include <optional>
#include <ostream>

namespace openapi {

void write_prelude(std::string_view path_to_header,
                   std::ostream& header,
                   std::ostream& source,
                   std::optional<std::string_view> ns) {
  header << R"(#pragma once

#include <optional>
#include <string_view>
#include <map>
#include <string>

#include "boost/url.hpp"
#include "boost/json/fwd.hpp"

#include "openapi/date_time.h"
)";

  source << R"(#include ")" << path_to_header << "\"\n";
  source << R"(
#include "cista/hash.h"

#include "boost/json.hpp"

#include "utl/verify.h"

#include "openapi/json.h"
#include "openapi/parse.h"

)";

  if (ns.has_value()) {
    header << "namespace " << *ns << " {\n\n";
    source << "namespace " << *ns << " {\n\n";
  }
}

void write_postlude(std::ostream& header,
                    std::ostream& source,
                    std::optional<std::string_view> ns) {
  if (ns.has_value()) {
    header << "\n}  // namespace " << *ns << "\n";
    source << "\n}  // namespace " << *ns << "\n";
  }
}

type to_type(YAML::Node const& schema) {
  auto const s = schema["type"].as<std::string_view>();
  auto const format_node = schema["format"];
  auto const format =
      format_node.IsDefined() ? format_node.as<std::string_view>() : "";
  switch (cista::hash(s)) {
    case cista::hash("date-time"):;
    case cista::hash("integer"): return type::kInteger;
    case cista::hash("number"): return type::kNumber;
    case cista::hash("string"):
      if (format.empty()) {
        return type::kString;
      } else if (format == "date-time") {
        return type::kDate;
      } else {
        throw utl::fail("unknown format {}", format);
      }
    case cista::hash("array"): return type::kArray;
    case cista::hash("boolean"): return type::kBoolean;
    case cista::hash("object"): return type::kObject;
    default: throw utl::fail("invalid type {}", s);
  }
}

std::string_view to_cpp(type const t) {
  switch (t) {
    case type::kDate: return "openapi::date_time_t";
    case type::kInteger: return "std::int64_t";
    case type::kNumber: return "double";
    case type::kString: return "std::string";
    case type::kBoolean: return "bool";
    case type::kArray: return "std::vector";
    case type::kObject: return "std::map<std::string, std::uint64_t>";
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

bool gen_enum(std::string_view type_name,
              YAML::Node const& schema,
              std::ostream& header,
              std::ostream& source) {
  if (schema["$ref"].IsDefined()) {
    return false;
  }

  auto const name = std::string{type_name} + "Enum";
  auto const enumera = schema["enum"];
  if (enumera.IsDefined()) {
    {
      header << "enum class " << name << " {";
      auto ind = indent{1};
      for (auto const& e : enumera) {
        ind(header);
        header << e;
      }
      header << "\n};\n\n";
    }

    {
      header << name << " tag_invoke(boost::json::value_to_tag<" << name
             << ">, boost::json::value const&);\n";

      source << name << " tag_invoke(boost::json::value_to_tag<" << name
             << ">, boost::json::value const& jv) {\n";
      source << "  auto x = " << name << "{};\n";
      source << "  auto const sv = std::string_view{jv.as_string()};\n";
      source << "  switch (cista::hash(sv)) {";
      auto ind = indent{2, 0};
      for (auto const& e : enumera) {
        ind(source);
        source << "case cista::hash(\"" << e << "\"): x = " << name << "::" << e
               << "; break;";
      }
      ind(source);
      source << "default: throw utl::fail(\"enum " << name
             << ": unknown value {}\", sv);\n";
      source << "  }\n";
      source << "  return x;\n";
      source << "}\n\n";
    }

    {
      header << "std::ostream& operator<<(std::ostream&, " << name << ");\n\n";
      header << "void tag_invoke(boost::json::value_from_tag, "
                "boost::json::value&, "
             << name << ");\n";

      source << "std::ostream& operator<<(std::ostream& out, " << name
             << " x) {\n"
             << "  return out << "
                "boost::json::serialize(boost::json::value_from(x));\n"
             << "}\n\n";
      source << "void tag_invoke(boost::json::value_from_tag, "
                "boost::json::value& jv, "
             << name
             << " const v) {\n"
                "  switch (v) {";
      auto ind = indent{2, 0};
      for (auto const& e : enumera) {
        ind(source);
        source << "case " << name << "::" << e << ": jv = \"" << e
               << "\"; return;";
      }
      ind(source);
      source << "}\n";
      source << "  throw utl::fail(\"invalid " << name
             << " value {}\", static_cast<int>(v));\n"
             << "}\n\n";
    }
    return true;
  }
  return false;
}

std::string_view ref_name(YAML::Node const& ref) {
  auto const prefix = std::string_view{"#/components/schemas/"};
  auto const type = ref.as<std::string_view>().substr(prefix.size());
  return type;
}

YAML::Node resolve_schema(YAML::Node const& root, YAML::Node const& schema) {
  auto const ref = schema["$ref"];
  return ref.IsDefined() ? root["components"]["schemas"][ref_name(ref)]
                         : schema;
}

std::string get_type(YAML::Node const& root,
                     std::string_view name,
                     YAML::Node const& schema,
                     bool const required) {
  auto const ref = schema["$ref"];
  if (ref.IsDefined()) {
    auto const enum_postfix =
        resolve_schema(root, schema)["enum"].IsDefined() ? "Enum" : "";
    auto const x = std::string{ref_name(ref)} + enum_postfix;
    auto const has_default = schema["default"].IsDefined();
    return required || has_default ? x
                                   : std::string{"std::optional<"} + x + ">";
  }

  auto const type = to_type(schema);
  auto const enumera = schema["enum"];
  auto const t = std::string{enumera.IsDefined() ? std::string{name} + "Enum"
                                                 : to_cpp(type)};
  auto const items = schema["items"];
  auto const has_default = schema["default"].IsDefined();
  auto const x =
      items.IsDefined() ? t + '<' + get_type(root, name, items) + '>' : t;
  return required || has_default ? x : std::string{"std::optional<"} + x + ">";
}

bool is_required(YAML::Node const& n) {
  auto const required = n["required"];
  return required.IsDefined() && required.as<bool>();
}

void gen_member(YAML::Node const& root,
                std::string_view name,
                bool required,
                YAML::Node const& schema,
                std::ostream& out) {
  out << "  " << get_type(root, name, schema, required) << " " << name
      << "_{};\n";
}

void gen_value(YAML::Node const& root,
               std::string_view name,
               YAML::Node const& schema,
               YAML::Node const& default_value,
               std::ostream& out) {
  if (auto const ref = schema["$ref"]; ref.IsDefined()) {
    gen_value(root, ref_name(ref), resolve_schema(root, schema), default_value,
              out);
    return;
  }

  auto const type = to_type(schema);
  auto const enumera = schema["enum"];
  if (enumera) {
    out << name << "Enum::" << default_value;
    return;
  }
  switch (type) {
    case type::kArray: {
      auto const item_schema = schema["items"];
      out << get_type(root, name, schema) << "{";
      auto ind = indent{-1, ','};
      for (auto const& v : default_value) {
        ind(out);
        gen_value(root, name, item_schema, v, out);
      }
      out << "}";
    } break;
    case type::kString: out << '"' << default_value << '"'; break;
    default: out << default_value;
  }
}

void gen_member_init(YAML::Node const& root,
                     YAML::Node const& x,
                     bool const is_required,
                     std::ostream& out) {
  auto const schema = x["schema"];
  auto const name = x["name"].as<std::string_view>();
  auto const type = get_type(root, name, schema, is_required);
  out << "  " << name << "_{::openapi::parse_param<" << type << ">(params, \""
      << name << "\"";
  auto const default_value = schema["default"];
  if (default_value.IsDefined()) {
    out << ", ";
    gen_value(root, name, schema, default_value, out);
  }
  out << ")}";
}

void write_params(YAML::Node const& root,
                  YAML::Node const& n,
                  std::ostream& header,
                  std::ostream& source) {
  for (auto const& p : n["parameters"]) {
    auto const name = p["name"].as<std::string_view>();
    auto const items = p["schema"]["items"];
    if (items.IsDefined()) {
      gen_enum(name, items, header, source);
    } else {
      gen_enum(name, p["schema"], header, source);
    }
  }

  auto const id = n["operationId"].as<std::string>() + "_params";

  header << "struct " << id << " {\n";
  header << "  explicit " << id << "(boost::urls::params_view const&);\n";

  source << id << "::" << id << "(boost::urls::params_view const& params)";

  auto const parameters = n["parameters"];
  if (parameters.IsDefined() && parameters.size() != 0) {
    source << " :";
    auto ind = indent{2};
    for (auto const& p : parameters) {
      ind(source);
      gen_member_init(root, p, is_required(p), source);
    }
  }
  source << "\n  {}\n\n";
  for (auto const& p : n["parameters"]) {
    auto const name = p["name"].as<std::string_view>();
    gen_member(root, name, is_required(p), p["schema"], header);
  }
  header << "};\n\n";
}

void gen_type(std::string_view name,
              YAML::Node const& root,
              YAML::Node const& schema,
              std::ostream& header,
              std::ostream& source) {
  if (schema["$ref"].IsDefined()) {
    return;
  }

  auto const type = to_type(schema);

  auto const is_in_required_list =
      [&, required = schema["required"]](std::string_view name) {
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

  if (gen_enum(name, schema, header, source)) {
    return;
  }

  for (auto const& p : schema["properties"]) {
    auto const prop_name = p.first.as<std::string_view>();
    auto const& prop_schema = p.second;
    auto const& items = schema["items"];
    if (items.IsDefined()) {
      gen_enum(prop_name, items, header, source);
    } else {
      gen_enum(prop_name, prop_schema, header, source);
    }
  }

  switch (type) {
    case type::kObject:
      header << "struct " << name << " {\n";

      header << fmt::format(R"(
  auto operator<=>({} const&) const;
  bool operator==({} const&) const;
  bool operator!=({} const&) const;
  bool operator<({} const&) const;
  bool operator<=({} const&) const;
  bool operator>({} const&) const;
  bool operator>=({} const&) const;
)",
                            name, name, name, name, name, name, name, name,
                            name, name, name, name, name, name);

      source << fmt::format(R"(
auto {}::operator<=>({} const&) const = default;
bool {}::operator==({} const&) const = default;
bool {}::operator!=({} const&) const = default;
bool {}::operator<({} const&) const = default;
bool {}::operator<=({} const&) const = default;
bool {}::operator>({} const&) const = default;
bool {}::operator>=({} const&) const = default;
)",
                            name, name, name, name, name, name, name, name,
                            name, name, name, name, name, name);

      // OSTREAM
      header << "  friend std::ostream& operator<<(std::ostream&, " << name
             << " const&);\n\n";
      source << "std::ostream& operator<<(std::ostream& out, " << name
             << " const& x) {\n"
             << "  return out << "
                "boost::json::serialize(boost::json::value_from(x));\n"
             << "}\n\n";

      // JSON -> TYPE
      header << "  friend " << name << " tag_invoke(boost::json::value_to_tag<"
             << name << ">, boost::json::value const&);\n";

      source << name << " tag_invoke(boost::json::value_to_tag<" << name
             << ">, "
                "boost::json::value const& jv) {\n"
                "    auto v = "
             << name << "{};\n";
      for (auto const& p : schema["properties"]) {
        auto const member_name = p.first.as<std::string_view>();
        source << "    openapi::extract_member(jv.as_object(), v."
               << member_name << "_, \"" << member_name << "\");\n";
      }
      source << "    return v;\n"
                "  }\n\n";

      // TYPE -> JSON
      header << "  friend void tag_invoke(boost::json::value_from_tag, "
                "boost::json::value& "
                "jv, "
             << name << " const& v);\n\n";

      source << "void tag_invoke(boost::json::value_from_tag, "
                "boost::json::value& "
                "jv, "
             << name
             << " const& v) {\n"
                "    auto& o = (jv = boost::json::object{}).as_object();\n";
      for (auto const& p : schema["properties"]) {
        auto const member_name = p.first.as<std::string_view>();
        source << "    openapi::write_member(o, v." << member_name << "_, \""
               << member_name << "\");\n";
      }
      source << "  }\n\n";

      for (auto const& p : schema["properties"]) {
        auto const member_name = p.first.as<std::string_view>();
        auto const required =
            is_in_required_list(member_name) || is_required(p.second);
        gen_member(root, member_name, required, p.second, header);
      }
      header << "};\n\n";
      break;

    case type::kArray:
      gen_enum(std::string{name}, schema["items"], header, source);
      [[fallthrough]];

    default:
      header << "using " << name << " = " << get_type(root, name, schema)
             << ";\n\n";
      break;
  }
}

void write_types(YAML::Node const& root,
                 std::string_view path_to_header,
                 std::ostream& header,
                 std::ostream& source,
                 std::optional<std::string_view> ns) {
  write_prelude(path_to_header, header, source, ns);

  auto const components = root["components"];
  if (components.IsDefined()) {
    for (auto const& c : components["schemas"]) {
      gen_type(c.first.as<std::string_view>(), root, c.second, header, source);
    }
  }

  for (auto const& path : root["paths"]) {
    for (auto const& method : path.second) {
      write_params(root, method.second, header, source);

      for (auto const& response : method.second["responses"]) {
        gen_type(method.second["operationId"].as<std::string>() + "_response",
                 root, response.second["content"]["application/json"]["schema"],
                 header, source);
      }
    }
  }

  write_postlude(header, source, ns);
}

}  // namespace openapi