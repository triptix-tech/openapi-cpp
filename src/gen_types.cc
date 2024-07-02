#include "openapi/gen_types.h"

#include <ostream>

namespace openapi {

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

bool gen_enum(std::string_view type_name,
              YAML::Node const& schema,
              std::ostream& out) {
  auto const name = std::string{type_name} + "_enum";
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
      out << "inline " << name << " tag_invoke(boost::json::value_to_tag<"
          << name << ">, boost::json::value const& jv) {\n";
      out << "  auto x = " << name << "{};\n";
      out << "  auto const sv = std::string_view{jv.as_string()};\n";
      out << "  switch (cista::hash(sv)) {";
      auto ind = indent{2, 0};
      for (auto const& e : enumera) {
        ind(out);
        out << "case cista::hash(\"" << e << "\"): x = " << name << "::" << e
            << "; break;";
      }
      ind(out);
      out << "default: throw utl::fail(\"enum " << name
          << ": unknown value {}\", sv);\n";
      out << "  }\n";
      out << "  return x;\n";
      out << "}\n\n";
    }
    {
      out << "inline void tag_invoke(boost::json::value_from_tag, "
             "boost::json::value& jv, "
          << name
          << " const v) {\n"
             "  switch (v) {";
      auto ind = indent{2, 0};
      for (auto const& e : enumera) {
        ind(out);
        out << "case " << name << "::" << e << ": jv = \"" << e << "\"; break;";
      }
      ind(out);
      out << "default: std::unreachable();\n";
      out << "  }\n"
             "}\n\n";
    }
    return true;
  }
  return false;
}

std::string get_type(YAML::Node const& root,
                     std::string_view name,
                     YAML::Node const& schema,
                     bool const required) {
  auto const ref = schema["$ref"];
  if (ref.IsDefined()) {
    auto const prefix = std::string_view{"#/components/schemas/"};
    auto const type = ref.as<std::string_view>().substr(prefix.size());
    auto const enum_postfix =
        root["components"]["schemas"][type]["enum"].IsDefined() ? "_enum" : "";
    return std::string{type} + "" + enum_postfix;
  }

  auto const type = to_type(schema["type"].as<std::string_view>());
  auto const enumera = schema["enum"];
  auto const t = std::string{enumera.IsDefined() ? std::string{name} + "_enum"
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
      << "_;\n";
}

void gen_value(YAML::Node const& root,
               std::string_view name,
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
                     std::ostream& out) {
  auto const schema = x["schema"];
  auto const name = x["name"].as<std::string_view>();
  auto const type = get_type(root, name, schema);
  out << "  " << name << "_{::openapi::parse_param<" << type << ">(url, \""
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
                  std::ostream& out) {
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
      gen_member_init(root, p, out);
    }
  }
  out << "\n  {}\n\n";
  for (auto const& p : n["parameters"]) {
    auto const name = p["name"].as<std::string_view>();
    gen_member(root, name, is_required(p), p["schema"], out);
  }
  out << "};\n";
}

void write_types(YAML::Node const& root, std::ostream& out) {
  for (auto const& path : root["paths"]) {
    for (auto const& method : path.second) {
      write_params(root, method.second, out);
    }
  }

  auto const components = root["components"];
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

          out << "\n"
                 "  friend bool operator<=>("
              << name << " const&, " << name << " const&) = default;\n\n";

          // JSON -> TYPE
          out << "  friend " << name << " tag_invoke(boost::json::value_to_tag<"
              << name
              << ">, "
                 "boost::json::value const& jv) {\n"
                 "    auto v = "
              << name << "{};\n";
          for (auto const& p : schema["properties"]) {
            auto const member_name = p.first.as<std::string_view>();
            out << "    openapi::extract_member(jv.as_object(), v."
                << member_name << "_, \"" << member_name << "\");\n";
          }
          out << "    return v;\n"
                 "  }\n\n";

          // TYPE -> JSON
          out << "  friend void tag_invoke(boost::json::value_from_tag, "
                 "boost::json::value& "
                 "jv, "
              << name
              << " const& v) {\n"
                 "    auto& o = (jv = boost::json::object{}).as_object();\n";
          for (auto const& p : schema["properties"]) {
            auto const member_name = p.first.as<std::string_view>();
            out << "    openapi::write_member(o, v." << member_name << "_, \""
                << member_name << "\");\n";
          }
          out << "  }\n\n";

          for (auto const& p : schema["properties"]) {
            auto const member_name = p.first.as<std::string_view>();
            gen_member(root, member_name, is_required(member_name), p.second,
                       out);
          }
          out << "};\n\n";
          break;

        case type::kArray:
          gen_enum(std::string{name}, schema["items"], out);
          [[fallthrough]];

        default:
          out << "using " << name << " = " << get_type(root, name, schema)
              << ";\n\n";
          break;
      }
    }
  }
}

}  // namespace openapi