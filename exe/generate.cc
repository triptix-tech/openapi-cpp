#include <fstream>
#include <iostream>

#include "openapi/gen_types.h"

int main(int argc, char** argv) {
  if (argc < 5) {
    std::cout << "usage: openapi-generator [OPENAPI.YML] [/PATH/TO/HEADER.h] "
                 "[/PATH/TO/SOURCE.cc] "
                 "[NAMESPACE]\n";
    return 1;
  }

  auto const root = YAML::LoadFile(argv[1]);
  auto header = std::ofstream{argv[2]};
  auto source = std::ofstream{argv[3]};
  openapi::write_types(root, argv[2], header, source,
                       std::string_view{argv[4]});
}