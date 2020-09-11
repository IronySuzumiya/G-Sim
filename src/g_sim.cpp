#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include <list>

// Process Modules
#include "memory.h"
#include "dram.h"
#include "cache.h"
#include "crossbar.h"

// Pipeline Class
#include "pipeline.h"
#include "log.h"

// Utility
#include "option.h"
#include "graph.h"

// GraphMat
#include "bfs.h"
#include "cc.h"
#include "sssp.h"
#include "pr.h"

#define ITERATIONS 10000

#ifdef APP_BFS
typedef GraphMat::bfs_t vertex_t;
typedef double edge_t;
#endif

#ifdef APP_CC
typedef GraphMat::cc_t vertex_t;
typedef double edge_t;
#endif

#ifdef APP_SSSP
typedef GraphMat::sssp_t vertex_t;
typedef double edge_t;
#endif

#ifdef APP_PR
typedef GraphMat::pr_t vertex_t;
typedef double edge_t;
#endif

void print_queue(std::string name, std::list<uint64_t>* q, int iteration) {
  std::cout << "Iteration: " << iteration << " " << name << " Queue Size " << q->size();
  std::cout << "   " << name << " Queue: [ ";
  int i = 0;
  for(auto it = q->begin(); it != q->end() && i < 20; it++) {
    std::cout << *it << ", ";
    i++;
  }
  if(i < 20) {
    std::cout << "]\n" << std::flush;
  }
  else {
    std::cout << "...\n" << std::flush;
  }
}

int main(int argc, char** argv) {
  Utility::Options opt;
  opt.parse(argc, argv);
  std::list<uint64_t>* process = new std::list<uint64_t>;

  Utility::Graph<vertex_t, edge_t> graph;
  graph.import(opt.graph_path);

#ifdef APP_BFS
  GraphMat::BFS<vertex_t, edge_t> app;
#endif
#ifdef APP_CC
  GraphMat::CC<vertex_t, edge_t> app;
#endif
#ifdef APP_SSSP
  GraphMat::SSSP<vertex_t, edge_t> app;
#endif
#ifdef APP_PR
  GraphMat::PR<vertex_t, edge_t> app;
#endif
  app.initialize(graph, process);

  std::vector<SimObj::Pipeline<vertex_t, edge_t>*>* tile = new std::vector<SimObj::Pipeline<vertex_t, edge_t>*>;

  SimObj::Crossbar<vertex_t, edge_t>* crossbar = new SimObj::Crossbar<vertex_t, edge_t>(opt.num_pipelines);
  crossbar->set_name("Crossbar");

  SimObj::Memory* dram = new SimObj::DRAM;
  SimObj::Memory* cache = new SimObj::Cache(opt.cache_num_lines, opt.cache_line_data_width, opt.cache_num_set_associative_way, dram);
  cache->set_name("Cache");

  for(uint64_t i = 0; i < opt.num_pipelines; i++) {
    SimObj::Pipeline<vertex_t, edge_t>* temp = new SimObj::Pipeline<vertex_t, edge_t>(i, opt, &graph, process, &app, cache, dram, crossbar, opt.num_dst_readers);
    tile->push_back(temp);
  }

  uint64_t global_tick = 0;
  bool complete = false;
  uint64_t edges_processed = 0;
  uint64_t edges_process_phase = 0;
  uint64_t apply_size = 0;
  uint64_t process_cycles = 0;
  uint64_t apply_cycles = 0;

  // Iteration Loop:
  for(uint64_t iteration = 0; iteration < opt.num_iter && !process->empty(); iteration++) {
    app.do_every_iteration(graph, process);

    #ifdef LARGE_GRAPH
    // TODO: Graph Partitioning
    #endif

    // Reset all the stats Counters:
    SimObj::sim_out.write("---------------------------------------------------------------\n");
    SimObj::sim_out.write("ITERATION " + std::to_string(iteration) + "\n");
    SimObj::sim_out.write("---------------------------------------------------------------\n");
    std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->clear_stats();});
    crossbar->clear_stats();

#ifdef DEBUG
    print_queue("Process", process, iteration);
