#ifndef JAK_DISASSEMBLER_CFGVTX_H
#define JAK_DISASSEMBLER_CFGVTX_H

#include <string>
#include <vector>
#include <cassert>

class CfgVtx {
 public:
  virtual std::string to_string() = 0;
  virtual ~CfgVtx() = default;
  CfgVtx* parent_container = nullptr;
  std::vector<CfgVtx*> succ;
  std::vector<CfgVtx*> pred;

  bool has_succ(CfgVtx* s) {
    for(auto* x : succ) {
      if(x == s) return true;
    }
    return false;
  }

  bool has_pred(CfgVtx* p) {
    for(auto* x : pred) {
      if(x == p) return true;
    }
    return false;
  }
};

class BlockVtx : public CfgVtx {
 public:
  explicit BlockVtx(int id) : block_id(id) { }
  std::string to_string() override;
  int block_id = -1;

  bool has_branch = false;
  bool branch_likely = false;
  bool branch_always = false;
};

class SequenceVtx : public CfgVtx {
 public:
  std::string to_string() override;
  std::vector<CfgVtx*> seq;
};

class EntryVtx : public CfgVtx {
 public:
  EntryVtx() = default;
  std::string to_string() override;
};

class ExitVtx : public CfgVtx {
 public:
  std::string to_string() override;
};

class ControlFlowGraph {
 public:
  ControlFlowGraph() {
    m_entry = alloc<EntryVtx>();
    m_exit = alloc<ExitVtx>();
  }

  const std::vector<BlockVtx*>& create_blocks(int count) {
    for(int i = 0; i < count; i++) {
      m_blocks.push_back(alloc<BlockVtx>(i));
    }
    return m_blocks;
  }

  template<typename T, class... Args>
  T* alloc(Args&&... args) {
    T* new_obj = new T(std::forward<Args>(args)...);
    m_node_pool.push_back(new_obj);
    return new_obj;
  }

  ~ControlFlowGraph() {
    for(auto* x : m_node_pool) {
      delete x;
    }
  }

  EntryVtx* entry() {
    return m_entry;
  }

  ExitVtx* exit() {
    return m_exit;
  }

  void link(CfgVtx* first, CfgVtx* second) {
    assert(first->parent_container == second->parent_container);
//    assert(!first->has_succ(second));
//    assert(!second->has_pred(first));
    first->succ.push_back(second);
    second->pred.push_back(first);
  }

  std::string to_dot();

 private:
  std::vector<BlockVtx*> m_blocks;
  std::vector<CfgVtx*> m_node_pool;
  EntryVtx* m_entry;
  ExitVtx* m_exit;
};

#endif  // JAK_DISASSEMBLER_CFGVTX_H
