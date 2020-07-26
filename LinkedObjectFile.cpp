/*!
 * @file LinkedObjectFile.cpp
 * An object file's data with linking information included.
 */
#include <cassert>
#include <numeric>
#include <algorithm>
#include "LinkedObjectFile.h"
#include "Disasm/InstructionDecode.h"


/*!
 * Set the number of segments in this object file.
 * This can only be done once, and must be done before adding any words.
 */
void LinkedObjectFile::set_segment_count(int n_segs) {
  assert(segments == 0);
  segments = n_segs;
  words_by_seg.resize(n_segs);
  label_per_seg_by_offset.resize(n_segs);
  offset_of_data_zone_by_seg.resize(n_segs);
  functions_by_seg.resize(n_segs);
}

/*!
 * Add a single word to the given segment.
 */
void LinkedObjectFile::push_back_word_to_segment(uint32_t word, int segment) {
  words_by_seg.at(segment).emplace_back(word);
}

/*!
 * Get a label ID for a label which points to the given offset in the given segment.
 * Will return an existing label if one exists.
 */
int LinkedObjectFile::get_label_id_for(int seg, int offset) {
  auto kv = label_per_seg_by_offset.at(seg).find(offset);
  if(kv == label_per_seg_by_offset.at(seg).end()) {
    // create a new label
    int id = labels.size();
    Label label;
    label.target_segment = seg;
    label.offset = offset;
    label.name = "L" + std::to_string(id);
    label_per_seg_by_offset.at(seg)[offset] = id;
    labels.push_back(label);
    return id;
  } else {
    // return an existing label
    auto& label = labels.at(kv->second);
    assert(label.offset == offset);
    assert(label.target_segment == seg);
    return kv->second;
  }
}

/*!
 * Get the ID of the label which points to the given offset in the given segment.
 * Returns -1 if there is no label.
 */
int LinkedObjectFile::get_label_at(int seg, int offset) {
  auto kv = label_per_seg_by_offset.at(seg).find(offset);
  if(kv == label_per_seg_by_offset.at(seg).end()) {
    return -1;
  }

  return kv->second;
}

std::string LinkedObjectFile::get_label_name(int label_id) {
  return labels.at(label_id).name;
}

/*!
 * Add link information that a word is a pointer to another word.
 */
void LinkedObjectFile::pointer_link_word(int source_segment, int source_offset, int dest_segment, int dest_offset) {
  assert((source_offset % 4) == 0);

  auto& word = words_by_seg.at(source_segment).at(source_offset / 4);
  assert(word.kind == LinkedWord::PLAIN_DATA);
  assert(dest_offset / 4 <= (int)words_by_seg.at(dest_segment).size());

  word.kind = LinkedWord::PTR;
  word.label_id = get_label_id_for(dest_segment, dest_offset);
}

/*!
 * Add link information that a word is linked to a symbol/type/empty list.
 */
void LinkedObjectFile::symbol_link_word(int source_segment, int source_offset, const char *name, LinkedWord::Kind kind) {
  assert((source_offset % 4) == 0);
  auto& word = words_by_seg.at(source_segment).at(source_offset / 4);
  assert(word.kind == LinkedWord::PLAIN_DATA);
  word.kind = kind;
  word.symbol_name = name;
}

/*!
 * Add link information that a word's lower 16 bits are the offset of the given symbol relative to the symbol
 * table register.
 */
void LinkedObjectFile::symbol_link_offset(int source_segment, int source_offset, const char *name) {
  assert((source_offset % 4) == 0);
  auto& word = words_by_seg.at(source_segment).at(source_offset / 4);
  assert(word.kind == LinkedWord::PLAIN_DATA);
  word.kind = LinkedWord::SYM_OFFSET;
  word.symbol_name = name;
}

/*!
 * Add link information that a lui/ori pair will load a pointer.
 */
void LinkedObjectFile::pointer_link_split_word(int source_segment, int source_hi_offset, int source_lo_offset, int dest_segment, int dest_offset) {
  assert((source_hi_offset % 4) == 0);
  assert((source_lo_offset % 4) == 0);

  auto& hi_word = words_by_seg.at(source_segment).at(source_hi_offset / 4);
  auto& lo_word = words_by_seg.at(source_segment).at(source_lo_offset / 4);

//  assert(dest_offset / 4 <= (int)words_by_seg.at(dest_segment).size());
  assert(hi_word.kind == LinkedWord::PLAIN_DATA);
  assert(lo_word.kind == LinkedWord::PLAIN_DATA);

  hi_word.kind = LinkedWord::HI_PTR;
  hi_word.label_id = get_label_id_for(dest_segment, dest_offset);

  lo_word.kind = LinkedWord::LO_PTR;
  lo_word.label_id = hi_word.label_id;
}

