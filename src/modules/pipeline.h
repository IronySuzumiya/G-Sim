/*
 *
 * Andrew Smith
 *
 * Single Graphicionado Pipeline, can be connected together via the crossbar module.
 * Each pipeline get its own Scratchpad
 *
 * 10/07/19
 *
 */

#ifndef PIPELINE_H
#define PIPELINE_H

#include <iostream>
#include <vector>
#include <queue>
#include <list>
#include <algorithm>

// Process Modules
#include "module.h"
#include "memory.h"
#include "crossbar.h"
#include "readSrcProperty.h"
#include "readSrcEdges.h"
#include "allocator.h"
#include "arbiter.h"
#include "readDstProperty.h"
#include "controlAtomicUpdate.h"
#include "processEdge.h"
#include "readTempDstProperty.h"
#include "reduce.h"
#include "writeTempDstProperty.h"

// Apply Modules
#include "apply.h"
#include "readVertexProperty.h"
#include "readTempVertexProperty.h"
#include "writeVertexProperty.h"
#include "graph.h"

#include "scratchpad.h"

// Utility
#include "option.h"

namespace SimObj {

template<class v_t, class e_t>
class Pipeline {
private:
  std::list<uint64_t>* apply;
  std::list<uint64_t>* process;
  Crossbar<v_t, e_t>* crossbar;
  std::map<uint64_t, Utility::pipeline_data<v_t, e_t>>* scratchpad_map;
  SimObj::Memory* scratchpad;


  SimObj::ReadSrcProperty<v_t, e_t>* p1;
  SimObj::ReadSrcEdges<v_t, e_t>* p2;
  SimObj::Allocator<v_t, e_t>* alloc;
  std::vector<Module<v_t, e_t>*> parallel_vertex_readers;
  SimObj::Arbiter<v_t, e_t>* arbiter;
  SimObj::ProcessEdge<v_t, e_t>* p4;
  SimObj::ControlAtomicUpdate<v_t, e_t>* p5;
  SimObj::ReadTempDstProperty<v_t, e_t>* p6;
  SimObj::Reduce<v_t, e_t>* p7;
  SimObj::WriteTempDstProperty<v_t, e_t>* p8;

  SimObj::ReadVertexProperty<v_t, e_t>* a1;
  SimObj::ReadTempVertexProperty<v_t, e_t>* a2;
  SimObj::Apply<v_t, e_t>* a3;
  SimObj::WriteVertexProperty<v_t, e_t>* a4;

  Utility::Graph<v_t, e_t>* _graph;

  uint64_t _tick;
  int _id;

  uint64_t _num_edges_per_entry;
  uint64_t _scratchpad_entry_size;
  uint64_t _num_pipelines;

public:
  // Constructor:
  Pipeline(uint64_t pipeline_id, const Utility::Options opt, Utility::Graph<v_t, e_t>* graph, std::list<uint64_t>* process,
          GraphMat::GraphApp<v_t, e_t>* application, Memory* cache, Memory* dram, Crossbar<v_t, e_t>* crossbar, int num_dst_readers);

  // Destructor:
  ~Pipeline();


  // Methods:
  void tick_process();
  void tick_apply();

  bool process_complete();
  bool apply_complete();

  void process_ready();
  void apply_ready();

  void clear_stats();
  void print_stats_csv();
  void print_stats();
  void print_debug();

  // Stats Interface:
  uint64_t apply_size() {
    return apply->size();
  }

  void clear_apply() {
    apply->clear();
  }

  /*void set_apply(uint64_t start, uint64_t end) {
    for(uint64_t i = start; i < end; ++i) {
      apply->push_back(i);
    }
  }*/

  void make_apply_unique() {
    apply->sort();
    apply->erase(std::unique(apply->begin(), apply->end()), apply->end());
  }

  void clear_scratchpad() {
    scratchpad_map->clear();
  }

  void prefetch_edges(uint64_t v_start, uint64_t v_end, bool **prefetch_signal_list, uint64_t offset);

  void print_scratchpad_stats();
}; // class Pipeline

}; // namespace SimObj

#include "pipeline.tcc"

#endif
