#include <cassert>
#include "InstructionMatching.h"

bool is_no_link_instr(const Instruction& instr, MatchParam<InstructionKind> kind) {
  if (kind == instr.kind) {
    for (int i = 0; i < instr.n_src; i++) {
      if (instr.src[i].is_link_or_label()) {
        return false;
      }
    }

    for (int i = 0; i < instr.n_dst; i++) {
      if (instr.dst[i].is_link_or_label()) {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool is_no_link_gpr_store(const Instruction& instr,
                          MatchParam<int> size,
                          MatchParam<Register> src,
                          MatchParam<int> offset,
                          MatchParam<Register> dest) {
  // match the opcode
  if (!size.is_wildcard) {
    switch (size.value) {
      case 1:
        if (instr.kind != InstructionKind::SB) {
          return false;
        }
        break;
      case 2:
        if (instr.kind != InstructionKind::SH) {
          return false;
        }
        break;
      case 4:
        if (instr.kind != InstructionKind::SW) {
          return false;
        }
        break;
      case 8:
        if (instr.kind != InstructionKind::SD) {
          return false;
        }
        break;
      case 16:
        if (instr.kind != InstructionKind::SQ) {
          return false;
        }
        break;
      default:
        assert(false);
    }
  } else {
    // just make sure it's a gpr store
    if (!is_gpr_store(instr)) {
      return false;
    }
  }

  assert(instr.n_src == 3);

  // match other arguments
  return src == instr.src[0].get_reg() && offset == instr.src[1].get_imm() &&
         dest == instr.src[2].get_reg();
}

bool is_no_link_gpr_load(const Instruction& instr,
                         MatchParam<int> size,
                         MatchParam<bool> is_signed,
                         MatchParam<Register> dst_reg,
                         MatchParam<int> offset,
                         MatchParam<Register> mem_reg) {
  // match the opcode
  if (!size.is_wildcard) {
    if (is_signed.is_wildcard) {
      switch (size.value) {
        case 1:
          if (instr.kind != InstructionKind::LB && instr.kind != InstructionKind::LBU) {
            return false;
          }
          break;
        case 2:
          if (instr.kind != InstructionKind::LH && instr.kind != InstructionKind::LHU) {
            return false;
          }
          break;
        case 4:
          if (instr.kind != InstructionKind::LW && instr.kind != InstructionKind::LWU) {
            return false;
          }
          break;
        case 8:
          if (instr.kind != InstructionKind::LD) {
            return false;
          }
          break;
        case 16:
          if (instr.kind != InstructionKind::LQ) {
            return false;
          }
          break;
        default:
          assert(false);
      }
    } else {
      if (is_signed.value) {
        switch (size.value) {
          case 1:
            if (instr.kind != InstructionKind::LB) {
              return false;
            }
            break;
          case 2:
            if (instr.kind != InstructionKind::LH) {
              return false;
            }
            break;
          case 4:
            if (instr.kind != InstructionKind::LW) {
              return false;
            }
            break;
          case 8:
            if (instr.kind != InstructionKind::LD) {
              return false;
            }
            break;
          case 16:
            if (instr.kind != InstructionKind::LQ) {
              return false;
            }
            break;
          default:
            assert(false);
        }
      } else {
        switch (size.value) {
          case 1:
            if (instr.kind != InstructionKind::LBU) {
              return false;
            }
            break;
          case 2:
            if (instr.kind != InstructionKind::LHU) {
              return false;
            }
            break;
          case 4:
            if (instr.kind != InstructionKind::LWU) {
              return false;
            }
            break;
          case 8:
            if (instr.kind != InstructionKind::LD) {
              return false;
            }
            break;
          case 16:
            if (instr.kind != InstructionKind::LQ) {
              return false;
            }
            break;
          default:
            assert(false);
        }
      }
    }
  } else {
    // just make sure it's a gpr store
    if (!is_gpr_load(instr, is_signed)) {
      return false;
    }
  }

  // match other arguments
  return dst_reg == instr.get_dst(0).get_reg() && offset == instr.get_src(0).get_imm() &&
         mem_reg == instr.get_src(1).get_reg();
}

bool is_no_link_fpr_store(const Instruction& instr,
                          MatchParam<Register> src,
                          MatchParam<int> offset,
                          MatchParam<Register> dest) {
  return instr.kind == InstructionKind::SWC1 && src == instr.src[0].get_reg() &&
         offset == instr.src[1].get_imm() && dest == instr.src[2].get_reg();
}

bool is_no_link_fpr_load(const Instruction& instr,
                         MatchParam<Register> dst_reg,
                         MatchParam<int> offset,
                         MatchParam<Register> mem_reg) {
  return instr.kind == InstructionKind::LWC1 && dst_reg == instr.get_dst(0).get_reg() &&
         offset == instr.get_src(0).get_imm() && mem_reg == instr.get_src(1).get_reg();
}

namespace {
auto gpr_stores = {InstructionKind::SB, InstructionKind::SH, InstructionKind::SW,
                   InstructionKind::SD, InstructionKind::SQ};
auto gpr_signed_loads = {InstructionKind::LB, InstructionKind::LH, InstructionKind::LW,
                         InstructionKind::LD, InstructionKind::LQ};
auto gpr_unsigned_loads = {InstructionKind::LBU, InstructionKind::LHU, InstructionKind::LWU,
                           InstructionKind::LD, InstructionKind::LQ};
auto gpr_all_loads = {InstructionKind::LBU, InstructionKind::LB, InstructionKind::LH,
                      InstructionKind::LHU, InstructionKind::LW, InstructionKind::LWU,
                      InstructionKind::SD,  InstructionKind::SQ};
}  // namespace

bool is_gpr_store(const Instruction& instr) {
  for (auto x : gpr_stores) {
    if (instr.kind == x) {
      return true;
    }
  }
  return false;
}

bool is_gpr_load(const Instruction& instr, MatchParam<bool> is_signed) {
  if (is_signed.is_wildcard) {
    for (auto x : gpr_all_loads) {
      if (instr.kind == x) {
        return true;
      }
    }
    return false;
  } else if (is_signed.value) {
    for (auto x : gpr_signed_loads) {
      if (instr.kind == x) {
        return true;
      }
    }
    return false;
  } else {
    for (auto x : gpr_unsigned_loads) {
      if (instr.kind == x) {
        return true;
      }
    }
    return false;
  }
}

int32_t get_gpr_store_offset(const Instruction& instr) {
  assert(is_gpr_store(instr));
  assert(instr.n_src == 3);
  return instr.src[1].get_imm();
}

bool is_gpr_3(const Instruction& instr,
              MatchParam<InstructionKind> kind,
              MatchParam<Register> dst,
              MatchParam<Register> src0,
              MatchParam<Register> src1) {
  return kind == instr.kind && dst == instr.get_dst(0).get_reg() &&
         src0 == instr.get_src(0).get_reg() && src1 == instr.get_src(1).get_reg();
}

bool is_gpr_2_imm(const Instruction& instr,
                  MatchParam<InstructionKind> kind,
                  MatchParam<Register> dst,
                  MatchParam<Register> src,
                  MatchParam<int32_t> imm) {
  return kind == instr.kind && dst == instr.get_dst(0).get_reg() &&
         src == instr.get_src(0).get_reg() && imm == instr.get_src(1).get_imm();
}

Register make_gpr(Reg::Gpr gpr) {
  return Register(Reg::GPR, gpr);
}

Register make_fpr(int fpr) {
  return Register(Reg::FPR, fpr);
}

bool is_nop(const Instruction& instr) {
  return is_gpr_2_imm(instr, InstructionKind::SLL, make_gpr(Reg::R0), make_gpr(Reg::R0), 0);
}

bool is_jr_ra(const Instruction& instr) {
  return instr.kind == InstructionKind::JR && instr.get_src(0).get_reg() == make_gpr(Reg::RA);
}