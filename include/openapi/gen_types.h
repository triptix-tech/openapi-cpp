#pragma once

#include <optional>
#include <ostream>
#include <string_view>

#include "utl/verify.h"

#include "yaml-cpp/yaml.h"

#include "cista/hash.h"

#include "utl/to_vec.h"

namespace openapi {

enum class type { kBoolean, kInteger, kNumber, kString, kArray, kObject };

type to_type(std::string_view);

std::string_view to_cpp(type const);

bool gen_enum(std::string_view name, YAML::Node const& schema, std::ostream&);

std::string get_type(YAML::Node const& root,
                     std::string_view name,
                     YAML::Node const& schema,
                     bool const required = true);

bool is_required(YAML::Node const& n);

void gen_member(YAML::Node const& root,
                std::string_view name,
                bool required,
                YAML::Node const& schema,
                std::ostream&);

void gen_value(YAML::Node const& root,
               std::string_view name,
               YAML::Node const& schema,
               YAML::Node const& default_value,
               std::ostream&);

void gen_member_init(YAML::Node const& root,
                     YAML::Node const& x,
                     std::ostream&);

void write_params(YAML::Node const& root, YAML::Node const&, std::ostream&);

void write_types(YAML::Node const&,
                 std::ostream&,
                 std::optional<std::string_view>);

}  // namespace openapi