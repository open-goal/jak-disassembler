#include <cstdio>
#include <string>
#include <vector>

#include <ObjectFileDB.h>
#include <util\FileIO.h>
#include "Api.h"

int main(int argc, char** argv) {
  printf("Jak 2 Disassembler\n");
  if (argc < 3) {
    printf("usage: jak2_disassembler <output path> <dgos>\n");
    return 1;
  }

  std::string output_path = argv[1];
  std::vector<std::string> dgos;
  for (int i = 2; i < argc; i++) {
    dgos.emplace_back(argv[i]);
  }

  //  printf("output folder is %s\n", output_path.c_str());
  //  printf("input dgos are:\n");
  //  for(auto& dgo : dgos) {
  //    printf("  %s\n", dgo.c_str());
  //  }

  return Api::disassemble(output_path, dgos);
}
