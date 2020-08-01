#include <cassert>
#include <vector>
#include "Function.h"
#include "Disasm/InstructionMatching.h"

namespace {
std::vector<Register> gpr_backups = {make_gpr(Reg::GP), make_gpr(Reg::S5), make_gpr(Reg::S4),
                                     make_gpr(Reg::S3), make_gpr(Reg::S2), make_gpr(Reg::S1),
                                     make_gpr(Reg::S0)};

std::vector<Register> fpr_backups = {make_fpr(30), make_fpr(28), make_fpr(26),
                                     make_fpr(24), make_fpr(22), make_fpr(20)};

Register get_expected_gpr_backup(int n, int total) {
  assert(total <= int(gpr_backups.size()));
  assert(n < total);
  return gpr_backups.at((total - 1) - n);
}

Register get_expected_fpr_backup(int n, int total) {
  assert(total <= int(fpr_backups.size()));
  assert(n < total);
  return fpr_backups.at((total - 1) - n);
}

uint32_t align16(uint32_t in) {
  return (in + 15) & (~15);
}

uint32_t align8(uint32_t in) {
  return (in + 7) & (~7);
}

uint32_t align4(uint32_t in) {
  return (in + 3) & (~3);
}

}  // namespace

Function::Function(int _start_word, int _end_word) : start_word(_start_word), end_word(_end_word) {}

/*!
 * Remove the function prologue from the first basic block and populate this->prologue with info.
 */