/*!
 * Rename the labels so they are named L1, L2, ..., in the order of the addresses that they refer to.
 * Will clear any custom label names.
 */
uint32_t LinkedObjectFile::set_ordered_label_names() {
  std::vector<int> indices(labels.size());
  std::iota(indices.begin(), indices.end(), 0);

  std::sort(indices.begin(), indices.end(), [&](int a, int b) {
    auto& la = labels.at(a);
    auto& lb = labels.at(b);
    if(la.target_segment == lb.target_segment) {
      return la.offset < lb.offset;
    }
    return la.target_segment < lb.target_segment;
  });

  for(size_t i = 0; i < indices.size(); i++) {
    auto& label = labels.at(indices[i]);
    label.name = "L" + std::to_string(i + 1);
  }

  return labels.size();
}

static const char* segment_names[] = {"main segment", "debug segment", "top-level segment"};

/*!
 * Print all the words, with link information and labels.
 */
std::string LinkedObjectFile::print_words() {
  std::string result;

  assert(segments <= 3);
  for(int seg = segments; seg-- > 0;) {
    // segment header
    result += ";------------------------------------------\n;  ";
    result += segment_names[seg];
    result += "\n;------------------------------------------\n";

    // print each word in the segment
    for(size_t i = 0; i < words_by_seg.at(seg).size(); i++) {
      for(int j = 0; j < 4; j++) {
        auto label_id = get_label_at(seg, i*4 + j);
        if(label_id != -1) {
          result += labels.at(label_id).name + ":";
          if(j != 0) {
            result += " (offset " + std::to_string(j) + ")";
          }
          result += "\n";
        }
      }

      auto& word = words_by_seg[seg][i];
      append_word_to_string(result, word);
    }
  }

  return result;
}

/*!
 * Add a word's printed representation to the end of a string. Internal helper for print_words.
 */
void LinkedObjectFile::append_word_to_string(std::string &dest, const LinkedWord &word) {
  char buff[128];

  switch(word.kind) {
    case LinkedWord::PLAIN_DATA:
      sprintf(buff, "    .word 0x%x\n", word.data);
      break;
    case LinkedWord::PTR:
      sprintf(buff, "    .word %s\n", labels.at(word.label_id).name.c_str());
      break;
    case LinkedWord::SYM_PTR:
      sprintf(buff, "    .symbol %s\n", word.symbol_name.c_str());
      break;
    case LinkedWord::TYPE_PTR:
      sprintf(buff, "    .type %s\n", word.symbol_name.c_str());
      break;
    case LinkedWord::EMPTY_PTR:
      sprintf(buff, "    .empty-list\n"); // ?
      break;
    case LinkedWord::HI_PTR:
      sprintf(buff, "    .ptr-hi 0x%x %s\n", word.data >> 16, labels.at(word.label_id).name.c_str());
      break;
    case LinkedWord::LO_PTR:
      sprintf(buff, "    .ptr-lo 0x%x %s\n", word.data >> 16, labels.at(word.label_id).name.c_str());
      break;
    case LinkedWord::SYM_OFFSET:
      sprintf(buff, "    .sym-off 0x%x %s\n", word.data >> 16, word.symbol_name.c_str());
      break;
    default:
      throw std::runtime_error("nyi");

  }

  dest += buff;
}

/*!
 * For each segment, determine where the data area starts.  Before the data area is the code area.
 */
