#ifndef JAK_DISASSEMBLER_CFGVTX_H
#define JAK_DISASSEMBLER_CFGVTX_H

#include <string>
#include <vector>
#include <cassert>
#include "util/LispPrint.h"

/*!
 * In v, find an item equal to old, and replace it with replace.
 * Will throw an error is there is not exactly one thing equal to old.
 */
template <typename T>
void replace_exactly_one_in(std::vector<T>& v, T old, T replace) {
  bool replaced = false;
  for (auto& x : v) {
    if (x == old) {
      assert(!replaced);
      x = replace;
      replaced = true;
    }
  }
  assert(replaced);
}

/*!
 * Representation of a vertex in the control flow graph.
 *
 * The desired state of the control flow graph is to have a single "top-level" node, with NULL as
 * its parent. This top level node can then be viewed as the entire control flow for the function.
 * When the graph is fully understood, the only relation between vertices should be parent-child.
 * For example, an "if_else" vertex will have a "condition" vertex, "true_case" vertex, and "false
 * case" vertex as children.
 *
 * However, the initial state of the CFG is to have all the vertices be in the top level. When there
 * are multiple top level vertices, the graph is considered to be "unresolved", as there are
 * relations between these that are not explained by parent-child control structuring. These
 * relations are either pred/succ, indicating program control flow, and next/prev indicating code
 * layout order.  These are undesirable because these do not map to high-level program structure.
 *
 * The graph attempts to "resolve" itself, meaning these pred/succ relations are destroyed and
 * replaced with nested control flow. The pred/succ and next/prev relations should only exist at the
 * top level.
 *
 * Once resolved, there will be a single "top level" node containing the entire control flow
 * structure.
 *
 * All CfgVtxs should be created from the ControlFlowGraph::alloc function, which allocates them
 * from a pool and cleans them up when the ControlFlowGraph is destroyed.  This approach avoids
 * circular reference issues from a referencing counting approach, but does mean that temporary
 * allocations aren't cleaned up until the entire graph is deleted, but this is probably fine.
 *
 * Note - there are two special "top-level" vertices that are always present, called Entry and Exit.
 * These always exist and don't count toward making the graph unresolved.
 * These vertices won't be counted in the get_top_level_vertices_count.
 *
 * Desired end state of the graph:
 *       Entry -> some-top-level-control-flow-structure -> Exit
 */
class CfgVtx {
 public:
  virtual std::string to_string() = 0;          // convert to a single line string for debugging
  virtual std::shared_ptr<Form> to_form() = 0;  // recursive print as LISP form.
  virtual ~CfgVtx() = default;



  CfgVtx* parent = nullptr;       // parent structure, or nullptr if top level
  CfgVtx* succ_branch = nullptr;  // possible successor from branching, or NULL if no branch
  CfgVtx* succ_ft = nullptr;      // possible successor from falling through, or NULL if impossible
  CfgVtx* next = nullptr;         // next code in memory
  CfgVtx* prev = nullptr;         // previous code in memory
  std::vector<CfgVtx*> pred;      // all vertices which have us as succ_branch or succ_ft

  struct {
    bool has_branch = false;     // does the block end in a branch (any kind)?
    bool branch_likely = false;  // does the block end in a likely branch?
    bool branch_always = false;  // does the branch always get taken?
  } end_branch;


  // each child class of CfgVtx will define its own children.

  /*!
   * Do we have s as a successor?
   */
  bool has_succ(CfgVtx* s) const { return succ_branch == s || succ_ft == s; }

  /*!
   * Do we have p as a predecessor?
   */
  bool has_pred(CfgVtx* p) const {
    for (auto* x : pred) {
      if (x == p)
        return true;
    }
    return false;
  }

  /*!
   * Lazy function for getting all non-null succesors
   */
  std::vector<CfgVtx*> succs() {
    std::vector<CfgVtx*> result;
    if (succ_branch) {
      result.push_back(succ_branch);
    }
    if (succ_ft) {
      result.push_back(succ_ft);
    }
    return result;
  }

  void parent_claim(CfgVtx* new_parent);
  void replace_pred_and_check(CfgVtx* old_pred, CfgVtx* new_pred);
  void replace_succ_and_check(CfgVtx* old_succ, CfgVtx* new_succ);
  void replace_preds_with_and_check(std::vector<CfgVtx*> old_preds, CfgVtx* new_pred);

  std::string links_to_string();
};

/*!
 * Special Entry vertex representing the beginning of the function
 */
class EntryVtx : public CfgVtx {
 public:
  EntryVtx() = default;
  std::shared_ptr<Form> to_form() override;
  std::string to_string() override;
};

/*!
 * Special Exit vertex representing the end of the function
 */