#endif
    cache->reset();
    // Processing Phase 
    std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->process_ready();});
    complete = false;
    while(!complete || (process->size() != 0)) {
      global_tick++;
      process_cycles++;
//#ifndef APP_PR
      crossbar->tick();
//#endif
      dram->tick();
      cache->tick();
      std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->tick_process();});
      //std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->print_debug();});
      complete = true;
      std::for_each(tile->begin(), tile->end(), [&complete, crossbar](SimObj::Pipeline<vertex_t, edge_t>* a) mutable {
        if(!a->process_complete() || crossbar->busy()) complete = false;
      });
    }

/*
#ifdef APP_PR
    uint64_t apply_size_each_tile = graph.vertex.size() / opt.num_pipelines;
    uint64_t tile_start = 0;
    for(uint64_t i = 0; i < opt.num_pipelines - 1; i++) {
      (*tile)[i]->set_apply(tile_start, tile_start + apply_size_each_tile);
      tile_start += apply_size_each_tile;
    }
    (*tile)[opt.num_pipelines - 1]->set_apply(tile_start, graph.vertex.size());
#endif
*/

    // Accumulate the edges processed each iteration
    edges_process_phase = 0;
    std::for_each(tile->begin(), tile->end(), [&edges_process_phase](SimObj::Pipeline<vertex_t, edge_t>* a) mutable {
      edges_process_phase += a->apply_size();
    });

    std::cout << "Iteration: " << iteration << " Apply Size: " << edges_process_phase << "\n";
    edges_processed += edges_process_phase;

#ifdef APP_PR
    std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->make_apply_unique();});
    edges_process_phase = 0;
    std::for_each(tile->begin(), tile->end(), [&edges_process_phase](SimObj::Pipeline<vertex_t, edge_t>* a) mutable {
      edges_process_phase += a->apply_size();
    });
    std::cout << "PageRank Actual Apply Size: " << edges_process_phase << "\n";
#endif
    
    // Apply Phase
    std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->apply_ready();});
    complete = false;
    while(!complete || (apply_size != 0)) {
      global_tick++;
      apply_cycles++;
      dram->tick();
      cache->tick();
      std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->tick_apply();});
      //std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->print_debug();});
      complete = true;
      std::for_each(tile->begin(), tile->end(), [&complete](SimObj::Pipeline<vertex_t, edge_t>* a) mutable {
        if(!a->apply_complete()) complete = false;
      });
      apply_size = 0;
      std::for_each(tile->begin(), tile->end(), [&apply_size](SimObj::Pipeline<vertex_t, edge_t>* a) mutable {
        apply_size += a->apply_size();
      });
    }

#ifdef APP_PR
    for(uint64_t i = 0; i < graph.vertex.size(); ++i) {
      process->push_back(i);
    }
    /*if(iteration % 2) {
      for(uint64_t i = 0; i < graph.vertex.size(); ++i) {
        process->push_back(i);
      }
    } else {
      for(uint64_t i = graph.vertex.size() - 1; i >= 0; --i) {
        process->push_back(i);
      }
    }*/
#endif

    // Print all the stats counters:
    std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->print_stats();});
//#ifndef APP_PR
    crossbar->print_stats();
//#endif
    cache->print_stats();

#ifdef APP_PR
    std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->clear_scratchpad();});
#endif
  }
#ifdef DEBUG
  graph.printVertexProperties(30);
  SimObj::sim_out.write("---------------------------------------------------------------\n");
  SimObj::sim_out.write("DONE!\n");
  SimObj::sim_out.write("---------------------------------------------------------------\n");
  SimObj::sim_out.write("  Global Ticks:             " + std::to_string(global_tick) + " cycles\n");
  SimObj::sim_out.write("  Edges Processed:          " + std::to_string(edges_processed) + "\n");
  SimObj::sim_out.write("  Throughput (Edges/Ticks): " + std::to_string((float)edges_processed/(float)global_tick) + "\n");
  SimObj::sim_out.write("  Process Phase Durations:  " + std::to_string(process_cycles) + " cycles\n");
  SimObj::sim_out.write("  Apply Phase Durations:    " + std::to_string(apply_cycles) + " cycles\n");
#endif

  dram->print_stats();

  for(uint64_t i = 0; i < opt.num_pipelines; i++) {
    delete tile->operator[](i);
  }

  graph.writeVertexProperties(opt.result);

  return 0;
}
