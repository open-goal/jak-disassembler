#include <cassert>
#include "Disasm/InstructionMatching.h"
#include "ObjectFile/LinkedObjectFile.h"
#include "CfgVtx.h"
#include "Function.h"

/////////////////////////////////////////
/// CfgVtx
/////////////////////////////////////////

/*!
 * Make this vertex a child vertex of new_parent!
 */
void CfgVtx::parent_claim(CfgVtx *new_parent) {
  parent = new_parent;

  // clear out all this junk - we don't need it now that we are a part of the "real" CFG!
  next = nullptr;
  prev = nullptr;
  pred.clear();
  succ_ft = nullptr;
  succ_branch = nullptr;
}

/*!
 * Replace reference to old_pred as a predecessor with new_pred.
 * Errors if old_pred wasn't referenced.
 */
void CfgVtx::replace_pred_and_check(CfgVtx *old_pred, CfgVtx *new_pred) {
  bool replaced = false;
  for (auto& x : pred) {
    if (x == old_pred) {
      assert(!replaced);
      x = new_pred;
      replaced = true;
    }
  }
  assert(replaced);
}

/*!
 * Replace references to old_succ with new_succ in the successors.
 * Errors if old_succ wasn't replaced.
 */
void CfgVtx::replace_succ_and_check(CfgVtx *old_succ, CfgVtx *new_succ) {
  bool replaced = false;
  if(succ_branch == old_succ) {
    succ_branch = new_succ;
    replaced = true;
  }

  if(succ_ft == old_succ) {
    succ_ft = new_succ;
    replaced = true;
  }

  assert(replaced);
}

/*!
 * Replace references to old_preds with a single new_pred.
 * Doesn't insert duplicates.
 * Error if all old preds aren't found.
 */
void CfgVtx::replace_preds_with_and_check(std::vector<CfgVtx *> old_preds, CfgVtx *new_pred) {
  std::vector<bool> found(old_preds.size(), false);

  std::vector<CfgVtx*> new_pred_list;

  for(auto* existing_pred : pred) {
    bool match = false;
    size_t idx = -1;
    for(size_t i = 0; i < old_preds.size(); i++) {
      if(existing_pred == old_preds[i]) {
        assert(!match);
        idx = i;
        match = true;
      }
    }

    if(match) {
      found.at(idx) = true;
    } else {
      new_pred_list.push_back(existing_pred);
    }
  }

  new_pred_list.push_back(new_pred);
  pred = new_pred_list;

  for(auto x : found) {
    assert(x);
  }
}

/////////////////////////////////////////
/// VERTICES
/////////////////////////////////////////

std::string BlockVtx::to_string() {
  return "Block " + std::to_string(block_id);
}

std::shared_ptr<Form> BlockVtx::to_form() {
  return toForm("b" + std::to_string(block_id));
}

std::string SequenceVtx::to_string() {
  assert(!seq.empty());
  // todo - this is not a great way to print it. Maybe sequences should have an ID or name?
  std::string result = "Seq " + seq.front()->to_string() + " ... " + seq.back()->to_string();
  return result;
}

std::shared_ptr<Form> SequenceVtx::to_form() {
  std::vector<std::shared_ptr<Form>> forms;
  forms.push_back(toForm("seq"));
  for (auto* x : seq) {
    forms.push_back(x->to_form());
  }
  return buildList(forms);
}

std::string EntryVtx::to_string() {
  return "ENTRY";
}

std::shared_ptr<Form> EntryVtx::to_form() {
  return toForm("entry");
}

std::string ExitVtx::to_string() {
  return "EXIT";
}

std::shared_ptr<Form> ExitVtx::to_form() {
  return toForm("exit");
}

std::shared_ptr<Form> IfElseVtx::to_form() {
  std::vector<std::shared_ptr<Form>> forms = {toForm("if"), condition->to_form(),
                                              true_case->to_form(), false_case->to_form()};
  return buildList(forms);
}

std::string IfElseVtx::to_string() {
  return "if_else";  // todo - something nicer
}

ControlFlowGraph::ControlFlowGraph() {
  // allocate the entry and exit vertices.
  m_entry = alloc<EntryVtx>();
  m_exit = alloc<ExitVtx>();
}

ControlFlowGraph::~ControlFlowGraph() {
  for (auto* x : m_node_pool) {
    delete x;
  }
}

/*!
 * Convert unresolved portion of CFG into a format that can be read by dot, a graph layout tool.
 * This is intended to help with debugging why a cfg couldn't be resolved.
 */
