#include <cassert>

template<class v_t, class e_t>
SimObj::Pipeline<v_t, e_t>::Pipeline(uint64_t pipeline_id, const Utility::Options opt, Utility::Graph<v_t, e_t>* graph, std::list<uint64_t>* process,
                                    GraphMat::GraphApp<v_t, e_t>* application, Memory* cache, Memory* dram, Crossbar<v_t, e_t>* crossbar, int num_dst_readers) {
  // Assert inputs are OK
  assert(graph != NULL);
  assert(application != NULL);
  assert(cache != NULL);
  assert(dram != NULL);
  assert(crossbar != NULL);
  assert(process != NULL);
  assert(num_dst_readers > 0);

  _graph = graph;

  // Allocate Scratchpad
  scratchpad_map = new std::map<uint64_t, Utility::pipeline_data<v_t, e_t>>;
  scratchpad = new SimObj::Scratchpad(opt.scratchpad_num_lines, opt.scratchpad_line_data_width, opt.scratchpad_num_set_associative_way,
                                      dram, opt.scratchpad_read_latency, opt.scratchpad_write_latency, opt.scratchpad_num_simultaneous_requests);
  _id = pipeline_id;

  // Allocate apply queue
  apply = new std::list<uint64_t>;

  parallel_vertex_readers.resize(num_dst_readers);

  // Allocate Pipeline Modules
  p1 = new SimObj::ReadSrcProperty<v_t, e_t>(cache, process, graph, "ReadSrcProperty", _id);
  p2 = new SimObj::ReadSrcEdges<v_t, e_t>(scratchpad, graph, "ReadSrcEdges", _id);
  int _num = 0;
  for(auto mod = parallel_vertex_readers.begin(); mod != parallel_vertex_readers.end(); mod++) {
    *mod = new SimObj::ReadDstProperty<v_t, e_t>(cache, graph, "ReadDstProperty", _id, _num);
    _num++;
  }
  alloc = new SimObj::Allocator<v_t, e_t>(parallel_vertex_readers);
  arbiter = new SimObj::Arbiter<v_t, e_t>();
  p4 = new SimObj::ProcessEdge<v_t, e_t>(1, application, "ProcessEdge", _id);
  p5 = new SimObj::ControlAtomicUpdate<v_t, e_t>("ControlAtomicUpdate", _id);
  p6 = new SimObj::ReadTempDstProperty<v_t, e_t>(scratchpad, scratchpad_map, "ReadTempDstProperty", _id);
  p7 = new SimObj::Reduce<v_t, e_t>(1, application, "Reduce", _id);
  p8 = new SimObj::WriteTempDstProperty<v_t, e_t>(scratchpad, p5, scratchpad_map, apply, "WriteTempDstProperty", _id);

  a1 = new SimObj::ReadVertexProperty<v_t, e_t>(cache, apply, graph, "ReadVertexProperty", _id);
  a2 = new SimObj::ReadTempVertexProperty<v_t, e_t>(scratchpad, scratchpad_map, "ReadTempVertexProperty", _id);
  a3 = new SimObj::Apply<v_t, e_t>(1, application, "Apply", _id);
  a4 = new SimObj::WriteVertexProperty<v_t, e_t>(cache, process, graph, "WriteVertexProperty", _id);
  
  // Connect Pipeline
  p1->set_next(p2);
  p1->set_prev(NULL);
  p2->set_next(crossbar);
  p2->set_prev(p1);

/*
#ifndef APP_PR
  p2->set_next(crossbar);
  p2->set_prev(p1);
  // Crossbar goes here:
  crossbar->connect_input(p2, pipeline_id);
  crossbar->connect_output(alloc, pipeline_id);
  alloc->set_prev(crossbar);
  for(auto mod = parallel_vertex_readers.begin(); mod != parallel_vertex_readers.end(); mod++) {
    (*mod)->set_next(arbiter);
    (*mod)->set_prev(alloc);
  }
  arbiter->set_next(p4);
  arbiter->set_prev(parallel_vertex_readers[0]);
  p4->set_next(p5);
  p4->set_prev(arbiter);
#else
  p2->set_next(p4);
  p2->set_prev(p1);
  p4->set_next(p5);
  p4->set_prev(p2);
#endif
*/

#ifndef APP_PR
  // Crossbar goes here:
  crossbar->connect_input(p2, pipeline_id);
  crossbar->connect_output(alloc, pipeline_id);
  alloc->set_prev(crossbar);
  for(auto mod = parallel_vertex_readers.begin(); mod != parallel_vertex_readers.end(); mod++) {
    (*mod)->set_next(arbiter);
    (*mod)->set_prev(alloc);
  }
  arbiter->set_next(p4);
  arbiter->set_prev(parallel_vertex_readers[0]);
  p4->set_next(p5);
  p4->set_prev(arbiter);
#else
  crossbar->connect_input(p2, pipeline_id);
  crossbar->connect_output(p4, pipeline_id);
  p4->set_next(p5);
  p4->set_prev(p2);
#endif
  
  p5->set_next(p6);
  p5->set_prev(p4);
  p6->set_next(p7);
  p6->set_prev(p5);
  p7->set_next(p8);
  p7->set_prev(p6);
  p8->set_next(NULL);
  p8->set_prev(p7);

  a1->set_prev(NULL);
  a1->set_next(a2);
  a2->set_prev(a1);
  a2->set_next(a3);
  a3->set_prev(a2);
  a3->set_next(a4);
  a4->set_prev(a3);
  a4->set_next(NULL);

  // Name Modules
  p1->set_name("ReadSrcProperty " + std::to_string(pipeline_id));
  p2->set_name("ReadSrcEdges " + std::to_string(pipeline_id));
  int i = 0;
  for(auto mod = parallel_vertex_readers.begin(); mod != parallel_vertex_readers.end(); mod++) {
    (*mod)->set_name("ReadDstProperty " + std::to_string(pipeline_id) + " " + std::to_string(i));
    i++;
  }
  alloc->set_name("Allocator " + std::to_string(pipeline_id));
  arbiter->set_name("Arbiter " + std::to_string(pipeline_id));
  p4->set_name("ProcessEdge " + std::to_string(pipeline_id));
  p5->set_name("ControlAtomicUpdate " + std::to_string(pipeline_id));
  p6->set_name("ReadTempDstProperty " + std::to_string(pipeline_id));
  p7->set_name("Reduce " + std::to_string(pipeline_id));
  p8->set_name("WriteTempDstProperty " + std::to_string(pipeline_id));

  a1->set_name("ReadVertexProperty " + std::to_string(pipeline_id));
  a2->set_name("ReadTempVertexProperty " + std::to_string(pipeline_id));
  a3->set_name("Apply " + std::to_string(pipeline_id));
  a4->set_name("WriteVertexProperty " + std::to_string(pipeline_id));

  scratchpad->set_name("Scratchpad " + std::to_string(pipeline_id));

  _scratchpad_entry_size = opt.scratchpad_line_data_width;
  _num_edges_per_entry = _scratchpad_entry_size / e_t().size;
  _num_pipelines = opt.num_pipelines;
}