void LinkedObjectFile::find_code() {
  if(segments == 1) {
    // single segment object files should never have any code.
    auto& seg = words_by_seg.front();
    for(auto& word : seg) {
      if(!word.symbol_name.empty()) {
        assert(word.symbol_name != "function");
      }
    }
    offset_of_data_zone_by_seg.at(0) = 0;
    stats.data_bytes = words_by_seg.front().size() * 4;
    stats.code_bytes = 0;

  } else if(segments == 3) {

    // V3 object files will have all the functions, then all the static data.  So to find the divider, we look for the
    // last "function" tag, then find the last jr $ra instruction after that (plus one for delay slot) and assume
    // that after that is data.  Additionally, we check to make sure that there are no "function" type tags in the data
    // section, although this is redundant.
    for(int i = 0; i < segments; i++) {
      // try to find the last reference to "function":
      bool found_function = false;
      size_t function_loc = -1;
      for(size_t j = words_by_seg.at(i).size(); j-- > 0;) {
        auto& word = words_by_seg.at(i).at(j);
        if(word.kind == LinkedWord::TYPE_PTR && word.symbol_name == "function") {
          function_loc = j;
          found_function = true;
          break;
        }
      }

      if(found_function) {
        // look forward until we find "jr ra"
        const uint32_t jr_ra = 0x3e00008;
        bool found_jr_ra = false;
        size_t jr_ra_loc = -1;

        for(size_t j = function_loc; j < words_by_seg.at(i).size(); j++) {
          auto& word = words_by_seg.at(i).at(j);
          if(word.kind == LinkedWord::PLAIN_DATA && word.data == jr_ra) {
            found_jr_ra = true;
            jr_ra_loc = j;
          }
        }

        assert(found_jr_ra);
        assert(jr_ra_loc + 1 < words_by_seg.at(i).size());
        offset_of_data_zone_by_seg.at(i) = jr_ra_loc + 2;

      } else {
        // no functions
        offset_of_data_zone_by_seg.at(i) = 0;
      }

      // add label for debug purposes
      if(offset_of_data_zone_by_seg.at(i) < words_by_seg.at(i).size()) {
        auto data_label_id = get_label_id_for(i, 4 * (offset_of_data_zone_by_seg.at(i)));
        labels.at(data_label_id).name = "L-data-start";
      }

      // verify there are no functions after the data section starts
      for(size_t j = offset_of_data_zone_by_seg.at(i); j < words_by_seg.at(i).size(); j++) {
        auto& word = words_by_seg.at(i).at(j);
        if(word.kind == LinkedWord::TYPE_PTR && word.symbol_name == "function") {
          assert(false);
        }
      }

      // sizes:
      stats.data_bytes += 4 * (words_by_seg.at(i).size() - offset_of_data_zone_by_seg.at(i)) * 4;
      stats.code_bytes += 4 * offset_of_data_zone_by_seg.at(i);
    }
  } else {
    assert(false);
  }
}

/*!
 * Find all the functions in each segment.
 */
void LinkedObjectFile::find_functions() {
  if(segments == 1) {
    // it's a v2 file, shouldn't have any functions
    assert(offset_of_data_zone_by_seg.at(0) == 0);
  } else {

    // we assume functions don't have any data in between them, so we use the "function" type tag to mark the end
    // of the previous function and the start of the next.  This means that some functions will have a few
    // 0x0 words after then for padding (GOAL functions are aligned), but this is something that the disassembler
    // should handle.
    for(int seg = 0; seg < segments; seg++) {

      // start at the end and work backward...
      int function_end = offset_of_data_zone_by_seg.at(seg);
      while(function_end > 0) {

        // back up until we find function type tag
        int function_tag_loc = function_end;
        bool found_function_tag_loc = false;
        for(; function_tag_loc-- > 0;) {
          auto& word = words_by_seg.at(seg).at(function_tag_loc);
          if(word.kind == LinkedWord::TYPE_PTR && word.symbol_name == "function") {
            found_function_tag_loc = true;
            break;
          }
        }

        // mark this as a function, and try again from the current function start
        assert(found_function_tag_loc);
        stats.function_count++;
        functions_by_seg.at(seg).emplace_back(function_tag_loc, function_end);
        function_end = function_tag_loc;
      }

      std::reverse(functions_by_seg.at(seg).begin(), functions_by_seg.at(seg).end());
    }
  }
}

/*!
 * Run the disassembler on all functions.
 */
void LinkedObjectFile::disassemble_functions() {
  for(int seg = 0; seg < segments; seg++) {
    for(auto& function : functions_by_seg.at(seg)) {
      for(auto word = function.start_word; word < function.end_word; word++) {
        // decode!
        function.instructions.push_back(decode_instruction(words_by_seg.at(seg).at(word), *this, seg, word));
        if(function.instructions.back().is_valid()) {
          stats.decoded_ops++;
        }
      }
    }
  }
}

/*!
 * Analyze disassembly for use of the FP register, and add labels for fp-relative data access
 */