void Function::analyze_prologue(const LinkedObjectFile& file) {
  int idx = 1;

  // first we look for daddiu sp, sp, -x to determine how much stack is used
  if (is_gpr_2_imm(instructions.at(idx), InstructionKind::DADDIU, make_gpr(Reg::SP),
                   make_gpr(Reg::SP), {})) {
    prologue.total_stack_usage = -instructions.at(idx).get_imm_src_int();
    idx++;
  } else {
    prologue.total_stack_usage = 0;
  }

  // don't include type tag
  prologue_end = 1;

  // if we use the stack, we may back up some registers onto it
  if (prologue.total_stack_usage) {
    // heuristics to detect asm functions
    {
      auto& instr = instructions.at(idx);
      // storing stack pointer on the stack is done by some ASM kernel functions
      if (instr.kind == InstructionKind::SW && instr.get_src(0).get_reg() == make_gpr(Reg::SP)) {
        printf("[Warning] Suspected ASM function based on this instruction in prologue: %s\n",
               instr.to_string(file).c_str());
        warnings += "Flagged as ASM function because of " + instr.to_string(file) + "\n";
        suspected_asm = true;
        return;
      }
    }

    // ra backup is always first
    if (is_no_link_gpr_store(instructions.at(idx), 8, Register(Reg::GPR, Reg::RA), {},
                             Register(Reg::GPR, Reg::SP))) {
      prologue.ra_backed_up = true;
      prologue.ra_backup_offset = get_gpr_store_offset(instructions.at(idx));
      assert(prologue.ra_backup_offset == 0);
      idx++;
    }

    {
      auto& instr = instructions.at(idx);

      // storing s7 on the stack is done by interrupt handlers, which we probably don't want to
      // support
      if (instr.kind == InstructionKind::SD && instr.get_src(0).get_reg() == make_gpr(Reg::S7)) {
        printf("[Warning] Suspected ASM function based on this instruction in prologue: %s\n",
               instr.to_string(file).c_str());
        warnings += "Flagged as ASM function because of " + instr.to_string(file) + "\n";
        suspected_asm = true;
        return;
      }
    }

    // next is fp backup
    if (is_no_link_gpr_store(instructions.at(idx), 8, Register(Reg::GPR, Reg::FP), {},
                             Register(Reg::GPR, Reg::SP))) {
      prologue.fp_backed_up = true;
      prologue.fp_backup_offset = get_gpr_store_offset(instructions.at(idx));
      // in Jak 1 like we never backup fp unless ra is also backed up, so the offset is always 8.
      // but it seems like it could be possible to do one without the other?
      assert(prologue.fp_backup_offset == 8);
      idx++;

      // after backing up fp, we always set it to t9.
      prologue.fp_set = is_gpr_3(instructions.at(idx), InstructionKind::OR, make_gpr(Reg::FP),
                                 make_gpr(Reg::T9), make_gpr(Reg::R0));
      assert(prologue.fp_set);
      idx++;
    }

    // next is gpr backups. these are in reverse order, so we should first find the length
    // GOAL will always do the exact same thing when the same number of gprs needs to be backed up
    // so we just need to determine the number of GPR backups, and we have all the info we need
    int n_gpr_backups = 0;
    int gpr_idx = idx;
    bool expect_nothing_after_gprs = false;

    while (is_no_link_gpr_store(instructions.at(gpr_idx), 16, {}, {}, make_gpr(Reg::SP))) {
      auto store_reg = instructions.at(gpr_idx).get_src(0).get_reg();

      // sometimes stack memory is zeroed immediately after gpr backups, and this fools the previous
      // check.
      if (store_reg == make_gpr(Reg::R0)) {
        printf(
            "[Warning] Stack Zeroing Detected in Function::analyze_prologue, prologue may be "
            "wrong\n");
        warnings += "Stack Zeroing Detected, prologue may be wrong\n";
        expect_nothing_after_gprs = true;
        break;
      }

      // this also happens a few times per game.  this a0/r0 check seems to be all that's needed to
      // avoid false positives here!
      if (store_reg == make_gpr(Reg::A0)) {
        suspected_asm = true;
        printf("[Warning] Suspected ASM function because register $a0 was stored on the stack!\n");
        warnings += "a0 on stack detected, flagging as asm\n";
        return;
      }

      n_gpr_backups++;
      gpr_idx++;
    }

    if (n_gpr_backups) {
      prologue.gpr_backup_offset = get_gpr_store_offset(instructions.at(idx));
      for (int i = 0; i < n_gpr_backups; i++) {
        int this_offset = get_gpr_store_offset(instructions.at(idx + i));
        auto this_reg = instructions.at(idx + i).get_src(0).get_reg();
        assert(this_offset == prologue.gpr_backup_offset + 16 * i);
        if (this_reg != get_expected_gpr_backup(i, n_gpr_backups)) {
          suspected_asm = true;
          printf("[Warning] Suspected asm function that isn't flagged due to stack store %s\n",
                 instructions.at(idx + i).to_string(file).c_str());
          warnings += "Suspected asm function due to stack store: " +
                      instructions.at(idx + i).to_string(file) + "\n";
          return;
        }
      }
    }
    prologue.n_gpr_backup = n_gpr_backups;
    idx = gpr_idx;

    int n_fpr_backups = 0;
    int fpr_idx = idx;
    if (!expect_nothing_after_gprs) {
      // FPR backups
      while (is_no_link_fpr_store(instructions.at(fpr_idx), {}, {}, make_gpr(Reg::SP))) {
        // auto store_reg = instructions.at(gpr_idx).get_src(0).get_reg();
        n_fpr_backups++;
        fpr_idx++;
      }

      if (n_fpr_backups) {
        prologue.fpr_backup_offset = instructions.at(idx).get_src(1).get_imm();
        for (int i = 0; i < n_fpr_backups; i++) {
          int this_offset = instructions.at(idx + i).get_src(1).get_imm();
          auto this_reg = instructions.at(idx + i).get_src(0).get_reg();
          assert(this_offset == prologue.fpr_backup_offset + 4 * i);
          if (this_reg != get_expected_fpr_backup(i, n_fpr_backups)) {
            suspected_asm = true;
            printf("[Warning] Suspected asm function that isn't flagged due to stack store %s\n",
                   instructions.at(idx + i).to_string(file).c_str());
            warnings += "Suspected asm function due to stack store: " +
                        instructions.at(idx + i).to_string(file) + "\n";
            return;
          }
        }
      }
    }
    prologue.n_fpr_backup = n_fpr_backups;
    idx = fpr_idx;

    prologue_start = 1;
    prologue_end = idx;

    prologue.stack_var_offset = 0;
    if (prologue.ra_backed_up) {
      prologue.stack_var_offset = 8;
    }
    if (prologue.fp_backed_up) {
      prologue.stack_var_offset = 16;
    }

    if (n_gpr_backups == 0 && n_fpr_backups == 0) {
      prologue.n_stack_var_bytes = prologue.total_stack_usage - prologue.stack_var_offset;
    } else if (n_gpr_backups == 0) {
      // fprs only
      prologue.n_stack_var_bytes = prologue.fpr_backup_offset - prologue.stack_var_offset;
    } else if (n_fpr_backups == 0) {
      // gprs only
      prologue.n_stack_var_bytes = prologue.gpr_backup_offset - prologue.stack_var_offset;
    } else {
      // both, use gprs
      assert(prologue.fpr_backup_offset > prologue.gpr_backup_offset);
      prologue.n_stack_var_bytes = prologue.gpr_backup_offset - prologue.stack_var_offset;
    }

    assert(prologue.n_stack_var_bytes >= 0);

    // check that the stack lines up by going in order

    // RA backup
    int total_stack = 0;
    if (prologue.ra_backed_up) {
      total_stack = align8(total_stack);
      assert(prologue.ra_backup_offset == total_stack);
      total_stack += 8;
    }

    if (!prologue.ra_backed_up && prologue.fp_backed_up) {
      // GOAL does this for an unknown reason.
      total_stack += 8;
    }

    // FP backup
    if (prologue.fp_backed_up) {
      total_stack = align8(total_stack);
      assert(prologue.fp_backup_offset == total_stack);
      total_stack += 8;
      assert(prologue.fp_set);
    }

    // Stack Variables
    if (prologue.n_stack_var_bytes) {
      // no alignment because we don't know how the stack vars are aligned.
      // stack var padding counts toward this section.
      assert(prologue.stack_var_offset == total_stack);
      total_stack += prologue.n_stack_var_bytes;
    }

    // GPRS
    if (prologue.n_gpr_backup) {
      total_stack = align16(total_stack);
      assert(prologue.gpr_backup_offset == total_stack);
      total_stack += 16 * prologue.n_gpr_backup;
    }

    // FPRS
    if (prologue.n_fpr_backup) {
      total_stack = align4(total_stack);
      assert(prologue.fpr_backup_offset == total_stack);
      total_stack += 4 * prologue.n_fpr_backup;
    }

    total_stack = align16(total_stack);

    // End!
    assert(prologue.total_stack_usage == total_stack);
  }

  // it's fine to have the entire first basic block be the prologue - you could loop back to the
  // first instruction past the prologue.
  assert(basic_blocks.at(0).end_word >= prologue_end);
  basic_blocks.at(0).start_word = prologue_end;
  prologue.decoded = true;

  check_epilogue(file);
}

