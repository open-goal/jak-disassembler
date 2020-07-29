#include <stdarg.h>

class Log {
  enum class LogType { Disassembly, Compression, Assembly, Verification };
  enum class LogLevel { Debug, Info, Warning, Error };

  void writeln(LogLevel level, LogType type, const char* fmt, ...) {
    // TODO - append level/type info to beginning of log
    va_list ap;
    va_start(ap, fmt);
    if (level = LogLevel.Error)
      vfprintf(stderr, fmt, ap);
    else
      vfprintf(stdout, fmt, ap);
    va_end(ap);
  }

  char* levelToString(LogLevel level) {
    switch (level) {
      case LogLevel.Debug:
        return "Debug";
      case LogLevel.Info:
        return "Info";
      case LogLevel.Warning:
        return "Warning";
      case LogLevel.Error:
        return "Error";
    }
  }

  void logToGui(LogType type) {
    // TODO - This class needs references to the GUI in some respect to send the log there as well
  }
};
