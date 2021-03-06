#include <cassert>

template<class v_t, class e_t>
SimObj::Pipeline<v_t, e_t>::Pipeline(uint64_t pipeline_id, const Utility::Options opt, Utility::readGraph<v_t>* graph, std::list<uint64_t>* process, GraphMat::GraphApp<v_t, e_t>* application, Memory* mem, Crossbar<v_t, e_t>* crossbar) {
  // Assert inputs are OK
  assert(graph != NULL);
  assert(application != NULL);
  assert(mem != NULL);
  assert(crossbar != NULL);
  assert(process != NULL);

  // Allocate Scratchpad
  scratchpad_map = new std::map<uint64_t, Utility::pipeline_data<v_t, e_t>>;
  scratchpad = new SimObj::Memory(opt.scratchpad_read_latency, opt.scratchpad_write_latency, opt.scratchpad_num_simultaneous_requests);
  _id = pipeline_id;

  // Allocate apply queue
  apply = new std::list<uint64_t>;

  // Allocate Pipeline Modules
  p1 = new SimObj::ReadSrcProperty<v_t, e_t>(mem, process, graph);
  p2 = new SimObj::ReadSrcEdges<v_t, e_t>(scratchpad, graph);
  p3 = new SimObj::ReadDstProperty<v_t, e_t>(mem, graph);
  p4 = new SimObj::ProcessEdge<v_t, e_t>(1, application);
  p5 = new SimObj::ControlAtomicUpdate<v_t, e_t>;
  p6 = new SimObj::ReadTempDstProperty<v_t, e_t>(scratchpad, graph, scratchpad_map);
  p7 = new SimObj::Reduce<v_t, e_t>(1, application);
  p8 = new SimObj::WriteTempDstProperty<v_t, e_t>(scratchpad, p5, scratchpad_map, apply);

  a1 = new SimObj::ReadVertexProperty<v_t, e_t>(mem, apply, graph);
  a2 = new SimObj::ReadTempVertexProperty<v_t, e_t>(scratchpad, graph, scratchpad_map);
  a3 = new SimObj::Apply<v_t, e_t>(1, application);
  a4 = new SimObj::WriteVertexProperty<v_t, e_t>(mem, process, graph);
  
  // Connect Pipeline
  p1->set_next(p2);
  p1->set_prev(NULL);
  p2->set_next(crossbar);
  p2->set_prev(p1);
  // Crossbar goes here:
  crossbar->connect_input(p2, pipeline_id);
  crossbar->connect_output(p3, pipeline_id);
  p3->set_next(p4);
  p3->set_prev(crossbar);
  p4->set_next(p5);
  p4->set_prev(p3);
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
  p3->set_name("ReadDstProperty " + std::to_string(pipeline_id));
  p4->set_name("ProcessEdge " + std::to_string(pipeline_id));
  p5->set_name("ControlAtomicUpdate " + std::to_string(pipeline_id));
  p6->set_name("ReadTempDstProperty " + std::to_string(pipeline_id));
  p7->set_name("Reduce " + std::to_string(pipeline_id));
  p8->set_name("WriteTempDstProperty " + std::to_string(pipeline_id));

  a1->set_name("ReadVertexProperty " + std::to_string(pipeline_id));
  a2->set_name("ReadTempVertexProperty " + std::to_string(pipeline_id));
  a3->set_name("Apply " + std::to_string(pipeline_id));
  a4->set_name("WriteVertexProperty " + std::to_string(pipeline_id));
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
  delete p3;
  p3 = NULL;
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
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::tick_process() {
  _tick++;
  p1->tick();
  p2->tick();
  p3->tick();
  p4->tick();
  p5->tick();
  p6->tick();
  p7->tick();
  p8->tick();
  scratchpad->tick();
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::tick_apply() {
  _tick++;
  a1->tick();
  a2->tick();
  a3->tick();
  a4->tick();
  scratchpad->tick();
}

template<class v_t, class e_t>
bool SimObj::Pipeline<v_t, e_t>::process_complete() {
  return (!p1->busy() && !p2->busy() && !p3->busy() && !p4->busy() &&
          !p5->busy() && !p6->busy() && !p7->busy() && !p8->busy());
}

template<class v_t, class e_t>
bool SimObj::Pipeline<v_t, e_t>::apply_complete() {
  return (!a1->busy() && !a2->busy() && !a3->busy() && !a4->busy());
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::process_ready() {
  p1->ready();
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::apply_ready() {
  a1->ready();
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::print_debug() {
  std::cout << "[ Pipeline " << _id << " ] " << " p1.busy() " << p1->busy() << ", p2.busy() " << p2->busy() << ", p3.busy() " << p3->busy();
  std::cout << ", p4.busy() " << p4->busy() << ", p5.busy() " << p5->busy() << ", p6.busy() " << p6->busy() << ", p7.busy() " << p7->busy() << ", p8.busy() " << p8->busy();
  std::cout << ", a1.busy() " << a1->busy() << ", a2.busy() " << a2->busy() << ", a3.busy() " << a3->busy() << ", a4.busy() " << a4->busy();
  std::cout << "\n" << std::flush;
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::print_stats() {
  std::cout << "------Pipeline " << _id << "--------------\n";
  p1->print_stats();
  p2->print_stats();
  p3->print_stats();
  p4->print_stats();
  p5->print_stats();
  p6->print_stats();
  p7->print_stats();
  p8->print_stats();
  a1->print_stats();
  a2->print_stats();
  a3->print_stats();
  a4->print_stats();
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::print_stats_csv() {
  std::cout << "------Pipeline " << _id << "--------------\n";
  p1->print_stats_csv();
  p2->print_stats_csv();
  p3->print_stats_csv();
  p4->print_stats_csv();
  p5->print_stats_csv();
  p6->print_stats_csv();
  p7->print_stats_csv();
  p8->print_stats_csv();
  a1->print_stats_csv();
  a2->print_stats_csv();
  a3->print_stats_csv();
  a4->print_stats_csv();
}

template<class v_t, class e_t>
void SimObj::Pipeline<v_t, e_t>::clear_stats() {
  p1->clear_stats();
  p2->clear_stats();
  p3->clear_stats();
  p4->clear_stats();
  p5->clear_stats();
  p6->clear_stats();
  p7->clear_stats();
  p8->clear_stats();
  a1->clear_stats();
  a2->clear_stats();
  a3->clear_stats();
  a4->clear_stats();
}
