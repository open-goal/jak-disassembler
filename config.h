#ifndef JAK2_DISASSEMBLER_CONFIG_H
#define JAK2_DISASSEMBLER_CONFIG_H

#include <string>
#include <vector>

struct Config {
  int game_version = -1;
  std::vector<std::string> dgo_names;
  bool write_disassembly = false;
  bool write_hexdump = false;
  bool write_scripts = false;
  bool write_hexdump_on_v3_only = false;
  bool disassemble_objects_without_functions = false;
  bool find_basic_blocks = false;
  bool write_hex_near_instructions = false;
};

Config& get_config();
void set_config(const std::string& path_to_config_file);

#endif  // JAK2_DISASSEMBLER_CONFIG_H