void LinkedObjectFile::process_fp_relative_links() {
  for(int seg = 0; seg < segments; seg++) {
    for(auto& function : functions_by_seg.at(seg)) {
      for(size_t instr_idx = 0; instr_idx < function.instructions.size(); instr_idx++) {

        // we possibly need to look at three instructions
        auto& instr = function.instructions[instr_idx];
        auto* prev_instr = (instr_idx > 0) ? &function.instructions[instr_idx - 1] : nullptr;
        auto* pprev_instr = (instr_idx > 1) ? &function.instructions[instr_idx - 2] : nullptr;

        // ignore storing FP onto the stack
        if((instr.kind == InstructionKind::SD || instr.kind == InstructionKind::SQ)
         &&instr.get_src(0).get_reg() == Register(Reg::GPR, Reg::FP)
         ) {
          continue;
        }


        // HACKs
        if(instr.kind == InstructionKind::PEXTLW) {
          continue;
        }

        // search over instruction sources
        for(int i = 0; i < instr.n_src; i++) {
          auto& src = instr.src[i];
          if(
             src.kind == InstructionAtom::REGISTER   // must be reg
            && src.get_reg().get_kind() == Reg::GPR    // gpr
            && src.get_reg().get_gpr() == Reg::FP) {   // fp reg.


            stats.n_fp_reg_use++;

            // offset of fp at this instruction.
            int current_fp = 4 * (function.start_word + 1);
            function.uses_fp_register = true;

            switch(instr.kind) {
              // fp-relative load
              case InstructionKind::LW:
              case InstructionKind::LWC1:
              case InstructionKind::LD:
              // generate pointer to fp-relative data
              case InstructionKind::DADDIU:
              {
                auto& atom = instr.get_imm_src();
                atom.set_label(get_label_id_for(seg, current_fp + atom.get_imm()));
                stats.n_fp_reg_use_resolved++;
              }
                break;

              // in the case that addiu doesn't have enough range (+/- 2^15), GOAL has two strategies:
              // 1). use ori + daddu (ori doesn't sign extend, so this lets us go +2^16, -0)
              // 2). use lui + ori + daddu (can reach anywhere in the address space)
              // It seems that addu is used to get pointers to floating point values and daddu is used in other cases.
              // Also, the position of the fp register is swapped between the two.
              case InstructionKind::DADDU:
              case InstructionKind::ADDU:
              {
                assert(prev_instr);
                assert(prev_instr->kind == InstructionKind::ORI);
                int offset_reg_src_id = instr.kind == InstructionKind::DADDU ? 0 : 1;
                auto offset_reg = instr.get_src(offset_reg_src_id).get_reg();
                assert(offset_reg == prev_instr->get_dst(0).get_reg());
                assert(offset_reg == prev_instr->get_src(0).get_reg());
                auto& atom = prev_instr->get_imm_src();
                int additional_offset = 0;
                if(pprev_instr && pprev_instr->kind == InstructionKind::LUI) {
                  assert(pprev_instr->get_dst(0).get_reg() == offset_reg);
                  additional_offset = (1 << 16) * pprev_instr->get_imm_src().get_imm();
                }
                atom.set_label(get_label_id_for(seg, current_fp + atom.get_imm() + additional_offset));
                stats.n_fp_reg_use_resolved++;
              }
                break;

              default:
                printf("unknown fp using op: %s\n", instr.to_string(*this).c_str());
                assert(false);
            }
          }
        }
      }
    }
  }
}

/*!
 * Print disassembled functions and data segments.
 */
std::string LinkedObjectFile::print_disassembly() {
  std::string result;

  assert(segments <= 3);
  for(int seg = segments; seg-- > 0;) {
    // segment header
    result += ";------------------------------------------\n;  ";
    result += segment_names[seg];
    result += "\n;------------------------------------------\n";

    // functions
    for(auto& func : functions_by_seg.at(seg)) {
      result += ";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n";
      result += "; .function " + func.guessed_name + "\n";

      // print each instruction in the function.
      bool in_delay_slot = false;

      for(int i = 1; i < func.end_word - func.start_word; i++) {
        auto label_id = get_label_at(seg, (func.start_word + i)*4);
        if(label_id != -1) {
          result += labels.at(label_id).name + ":\n";
        }

        for(int j = 1; j < 4; j++) {
//          assert(get_label_at(seg, (func.start_word + i)*4 + j) == -1);
          if(get_label_at(seg, (func.start_word + i)*4 + j) != -1) {
            result += "BAD OFFSET LABEL: ";
            result += labels.at(get_label_at(seg, (func.start_word + i)*4 + j)).name + "\n";
            assert(false);
          }
        }

        auto& instr = func.instructions.at(i);
        std::string line = "    " + instr.to_string(*this);

        if(line.length() < 60) {
          line.append(60 - line.length(), ' ');
        }
        result += line;
        result += " ;;";
        auto& word = words_by_seg[seg].at(func.start_word + i);
        append_word_to_string(result, word);

        if(in_delay_slot) {
          result += "\n";
          in_delay_slot = false;
        }

        if(gOpcodeInfo[(int)instr.kind].has_delay_slot) {
          in_delay_slot = true;
        }
      }
    }

    // print data
    for(size_t i = offset_of_data_zone_by_seg.at(seg); i < words_by_seg.at(seg).size(); i++) {
      for(int j = 0; j < 4; j++) {
        auto label_id = get_label_at(seg, i*4 + j);
        if(label_id != -1) {
          result += labels.at(label_id).name + ":";
          if(j != 0) {
            result += " (offset " + std::to_string(j) + ")";
          }
          result += "\n";
        }
      }

      auto& word = words_by_seg[seg][i];
      append_word_to_string(result, word);
    }
  }

  return result;
}

/*!
 * Return true if the object file contains any functions at all.
 */
bool LinkedObjectFile::has_any_functions() {
  for(auto& fv : functions_by_seg) {
    if(!fv.empty()) return true;
  }
  return false;
}