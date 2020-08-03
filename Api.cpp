#include <string>
#include <vector>

#include "Api.h"
#include "ObjectFileDB.h"
#include "util/FileIO.h"
#include "util/Log.h"
#include <TypeSystem\TypeInfo.h>

void Api::initialize() {
  init_crc();
  init_opcode_info();
}

void Api::setConfiguration(std::string configFilePath) {
  set_config(configFilePath);
}

void Api::setWriteScripts(bool val) {
  get_config().write_scripts = val;
}

void Api::setWriteHexdump(bool val) {
  get_config().write_hexdump = val;
}

void Api::setWriteDisassembly(bool val) {
  get_config().write_disassembly = val;
}

Api::DisassemblyInfo Api::disassembleFile(std::string outputPath, std::string inputPath) {
  DisassemblyInfo disasmMetadata;

  std::vector<std::string> inputPaths;
  inputPaths.push_back(inputPath);
  ObjectFileDB db(inputPaths);
  write_text_file(combine_path(outputPath, "dgo.txt"), db.generate_dgo_listing());

  db.process_link_data();
  db.find_code();
  db.process_labels();

  if (get_config().write_scripts) {
    db.find_and_write_scripts(outputPath);
  }

  if (get_config().write_hexdump) {
    db.write_object_file_words(outputPath, get_config().write_hexdump_on_v3_only);
  }

  db.analyze_functions();

  if (get_config().write_disassembly) {
    db.write_disassembly(outputPath, get_config().disassemble_objects_without_functions);
  }

  logger.writeln("%s", get_type_info().get_summary().c_str());

  // TODO - implement per file stats eventually, currently a bit tricky to return that from
  // ObjectFileDB
  disasmMetadata.status = "Completed";  // TODO - status for skipped files
  return disasmMetadata;
}

std::vector<Api::DisassemblyInfo> Api::disassembleFiles(std::string outputPath,
                                                        std::vector<std::string> inputPaths) {
  std::vector<DisassemblyInfo> disasmMetadata;
  for (int i = 0; i < inputPaths.size(); i++) {
    disasmMetadata.push_back(disassembleFile(outputPath, inputPaths[i]));
  }
  return disasmMetadata;
}
