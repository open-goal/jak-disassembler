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
#include <util\Log.h>

int main(int argc, char** argv) {
  logger.writeln("Jak Disassembler");

  Api::initialize();
  if (argc != 4) {
    logger.writeln("usage: jak_disassembler <config_file> <in_folder> <out_folder>");
    return 1;
  }

  Api::setConfiguration(argv[1]);
  std::string in_folder = argv[2];
  std::string out_folder = argv[3];
  logger.setOutputPath(out_folder);

  std::vector<std::string> dgos;
  for (const auto& dgo_name : get_config().dgo_names) {
    dgos.push_back(combine_path(in_folder, dgo_name));
  }

  Api::disassembleFiles(out_folder, dgos);

  return 0;
}