std::string Function::Prologue::to_string(int indent) const {
  char buff[512];
  char* buff_ptr = buff;
  std::string indent_str(indent, ' ');
  if (!decoded) {
    return indent_str + "BAD PROLOGUE";
  }
  buff_ptr += sprintf(buff_ptr, "%sstack: total 0x%02x, fp? %d ra? %d ep? %d", indent_str.c_str(),
                      total_stack_usage, fp_set, ra_backed_up, epilogue_ok);
  if (n_stack_var_bytes) {
    buff_ptr += sprintf(buff_ptr, "\n%sstack_vars: %d bytes at %d", indent_str.c_str(),
                        n_stack_var_bytes, stack_var_offset);
  }
  if (n_gpr_backup) {
    buff_ptr += sprintf(buff_ptr, "\n%sgprs:", indent_str.c_str());
    for (int i = 0; i < n_gpr_backup; i++) {
      buff_ptr += sprintf(buff_ptr, " %s", gpr_backups.at(i).to_string().c_str());
    }
  }
  if (n_fpr_backup) {
    buff_ptr += sprintf(buff_ptr, "\n%sfprs:", indent_str.c_str());
    for (int i = 0; i < n_fpr_backup; i++) {
      buff_ptr += sprintf(buff_ptr, " %s", fpr_backups.at(i).to_string().c_str());
    }
  }
  return {buff};
}