std::string ControlFlowGraph::to_dot() {
  std::string result = "digraph G {\n";
  std::string invis;
  for (auto* node : m_node_pool) {
    if (!node->parent) {
      auto me = "\"" + node->to_string() + "\"";
      if (!invis.empty()) {
        invis += " -> ";
      }
      invis += me;
      result += me + ";\n";

      // it's a top level node
      for (auto* s : node->succs()) {
        result += me + " -> \"" + s->to_string() + "\";\n";
      }
    }
  }
  result += "\n" + invis + " [style=invis];\n}\n";
  return result;
}

/*!
 * Is this CFG fully resolved?  Did we succeed in decoding the control flow?
 */
bool ControlFlowGraph::is_fully_resolved() {
  return get_top_level_vertices_count() == 1;
}

/*!
 * How many top level vertices are there?  Doesn't count entry and exit.
 */
int ControlFlowGraph::get_top_level_vertices_count() {
  int count = 0;
  for (auto* x : m_node_pool) {
    if (!x->parent && x != entry() && x != exit()) {
      count++;
    }
  }
  return count;
}

/*!
 * Get the top level vertex. Only safe to call if we are fully resolved.
 */
CfgVtx* ControlFlowGraph::get_single_top_level() {
  assert(get_top_level_vertices_count() == 1);
  for (auto* x : m_node_pool) {
    if (!x->parent && x != entry() && x != exit()) {
      return x;
    }
  }
  assert(false);
  return nullptr;
}

/*!
 * Turn into a form. If fully resolved, prints the nested control flow. Otherwise puts all the
 * ungrouped stuff into an "(ungrouped ...)" form and prints that.
 */
std::shared_ptr<Form> ControlFlowGraph::to_form() {
  if (get_top_level_vertices_count() == 1) {
    return get_single_top_level()->to_form();
  } else {
    std::vector<std::shared_ptr<Form>> forms = {toForm("ungrouped")};
    for (auto* x : m_node_pool) {
      if (!x->parent && x != entry() && x != exit()) {
        forms.push_back(x->to_form());
      }
    }
    return buildList(forms);
  }
}

/*!
 * Turn into a string. If fully resolved, prints the nested control flow. Otherwise puts all the
 * ungrouped stuff into an "(ungrouped ...)" form and prints that.
 */
std::string ControlFlowGraph::to_form_string() {
  return to_form()->toStringPretty();
}

// bool ControlFlowGraph::compact_top_level() {
//  int compact_count = 0;
//
//  std::string orig = to_dot();
//
//  while (compact_one_in_top_level()) {
//    compact_count++;
//  }
//
//  if (compact_count) {
//    printf("%s\nCHANGED TO\n%s\n", orig.c_str(), to_dot().c_str());
//    return true;
//  }
//
//  return false;
//}
//
// bool ControlFlowGraph::compact_one_in_top_level() {
//  for (auto* node : m_node_pool) {
//    if (node->parent_container) {
//      continue;
//    }
//
//    if (node != entry() && node->succ.size() == 1 && !node->has_succ(exit()) &&
//        node->succ.front()->pred.size() == 1 && !node->succ.front()->has_succ(node)) {
//      // can compact!
//      auto first = node;
//      auto second = node->succ.front();
//      assert(second->has_pred(first));
//
//      make_sequence(first, second);
//      return true;
//    }
//  }
//
//  return false;
//}
//

/*!
 * Do these 4 vertices make up an if else statement?
 * Vertices can be nullptr, in which case it always return false.
 * They should be in the order the blocks appear in memory (this is checked just in case)
 */
bool ControlFlowGraph::is_if_else(CfgVtx *b0, CfgVtx *b1, CfgVtx *b2, CfgVtx *b3) {
  // check existance
  if(!b0 || !b1 || !b2 || !b3) return false;

  // check verts
  if(b0->next != b1) return false;
  if(b0->succ_ft != b1) return false;
  // b0 prev, pred don't care

  if(b1->prev != b0) return false;
  if(!b1->has_pred(b0)) return false;
  if(b1->pred.size() != 1) return false;
  if(b1->next != b2) return false;
  if(b1->succ_ft) return false;
  if(b1->succ_branch != b3) return false;

  if(b2->prev != b1) return false;
  if(!b2->has_pred(b0)) return false;
  if(b2->pred.size() != 1) return false;
  if(b2->next != b3) return false;
  if(b2->succ_branch) return false;
  if(b2->succ_ft != b3) return false;

  if(b3->prev != b2) return false;
  if(!b3->has_pred(b2)) return false;
  if(!b3->has_pred(b1)) return false;

  return true;
}

