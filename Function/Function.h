#ifndef NEXT_FUNCTION_H
#define NEXT_FUNCTION_H

#include <string>
#include <vector>
#include "Disasm/Instruction.h"
#include "BasicBlocks.h"
#include "CfgVtx.h"

class Function {
 public:
  Function(int _start_word, int _end_word);
  void analyze_prologue(const LinkedObjectFile& file);
  void find_global_function_defs(LinkedObjectFile& file);

  int segment = -1;
  int start_word = -1;
  int end_word = -1;  // not inclusive, but does include padding.

  std::string guessed_name;

  bool suspected_asm = false;

  std::vector<Instruction> instructions;
  std::vector<BasicBlock> basic_blocks;
  std::shared_ptr<ControlFlowGraph> cfg;

  int prologue_start = -1;
  int prologue_end = -1;

  int epilogue_start = -1;
  int epilogue_end = -1;

  std::string warnings;

  struct Prologue {
    bool decoded = false;  // have we removed the prologue from basic blocks?
    int total_stack_usage = -1;

    // ra/fp are treated differently from other register backups
    bool ra_backed_up = false;
    int ra_backup_offset = -1;

    bool fp_backed_up = false;
    int fp_backup_offset = -1;

    bool fp_set = false;

    int n_gpr_backup = 0;
    int gpr_backup_offset = -1;

    int n_fpr_backup = 0;
    int fpr_backup_offset = -1;

    int n_stack_var_bytes = 0;
    int stack_var_offset = -1;

    bool epilogue_ok = false;

    std::string to_string(int indent = 0) const;

  } prologue;

  bool uses_fp_register = false;

 private:
  void check_epilogue(const LinkedObjectFile& file);
};

#endif  // NEXT_FUNCTION_H
