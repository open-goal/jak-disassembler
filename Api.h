#include "ObjectFileDB.h"
#include "util/FileIO.h"

class Api {
 public:
  static int disassemble(std::string outputPath, std::vector<std::string> inputPaths);
};
