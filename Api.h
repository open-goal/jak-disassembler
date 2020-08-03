#include "ObjectFileDB.h"
#include "util/FileIO.h"
#include <config.h>

class Api {
 public:
  struct DisassemblyInfo {
    std::string baseFileName;
    std::string status;
    int numObjects;
    long sizeBytes;
    float durationSeconds;
    int numFunctions;
    int numSymbols;
    int numLabels;
    int numBasicBlocks;
  };
  static void initialize();
  static void setConfiguration(std::string configFilePath);
  static void setWriteScripts(bool val);
  static void setWriteHexdump(bool val);
  static void setWriteDisassembly(bool val);
  static DisassemblyInfo disassembleFile(std::string outputPath, std::string inputPaths);
  static std::vector<DisassemblyInfo> disassembleFiles(std::string outputPath,
                                                       std::vector<std::string> inputPaths);
};