/*!
 * Find all if else statements in the top level.
 * TODO - will we accidentally find some cond statements like this?
 */
void ControlFlowGraph::find_if_else_top_level() {
  // Example:
  // B0:
  //  beq s7, v1, B2  ;; inverted branch condition (branch on condition not met)
  //  sll r0, r0, 0   ;; nop in delay slot
  // B1:
  //  true case!
  //  beq r0, r0, B3  ;; unconditional branch
  //  sll r0, r0, 0   ;; nop in delay slot
  // B2:
  //  false case!     ;; fall through
  // B3:
  //  rest of code

  bool needs_work = true;
  while(needs_work) {
    needs_work = false; // until we change something, assume we're done.

    for_each_top_level_vtx([&](CfgVtx* vtx) {
      // return true means "try again with a different vertex"
      // return false means "I changed something, bail out so we can start from the beginning again."

      // attempt to match b0, b1, b2, b3
      auto* b0 = vtx;
      auto* b1 = vtx->succ_ft;
      auto* b2 = vtx->succ_branch;
      auto* b3 = b2 ? b2->succ_ft : nullptr;

      if(is_if_else(b0, b1, b2, b3)) {
        needs_work = true;

        // create the new vertex!
        auto* new_vtx = alloc<IfElseVtx>();
        new_vtx->condition = b0;
        new_vtx->true_case = b1;
        new_vtx->false_case = b2;

        // link new vertex pred
        for(auto* new_pred : b0->pred) {
          new_pred->replace_succ_and_check(b0, new_vtx);
        }
        new_vtx->pred = b0->pred;

        // link new vertex succ
        b3->replace_preds_with_and_check({b1, b2}, new_vtx);
        new_vtx->succ_ft = b3;

        // setup next/prev
        new_vtx->prev = b0->prev;
        new_vtx->next = b3;

        b0->parent_claim(new_vtx);
        b1->parent_claim(new_vtx);
        b2->parent_claim(new_vtx);

        return false;
      } else {
        return true; // try again!
      }
    });
  }
}

/*!
 * Find sequences.
 * To generate more readable debug output, we should aim to run this as infrequent and as
 * late as possible, to avoid condition vertices with tons of extra junk packed in.
 */
void ControlFlowGraph::find_seq_top_level() {

}


// SequenceVtx* ControlFlowGraph::make_sequence(CfgVtx* first, CfgVtx* second) {
//  // check that this makes sense to do
//  assert(first->succ.size() == 1);
//  assert(first->has_succ(second));
//  assert(second->pred.size() == 1);
//  assert(second->has_pred(first));
//  assert(first->parent_container == second->parent_container);
//
//  auto* first_as_seq = dynamic_cast<SequenceVtx*>(first);
//  auto* second_as_seq = dynamic_cast<SequenceVtx*>(second);
//
//  if (first_as_seq && second_as_seq) {
//    assert(false);  // todo
//    return nullptr;
//  } else if (first_as_seq) {
//    // fixup parent
//    second->parent_container = first_as_seq;
//    assert(!first_as_seq->seq.empty());
//    assert(first_as_seq->seq.back()->succ.empty());
//    assert(first_as_seq->seq.front()->pred.empty());
//
//    // fixup first <-> second
//    first_as_seq->seq.back()->pred.push_back(second);
//    second->pred.clear();
//    second->pred.push_back(first_as_seq->seq.back());
//    first_as_seq->seq.push_back(second);
//
//    // fixup second -> thing
//    first_as_seq->succ.clear();
//    first_as_seq->succ = second->succ;
//
//    for (auto& succ : first_as_seq->succ) {
//      replace_exactly_one_in(succ->pred, second, first);
//    }
//
//    second->succ.clear();
//    return first_as_seq;
//  } else if (second_as_seq) {
//    assert(false);
//    return nullptr;
//  } else {
//    auto* new_seq = alloc<SequenceVtx>();
//    new_seq->parent_container = first->parent_container;
//    first->parent_container = new_seq;
//    second->parent_container = new_seq;
//    new_seq->seq.push_back(first);
//    new_seq->seq.push_back(second);
//    new_seq->pred = first->pred;
//    new_seq->succ = second->succ;
//    first->pred.clear();
//    second->succ.clear();
//
//    for (auto& succ : new_seq->succ) {
//      replace_exactly_one_in(succ->pred, second, (CfgVtx*)new_seq);
//    }
//
//    for (auto& pred : new_seq->pred) {
//      replace_exactly_one_in(pred->succ, first, (CfgVtx*)new_seq);
//    }
//
//    return new_seq;
//  }
//}

