/*
 *
 * Andrew Smith
 *
 * PR graphMat implementation for Graphicionado
 *
 * "The Page Rank (PR) algorithm calculates scores of vertices
 * in a graph based on some metric (e.g., popularity). Web pages are 
 * represented as vertices and hyperlinks are represented as edges. 
 * The equation below shows how the PageRank score is calculated for each 
 * vertex. α is a constant and Udeg is the out-degree and a constant 
 * property of vertex U. In PageRank, all vertices are considered active
 * in all iterations." 
 * -https://mrmgroup.cs.princeton.edu/papers/taejun_micro16.pdf
 * 
 */

#ifndef GRAPHMAT_PR_H
#define GRAPHMAT_PR_H

#include "graphMat.h"

namespace GraphMat {

#define ALPHA 0.85f
#define TOLERANCE 1.0e-3f

class pr_v_t {
public:
  static const uint32_t size = 16;
  uint32_t edge_list_offset;
  uint32_t degree;
  double pageRank;
  pr_v_t() {
    edge_list_offset = 0;
    degree = 0;
    pageRank = 0.0;
  }
  friend std::ostream& operator<<(std::ostream& os, const pr_v_t& obj) {
    os << obj.pageRank;
    return os;
  }
  bool operator==(const pr_v_t& obj) const {
    return this->pageRank == obj.pageRank;
  }
  bool operator!=(const pr_v_t& obj) const {
    return !(this->pageRank == obj.pageRank);
  }
};

class pr_e_t {
public:
  static const uint32_t size = 4;
  uint32_t dst_id;
  float weight; // actually this variable is not needed, so is not counted into total size.
  pr_e_t() {
    dst_id = 0;
    weight = 0.0;
  }
  friend std::ostream& operator<<(std::ostream& os, const pr_e_t& obj) {
    os << obj.dst_id << ", " << obj.weight;
    return os;
  }
};

template<class v_t, class e_t>
class PR : public GraphApp<v_t, e_t> {
protected:

public:
  // Constructor
  PR() {}

  // Destructor
  ~PR() { /* Do Nothing */ }

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

}; // class PR

}; // namespace GraphMat

#include "pr.tcc"

#endif