void Function::check_epilogue(const LinkedObjectFile& file) {
  if (!prologue.decoded || suspected_asm) {
    printf("not decoded, or suspected asm, skipping epilogue\n");
    return;
  }

  // start at the end and move up.
  int idx = int(instructions.size()) - 1;

  // seek past alignment nops
  while (is_nop(instructions.at(idx))) {
    idx--;
  }

  epilogue_end = idx;
  // stack restore
  if (prologue.total_stack_usage) {
    // hack - sometimes an asm function has a compiler inserted jr ra/daddu sp sp r0 that follows
    // the "true" return.  We really should have this function flagged as asm, but for now, we can
    // simply skip over the compiler-generated jr ra/daddu sp sp r0.
    if (is_gpr_3(instructions.at(idx), InstructionKind::DADDU, make_gpr(Reg::SP), make_gpr(Reg::SP),
                 make_gpr(Reg::R0))) {
      idx--;
      assert(is_jr_ra(instructions.at(idx)));
      idx--;
      printf(
          "[Warning] Double Return Epilogue Hack!  This is probably an ASM function in disguise\n");
      warnings += "Double Return Epilogue - this is probably an ASM function\n";
    }
    // delay slot should be daddiu sp, sp, offset
    assert(is_gpr_2_imm(instructions.at(idx), InstructionKind::DADDIU, make_gpr(Reg::SP),
                        make_gpr(Reg::SP), prologue.total_stack_usage));
    idx--;
  } else {
    // delay slot is always daddu sp, sp, r0...
    assert(is_gpr_3(instructions.at(idx), InstructionKind::DADDU, make_gpr(Reg::SP),
                    make_gpr(Reg::SP), make_gpr(Reg::R0)));
    idx--;
  }

  // jr ra
  assert(is_jr_ra(instructions.at(idx)));
  idx--;

  // restore gprs
  for (int i = 0; i < prologue.n_gpr_backup; i++) {
    int gpr_idx = prologue.n_gpr_backup - (1 + i);
    const auto& expected_reg = gpr_backups.at(gpr_idx);
    auto expected_offset = prologue.gpr_backup_offset + 16 * i;
    assert(is_no_link_gpr_load(instructions.at(idx), 16, true, expected_reg, expected_offset,
                               make_gpr(Reg::SP)));
    idx--;
  }

  // restore fprs
  for (int i = 0; i < prologue.n_fpr_backup; i++) {
    int fpr_idx = prologue.n_fpr_backup - (1 + i);
    const auto& expected_reg = fpr_backups.at(fpr_idx);
    auto expected_offset = prologue.fpr_backup_offset + 4 * i;
    assert(is_no_link_fpr_load(instructions.at(idx), expected_reg, expected_offset,
                               make_gpr(Reg::SP)));
    idx--;
  }

  // restore fp
  if (prologue.fp_backed_up) {
    assert(is_no_link_gpr_load(instructions.at(idx), 8, true, make_gpr(Reg::FP),
                               prologue.fp_backup_offset, make_gpr(Reg::SP)));
    idx--;
  }

  // restore ra
  if (prologue.ra_backed_up) {
    assert(is_no_link_gpr_load(instructions.at(idx), 8, true, make_gpr(Reg::RA),
                               prologue.ra_backup_offset, make_gpr(Reg::SP)));
    idx--;
  }

  assert(!basic_blocks.empty());
  assert(idx + 1 >= basic_blocks.back().start_word);
  basic_blocks.back().end_word = idx + 1;
  prologue.epilogue_ok = true;
  epilogue_start = idx + 1;
}