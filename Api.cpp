#include <string>
#include <vector>

#include "Api.h"
#include "ObjectFileDB.h"
#include "util/FileIO.h"

int Api::disassemble(std::string outputPath, std::vector<std::string> inputPaths) {
  init_crc();
  init_opcode_info();

  ObjectFileDB db(inputPaths);
  write_text_file(combine_path(outputPath, "dgo.txt"), db.generate_dgo_listing());

  db.process_link_data();
  db.find_code();
  db.process_labels();
  db.find_scripts(outputPath);

  //  db.write_object_file_words(output_path, false);
  db.write_disassembly(outputPath);

  return 0;
}