/*!
 * Create vertices for basic blocks.  Should only be called once to create all blocks at once.
 * Will set up the next/prev relation for all of them, but not the pred/succ.
 * The returned vector will have blocks in ordered, so the i-th entry is for the i-th block.
 */
const std::vector<BlockVtx*>& ControlFlowGraph::create_blocks(int count) {
  assert(m_blocks.empty());
  BlockVtx* prev = nullptr;  // for linking next/prev

  for (int i = 0; i < count; i++) {
    auto* new_block = alloc<BlockVtx>(i);

    // link next/prev
    new_block->prev = prev;
    if (prev) {
      prev->next = new_block;
    }
    prev = new_block;

    m_blocks.push_back(new_block);
  }

  return m_blocks;
}

/*!
 * Setup pred/succ for a block which falls through to the next.
 */
void ControlFlowGraph::link_fall_through(BlockVtx* first, BlockVtx* second) {
  assert(!first->succ_ft);  // don't want to overwrite something by accident.
  // can only fall through to the next code in memory.
  assert(first->next == second);
  assert(second->prev == first);
  first->succ_ft = second;

  if (!second->has_pred(first)) {
    // if a block can block fall through and branch to the same block, we want to avoid adding
    // it as a pred twice. This is rare, but does happen and makes sense with likely branches
    // which only run the delay slot when taken.
    second->pred.push_back(first);
  }
}

/*!
 * Setup pred/succ for a block which branches to second.
 */
void ControlFlowGraph::link_branch(BlockVtx* first, BlockVtx* second) {
  assert(!first->succ_branch);

  first->succ_branch = second;
  if (!second->has_pred(first)) {
    // see comment in link_fall_through
    second->pred.push_back(first);
  }
}

/*!
 * Build and resolve a Control Flow Graph as much as possible.
 */
std::shared_ptr<ControlFlowGraph> build_cfg(const LinkedObjectFile& file,
                                            int seg,
                                            const Function& func) {
  auto cfg = std::make_shared<ControlFlowGraph>();

  const auto& blocks = cfg->create_blocks(func.basic_blocks.size());

  // add entry block
  cfg->entry()->succ_ft = blocks.front();
  blocks.front()->pred.push_back(cfg->entry());

  // add exit block
  cfg->exit()->pred.push_back(blocks.back());
  blocks.back()->succ_ft = cfg->exit();

  // todo - early returns!

  // set up succ / pred
  for (int i = 0; i < int(func.basic_blocks.size()); i++) {
    auto& b = func.basic_blocks[i];
    bool not_last = (i + 1) < int(func.basic_blocks.size());

    if (b.end_word - b.start_word < 2) {
      // there's no room for a branch here, fall through to the end
      if (not_last) {
        cfg->link_fall_through(blocks.at(i), blocks.at(i + 1));
      }
    } else {
      // might be a branch
      int idx = b.end_word - 2;
      assert(idx >= b.start_word);
      auto& branch_candidate = func.instructions.at(idx);

      if (is_branch(branch_candidate, {})) {
        blocks.at(i)->has_branch = true;
        blocks.at(i)->branch_likely = is_branch(branch_candidate, true);
        bool branch_always = is_always_branch(branch_candidate);

        // need to find block target
        int block_target = -1;
        int label_target = branch_candidate.get_label_target();
        assert(label_target != -1);
        const auto& label = file.labels.at(label_target);
        assert(label.target_segment == seg);
        assert((label.offset % 4) == 0);
        int offset = label.offset / 4 - func.start_word;
        assert(offset >= 0);

        // the order here matters when there are zero size blocks. Unclear what the best answer is.
        //  i think in end it doesn't actually matter??
        //        for (int j = 0; j < int(func.basic_blocks.size()); j++) {
        for (int j = int(func.basic_blocks.size()); j-- > 0;) {
          if (func.basic_blocks[j].start_word == offset) {
            block_target = j;
            break;
          }
        }

        assert(block_target != -1);
        cfg->link_branch(blocks.at(i), blocks.at(block_target));

        if (branch_always) {
          // don't continue to the next one
          blocks.at(i)->branch_always = true;
        } else {
          // not an always branch
          if (not_last) {
            cfg->link_fall_through(blocks.at(i), blocks.at(i + 1));
          }
        }
      } else {
        // not a branch at all
        if (not_last) {
          cfg->link_fall_through(blocks.at(i), blocks.at(i + 1));
        }
      }
    }
  }

  cfg->find_if_else_top_level();
  return cfg;
}
