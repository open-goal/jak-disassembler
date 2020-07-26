#include <cstdio>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  printf("Jak 2 Disassembler\n");


  if(argc < 3) {
    printf("usage: jak2_disassembler <output path> <dgos>\n");
    return 1;
  }

  std::string output_path = argv[1];
  std::vector<std::string> dgos;
  for(int i = 2; i < argc; i++) {
    dgos.emplace_back(argv[i]);
  }


  return 0;
}
