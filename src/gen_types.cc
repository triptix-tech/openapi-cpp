#include "openapi/gen_types.h"

#include <ostream>

namespace openapi {

void generate_types(std::ostream& out, std::string_view) {
  out << "Hello!\n";
}

}