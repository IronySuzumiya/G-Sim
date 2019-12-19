template<class v_t, class e_t>
void GraphMat::SSSP<v_t, e_t>::initialize(Utility::Graph<v_t, e_t>& graph, std::list<uint64_t>* curr) {
  graph.vertex[0].property.distance = 0;
  curr->push_back(0);
}

template<class v_t, class e_t>
void GraphMat::SSSP<v_t, e_t>::do_every_iteration(Utility::Graph<v_t, e_t>& graph, std::list<uint64_t>* curr) {

}

template<class v_t, class e_t>
bool GraphMat::SSSP<v_t, e_t>::reduce(v_t& scratch, const v_t& message) {
  // Reduce
  if(message < scratch) {
    scratch = message;
    return true;
  }
  return false;
}

template<class v_t, class e_t>
void GraphMat::SSSP<v_t, e_t>::process_edge(v_t& message, const e_t& edge, const v_t& vertex) {
  // Process Edge
  message.distance = vertex.distance + (uint64_t)edge;
}

template<class v_t, class e_t>
bool GraphMat::SSSP<v_t, e_t>::apply(const v_t& scratch, v_t& dram) {
  // Apply return true if successfully updated DRAM
  if(scratch < dram) {
    dram = scratch;
    return true;
  }
  return false;
}
