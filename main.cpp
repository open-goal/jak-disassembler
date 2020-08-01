#include <cstdio>
#include <string>
#include <vector>
#include "TypeSystem/TypeInfo.h"
#include "ObjectFileDB.h"
#include "config.h"
#include "util/FileIO.h"

#include <ObjectFileDB.h>
#include <util\FileIO.h>
#include "Api.h"

int main(int argc, char** argv) {
  printf("Jak Disassembler\n");
  init_crc();
  init_opcode_info();
  if (argc != 4) {
    printf("usage: jak_disassembler <config_file> <in_folder> <out_folder>\n");
    return 1;
  }

  set_config(argv[1]);
  std::string in_folder = argv[2];
  std::string out_folder = argv[3];

  std::vector<std::string> dgos;
  for (const auto& dgo_name : get_config().dgo_names) {
    dgos.push_back(combine_path(in_folder, dgo_name));
  }

  ObjectFileDB db(dgos);
  write_text_file(combine_path(out_folder, "dgo.txt"), db.generate_dgo_listing());

  db.process_link_data();
  db.find_code();
  db.process_labels();

  if (get_config().write_scripts) {
    db.find_and_write_scripts(out_folder);
  }

  if (get_config().write_hexdump) {
    db.write_object_file_words(out_folder, get_config().write_hexdump_on_v3_only);
  }

  db.analyze_functions();

  if (get_config().write_disassembly) {
    db.write_disassembly(out_folder, get_config().disassemble_objects_without_functions);
  }

  printf("%s\n", get_type_info().get_summary().c_str());

  return 0;
  //  printf("output folder is %s\n", output_path.c_str());
  //  printf("input dgos are:\n");
  //  for(auto& dgo : dgos) {
  //    printf("  %s\n", dgo.c_str());
  //  }

  //return Api::disassemble(output_path, dgos);
}
