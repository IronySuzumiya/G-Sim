/*
 *
 * Andrew Smith
 *
 * CC graphMat implementation for Graphicionado
 *
 * Connected Components labels each vertex with an integer
 * corresponding to the set of verticies reachable from that
 * vertex.
 *
 */

#ifndef GRAPHMAT_CC_H
#define GRAPHMAT_CC_H

#include "graphMat.h"

namespace GraphMat {

class cc_t {
public:
  static const uint32_t size = 8;
  uint32_t edge_list_offset;
  uint32_t component;
  cc_t() {
    edge_list_offset = 0;
    component = ~0;
  }
  friend std::ostream& operator<<(std::ostream& os, const cc_t& obj) {
    os << obj.component;
    return os;
  }
  bool operator==(const cc_t& obj) const {
    return obj.component == this->component;
  }
  bool operator!=(const cc_t& obj) const {
    return obj.component != this->component;
  }
  bool operator<(const cc_t& obj) const {
    return this->component < obj.component;
  }
  bool operator>(const cc_t& obj) const {
    return this->component >= obj.component;
  }
};

template<class v_t, class e_t>
class CC : public GraphApp<v_t, e_t> {
protected:
  

public:
  // Constructor
  CC() {}

  // Destructor
  ~CC() { /* Do Nothing */ }

  // Init
  void initialize(Utility::Graph<v_t, e_t>& graph, std::list<uint64_t>* curr) override;

  // Function to do every iteration of the graph application
  void do_every_iteration(Utility::Graph<v_t, e_t>& graph, std::list<uint64_t>* curr) override;

  // Reduction Function
  bool reduce(v_t& a, const v_t& b) override;

  // Process Edge Function
  void process_edge(v_t& message, const e_t& edge, const v_t& vertex) override;

  // Apply
  bool apply(const v_t& scratch, v_t& dram) override;

}; // class CC

}; // namespace GraphMat

#include "cc.tcc"

#endif
