#include <cassert>
#include "CfgVtx.h"

std::string BlockVtx::to_string() {
  return "Block " + std::to_string(block_id);
}

std::string SequenceVtx::to_string() {
  assert(!seq.empty());
  std::string result = "Seq " + seq.front()->to_string() + " ... " + seq.back()->to_string();
  return result;
}

std::string EntryVtx::to_string() {
  return "ENTRY";
}

std::string ExitVtx::to_string() {
  return "EXIT";
}

std::string ControlFlowGraph::to_dot() {
  std::string result = "digraph G {\n";
  std::string invis;
  for(auto* node : m_node_pool) {
    if(!node->parent_container) {
      auto me = "\"" + node->to_string() + "\"";
      if(!invis.empty()) {
        invis += " -> ";
      }
      invis += me;
      result += me + ";\n";

      // it's a top level node
      for(auto* s : node->succ) {
        result += me + " -> \"" + s->to_string() + "\";\n";
      }
    }
  }
  result += "\n" + invis + " [style=invis];\n}\n";
  return result;
}