template<class v_t, class e_t>
SimObj::Pipeline<v_t, e_t>::~Pipeline() {
  delete scratchpad_map;
  scratchpad_map = NULL;
  delete scratchpad;
  scratchpad = NULL;
  delete apply;
  apply = NULL;

  delete p1;
  p1 = NULL;
  delete p2;
  p2 = NULL;
  delete alloc;
  alloc = NULL;
  for(auto mod = parallel_vertex_readers.begin(); mod != parallel_vertex_readers.end(); mod++) {
    delete *mod;
    *mod = NULL;
  }
  delete arbiter;
  arbiter = NULL;
  delete p4;
  p4 = NULL;
  delete p5;
  p5 = NULL;
  delete p6;
  p6 = NULL;
  delete p7;
  p7 = NULL;
  delete p8;
  p8 = NULL;

  delete a1;
  a1 = NULL;
  delete a2;
  a2 = NULL;
  delete a3;
  a3 = NULL;
  delete a4;
  a4 = NULL;

  process = NULL;
  crossbar = NULL;

  _graph = NULL;
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::tick_process() {
  _tick++;
  scratchpad->tick();
  p8->tick();
  p7->tick();
  p6->tick();
  p5->tick();
  p4->tick();

#ifndef APP_PR
  arbiter->tick();
  for(auto mod = parallel_vertex_readers.begin(); mod != parallel_vertex_readers.end(); mod++) {
    (*mod)->tick();
  }
  alloc->tick();
#endif

  p2->tick();
  p1->tick();
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::tick_apply() {
  _tick++;
  scratchpad->tick();
  a4->tick();
  a3->tick();
  a2->tick();
  a1->tick();
}

template<class v_t, class e_t>
bool SimObj::Pipeline<v_t, e_t>::process_complete() {
  bool pvr = false;
  for(auto mod = parallel_vertex_readers.begin(); mod != parallel_vertex_readers.end(); mod++) {
    if((*mod)->busy()) {
      pvr = true;
    }
  }
  return (!p1->busy() && !p2->busy() && !pvr && !p4->busy() &&
          !p5->busy() && !p6->busy() && !p7->busy() && !p8->busy());
}

template<class v_t, class e_t>
bool SimObj::Pipeline<v_t, e_t>::apply_complete() {
  return (!a1->busy() && !a2->busy() && !a3->busy() && !a4->busy());
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::process_ready() {
  p1->ready();
  // Reset the sequential modules:
  p1->reset();
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::apply_ready() {
  a1->ready();
  // Reset the sequential modules:
  a1->reset();
  a4->reset();
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::print_debug() {
#if !defined(APP_PR)
  bool pvr = false;
  for(auto mod = parallel_vertex_readers.begin(); mod != parallel_vertex_readers.end(); mod++) {
    if((*mod)->busy()) {
      pvr = true;
    }
  }
#endif
  std::cout << "[ Pipeline " << _id << " ] " << " p1.busy() " << p1->busy() << ", p2.busy() " << p2->busy();
#if !defined(APP_PR)
  std::cout << ", p3.busy() " << pvr;
#endif
  std::cout << ", p4.busy() " << p4->busy() << ", p5.busy() " << p5->busy() << ", p6.busy() " << p6->busy() << ", p7.busy() " << p7->busy() << ", p8.busy() " << p8->busy();
  std::cout << ", a1.busy() " << a1->busy() << ", a2.busy() " << a2->busy() << ", a3.busy() " << a3->busy() << ", a4.busy() " << a4->busy();
  std::cout << "\n" << std::flush;
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::print_stats() {
  sim_out.write("------Pipeline "+std::to_string(_id)+"--------------\n");
  p1->print_stats();
  p2->print_stats();
#ifndef APP_PR
  alloc->print_stats();
  for(auto mod = parallel_vertex_readers.begin(); mod != parallel_vertex_readers.end(); mod++) {
    (*mod)->print_stats();
  }
  arbiter->print_stats();
#endif
  p4->print_stats();
  p5->print_stats();
  p6->print_stats();
  p7->print_stats();
  p8->print_stats();
  a1->print_stats();
  a2->print_stats();
  a3->print_stats();
  a4->print_stats();
  scratchpad->print_stats();
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::print_stats_csv() {
  sim_out.write("------Pipeline "+std::to_string(_id)+"--------------\n");
  p1->print_stats_csv();
  p2->print_stats_csv();
#ifndef APP_PR
  alloc->print_stats_csv();
  for(auto mod = parallel_vertex_readers.begin(); mod != parallel_vertex_readers.end(); mod++) {
    (*mod)->print_stats_csv();
  }
  arbiter->print_stats_csv();
#endif
  p4->print_stats_csv();
  p5->print_stats_csv();
  p6->print_stats_csv();
  p7->print_stats_csv();
  p8->print_stats_csv();
  a1->print_stats_csv();
  a2->print_stats_csv();
  a3->print_stats_csv();
  a4->print_stats_csv();
  scratchpad->print_stats_csv();
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::clear_stats() {
  p1->clear_stats();
  p2->clear_stats();
  alloc->clear_stats();
  for(auto mod = parallel_vertex_readers.begin(); mod != parallel_vertex_readers.end(); mod++) {
    (*mod)->clear_stats();
  }
  arbiter->clear_stats();
  p4->clear_stats();
  p5->clear_stats();
  p6->clear_stats();
  p7->clear_stats();
  p8->clear_stats();
  a1->clear_stats();
  a2->clear_stats();
  a3->clear_stats();
  a4->clear_stats();

  scratchpad->reset();
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::prefetch_edges(uint64_t v_start, uint64_t v_end, bool **prefetch_signal_list, uint64_t offset) {
  //uint64_t i = v_start + ((_num_pipelines + (_id - (v_start % _num_pipelines))) % _num_pipelines);
  int tmp = v_start;
  while((tmp - _id) % _num_pipelines) {
    ++tmp;
  }
  for(uint64_t i = (uint64_t)tmp; i < v_end; i += _num_pipelines) {
    uint64_t start_edge = _graph->vertex[i].edge_list_offset;
    uint64_t last_edge = start_edge + _graph->vertex[i].edges.size() - 1;
    uint64_t prefetch_start_entry_id = start_edge / _num_edges_per_entry;
    uint64_t prefetch_last_entry_id = last_edge / _num_edges_per_entry;
    for(uint64_t j = prefetch_start_entry_id, k = 0; j <= prefetch_last_entry_id; ++j, ++k) {
      scratchpad->prefetch(EDGE_LIST_ADDR_OFFSET + j * _scratchpad_entry_size, &prefetch_signal_list[i - offset][k]);
    }
  }
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::print_scratchpad_stats() {
  scratchpad->print_stats();
}
