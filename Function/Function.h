#ifndef NEXT_FUNCTION_H
#define NEXT_FUNCTION_H

#include <string>
#include <vector>
#include "Disasm/Instruction.h"
#include "BasicBlocks.h"

class Function {
public:
  Function(int _start_word, int _end_word);

  int segment = -1;
  int start_word = -1;
  int end_word = -1; // not inclusive, but does include padding.

  std::string guessed_name;

  std::vector<Instruction> instructions;
  std::vector<BasicBlock> basic_blocks;

  struct Prologue {
    bool decoded = false;
    bool strange = false;
    int length = -1;
    int total_stack_size = -1;
    bool ra_backup = false;
    bool fp_backup = false;

    // gpr backup
    // fpr backup

  };

  bool uses_fp_register = false;
};


#endif //NEXT_FUNCTION_H
