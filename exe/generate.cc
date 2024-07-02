#include <fstream>
#include <iostream>

#include "openapi/gen_types.h"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << "usage: openapi-generator [OPENAPI.YML] [/PATH/TO/HEADER.h]\n";
    return 1;
  }

  auto const root = YAML::LoadFile(argv[1]);
  auto out = std::ofstream{argv[2]};
  openapi::write_types(root, out);
}