class ExitVtx : public CfgVtx {
 public:
  std::string to_string() override;
  std::shared_ptr<Form> to_form() override;
};

/*!
 * A vertex which represents a single basic block. It has no children.
 */
class BlockVtx : public CfgVtx {
 public:
  explicit BlockVtx(int id) : block_id(id) {}
  std::string to_string() override;
  std::shared_ptr<Form> to_form() override;
  int block_id = -1;           // which block are we?
};

/*!
 * A vertex representing a sequence of child vertices which are always represented in order.
 * Child vertices in here don't set their next/prev pred/succ pointers as this counts as resolved.
 */
class SequenceVtx : public CfgVtx {
 public:
  std::string to_string() override;
  std::shared_ptr<Form> to_form() override;
  std::vector<CfgVtx*> seq;
};

/*!
 * A vertex representing an "if else" statement. This is a statement with a single condition,
 * a vertex which taken if true, and a vertex which is take if false.
 *
 * The current behavior is that the condition vertex grabs an entire vertex
 * as condition, but attempts to keep this condition vertex as small as possible. However, if
 * the condition does not begin on a block boundary, it will still grab too much.
 *
 * Example:
 * (set! x 1)
 * (set! y 2)
 * (if (< x y) (do-thing) (do-other-thing))
 *
 * it will turn into
 *
 * (if (begin (set! x 1) (set! x 2) (< x y)) (do-thing) (do-other-thing))
 *

 * A later step will have to figure out how to "simplify" the branch condition. But we don't
 * have enough information to do this safely now.  Likely this can be included in the s-expression
 map part, as statement generation should figure out when we are cramming a (begin ) form into a
 condition where it probably doesn't make sense.
 */
class IfElseVtx : public CfgVtx {
 public:
  std::string to_string() override;
  std::shared_ptr<Form> to_form() override;

  CfgVtx* condition = nullptr;  // this can be "shared" with the block before it.
  CfgVtx* true_case = nullptr;
  CfgVtx* false_case = nullptr;
};

class WhileLoop : public CfgVtx {
 public:
  std::string to_string() override;
  std::shared_ptr<Form> to_form() override;

  CfgVtx* condition = nullptr;
  CfgVtx* body = nullptr;
};

/*!
 * The actual CFG class, which owns all the vertices.
 */
class ControlFlowGraph {
 public:
  ControlFlowGraph();
  ~ControlFlowGraph();

  std::shared_ptr<Form> to_form();
  std::string to_form_string();
  std::string to_dot();
  int get_top_level_vertices_count();
  bool is_fully_resolved();
  CfgVtx* get_single_top_level();

  const std::vector<BlockVtx*>& create_blocks(int count);
  void link_fall_through(BlockVtx* first, BlockVtx* second);
  void link_branch(BlockVtx* first, BlockVtx* second);
  bool find_if_else_top_level();
  bool find_seq_top_level();
  bool find_while_loop_top_level();


  /*!
   * Apply a function f to each top-level vertex.
   * If f returns false, stops.
   */
  template <typename Func>
  void for_each_top_level_vtx(Func f) {
    for (auto* x : m_node_pool) {
      if (!x->parent && x != entry() && x != exit()) {
        if (!f(x)) {
          return;
        }
      }
    }
  }

  EntryVtx* entry() { return m_entry; }
  ExitVtx* exit() { return m_exit; }

  /*!
   * Allocate and construct a node of the specified type.
   */
  template <typename T, class... Args>
  T* alloc(Args&&... args) {
    T* new_obj = new T(std::forward<Args>(args)...);
    m_node_pool.push_back(new_obj);
    return new_obj;
  }

 private:
  //  bool compact_one_in_top_level();
  bool is_if_else(CfgVtx* b0, CfgVtx* b1, CfgVtx* b2, CfgVtx* b3);
  bool is_sequence(CfgVtx* b0, CfgVtx* b1);
  bool is_sequence_of_non_sequences(CfgVtx* b0, CfgVtx* b1);
  bool is_sequence_of_non_sequence_and_sequence(CfgVtx* b0, CfgVtx* b1);
  bool is_while_loop(CfgVtx* b0, CfgVtx* b1, CfgVtx* b2);
  std::vector<BlockVtx*> m_blocks;   // all block nodes, in order.
  std::vector<CfgVtx*> m_node_pool;  // all nodes allocated
  EntryVtx* m_entry;                 // the entry vertex
  ExitVtx* m_exit;                   // the exit vertex
};

class LinkedObjectFile;
class Function;
std::shared_ptr<ControlFlowGraph> build_cfg(const LinkedObjectFile& file,
                                            int seg,
                                            const Function& func);

#endif  // JAK_DISASSEMBLER_CFGVTX_H
