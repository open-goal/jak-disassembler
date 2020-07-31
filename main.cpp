#include <cstdio>
#include <string>
#include <vector>
#include "ObjectFileDB.h"
#include "util/FileIO.h"

int main(int argc, char** argv) {
  printf("Jak 2 Disassembler\n");
  init_crc();
  init_opcode_info();

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

  ObjectFileDB db(dgos);
  write_text_file(combine_path(output_path, "dgo.txt"), db.generate_dgo_listing());

  db.process_link_data();
  db.find_code();
  db.process_labels();
  db.find_scripts(output_path);

  //  db.write_object_file_words(output_path, false);
  db.write_disassembly(output_path);

  return 0;
}
