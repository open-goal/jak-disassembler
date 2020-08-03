#include <stdarg.h>

#include "Log.h"
#include <stdexcept>

LogTeeWriter logger;

void LogTeeWriter::setOutputPath(std::string path) {
  outputPath = path;
}

void LogTeeWriter::writeln(const char* fmt, ...) {
  std::string fmtStr = fmt;
  fmtStr.append("\n");

  va_list ap;
  va_start(ap, fmt);
  vfprintf(stdout, fmtStr.c_str(), ap);
  writeToFile(fmtStr.c_str(), ap);
  va_end(ap);
}

void LogTeeWriter::write(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  writeToFile(fmt, ap);
  va_end(ap);
}

void LogTeeWriter::writeToFile(const char* fmt, va_list args) {
  if (!outputPath.empty()) {
    std::string filePath = outputPath;
#ifdef _WIN32
    filePath.append("\\logs.log");
#elif __linux__
    filePath.append("/logs.log");
#endif
    FILE* fp = fopen(filePath.c_str(), "a");
    if (!fp) {
      printf("Failed to fopen %s\n", filePath.c_str());
      throw std::runtime_error("Failed to open file");
    }
    vfprintf(fp, fmt, args);
    fclose(fp);
  }
}
