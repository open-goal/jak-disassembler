#include <string>

// Simple Class to `tee` the logs to both stdout and a file
// useful for non-unix environments ;)
class LogTeeWriter {
 private:
  std::string outputPath;
  void writeToFile(const char* fmt, va_list args);

 public:
  void setOutputPath(std::string outputPath);

  void writeln(const char* fmt, ...);
  void write(const char* fmt, ...);
};

extern LogTeeWriter logger;
