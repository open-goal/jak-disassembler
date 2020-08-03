#include <algorithm>
#include <cassert>
#include "BasicBlocks.h"
#include "ObjectFile/LinkedObjectFile.h"
#include "Disasm/InstructionMatching.h"

/*!
 * Find all basic blocks in a function.
 * All delay slot instructions are grouped with the branch instruction.
 * This is done by finding all "dividers", which are after branch delay instructions and before
 * branch destinations, then sorting them, ignoring duplicates, and creating the blocks.
 */
std::vector<BasicBlock> find_blocks_in_function(const LinkedObjectFile& file,
                                                int seg,
                                                const Function& func) {
  std::vector<BasicBlock> basic_blocks;

  // note - the first word of a function is the "function" type and should go in any basic block
  std::vector<int> dividers = {0, int(func.instructions.size())};

  for (int i = 0; i < int(func.instructions.size()); i++) {
    const auto& instr = func.instructions.at(i);
    const auto& instr_info = instr.get_info();

    if (instr_info.is_branch || instr_info.is_branch_likely) {
      // make sure the delay slot of this branch is included in the function
      assert(i + func.start_word < func.end_word - 1);
      // divider after delay slot
      dividers.push_back(i + 2);
      auto label_id = instr.get_label_target();
      assert(label_id != -1);
      const auto& label = file.labels.at(label_id);
      // should only jump to within our own function
      assert(label.target_segment == seg);
      assert(label.offset / 4 > func.start_word);
      assert(label.offset / 4 < func.end_word - 1);
      dividers.push_back(label.offset / 4 - func.start_word);
    }
  }

  std::sort(dividers.begin(), dividers.end());

  for (size_t i = 0; i < dividers.size() - 1; i++) {
    if (dividers[i] != dividers[i + 1]) {
      basic_blocks.emplace_back(dividers[i], dividers[i + 1]);
      assert(dividers[i] >= 0);
    }
  }

  return basic_blocks;
}

//std::shared_ptr<ControlFlowGraph> build_cfg(const LinkedObjectFile& file,
//                                            int seg,
//                                            const Function& func) {
//  auto cfg = std::make_shared<ControlFlowGraph>();
//  auto& blocks = cfg->create_blocks(func.basic_blocks.size());
//
//  // link in entry and exit
//  cfg->link(cfg->entry(), blocks.front());
//  cfg->link(blocks.back(), cfg->exit());
//
//  // todo link early returns
//
//  // link normal branches
//  for (int i = 0; i < int(func.basic_blocks.size()); i++) {
//    auto& b = func.basic_blocks[i];
//    bool not_last = (i + 1) < int(func.basic_blocks.size());
//
//    if (b.end_word - b.start_word < 2) {
//      // there's no room for a branch here, fall through to the end
//      if (not_last) {
//        cfg->link(blocks.at(i), blocks.at(i + 1));
//      }
//    } else {
//      // might be a branch
//      int idx = b.end_word - 2;
//      assert(idx >= b.start_word);
//      auto& branch_candidate = func.instructions.at(idx);
//
//      if (is_branch(branch_candidate, {})) {
//        // todo - distinguish between likely and not likely.
//        blocks.at(i)->has_branch = true;
//        blocks.at(i)->branch_likely = is_branch(branch_candidate, true);
//        bool branch_always = is_always_branch(branch_candidate);
//
//        // need to find block target
//        int block_target = -1;
//        int label_target = branch_candidate.get_label_target();
//        assert(label_target != -1);
//        const auto& label = file.labels.at(label_target);
//        assert(label.target_segment == seg);
//        assert((label.offset % 4) == 0);
//        int offset = label.offset / 4 - func.start_word;
//        assert(offset >= 0);
//
////        for (int j = 0; j < int(func.basic_blocks.size()); j++) {
//        for(int j = int(func.basic_blocks.size()); j-- > 0;) {
//          if (func.basic_blocks[j].start_word == offset) {
//            block_target = j;
//            break;
//          }
//        }
//
//        assert(block_target != -1);
//        cfg->link(blocks.at(i), blocks.at(block_target));
//
//        if (branch_always) {
//          // don't continue to the next one
//          blocks.at(i)->branch_always = true;
//        } else {
//          if (not_last) {
//            cfg->link(blocks.at(i), blocks.at(i + 1));
//          }
//        }
//      } else {
//        if (not_last) {
//          cfg->link(blocks.at(i), blocks.at(i + 1));
//        }
//      }
//    }
//  }
//
//  if (cfg->compact_top_level()) {
//    std::string result;
//    int bid = 0;
//    for (auto& bblock : func.basic_blocks) {
//      result += "BLOCK " + std::to_string(bid++) + "\n";
//      for (int i = bblock.start_word; i < bblock.end_word; i++) {
//        if (i >= 0 && i < func.instructions.size()) {
//          result += func.instructions.at(i).to_string(file) + "\n";
//        } else {
//          result += "BAD BBLOCK INSTR ID " + std::to_string(i);
//        }
//      }
//    }
//    printf("CODE:%s\n\n\n\n", result.c_str());
//  }
//
//  return cfg;
//}