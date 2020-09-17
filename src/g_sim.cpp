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

#ifdef APP_BFS
typedef GraphMat::bfs_t vertex_t;
typedef GraphMat::e_t edge_t;
#endif

#ifdef APP_CC
typedef GraphMat::cc_t vertex_t;
typedef GraphMat::e_t edge_t;
#endif

#ifdef APP_SSSP
typedef GraphMat::sssp_t vertex_t;
typedef GraphMat::e_t edge_t;
#endif

#ifdef APP_PR
typedef GraphMat::pr_v_t vertex_t;
typedef GraphMat::pr_e_t edge_t;
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
#if defined(APP_PR)
  std::vector<std::list<uint64_t>*>* process = new std::vector<std::list<uint64_t>*>;
  for(uint64_t i = 0; i < opt.num_pipelines; i++) {
    process->push_back(new std::list<uint64_t>);
  }
#else
  std::list<uint64_t>* process = new std::list<uint64_t>;
#endif

  Utility::Graph<vertex_t, edge_t> graph;
  graph.import(opt.graph_path);

#if defined(APP_BFS)
  GraphMat::BFS<vertex_t, edge_t> app;
#elif defined(APP_CC)
  GraphMat::CC<vertex_t, edge_t> app;
#elif defined(APP_SSSP)
  GraphMat::SSSP<vertex_t, edge_t> app;
#elif defined(APP_PR)
  GraphMat::PR<vertex_t, edge_t> app;
#endif

  std::vector<SimObj::Pipeline<vertex_t, edge_t>*>* tile = new std::vector<SimObj::Pipeline<vertex_t, edge_t>*>;

  SimObj::Crossbar<vertex_t, edge_t>* crossbar = new SimObj::Crossbar<vertex_t, edge_t>(opt.num_pipelines);
  crossbar->set_name("Crossbar");

  SimObj::Memory* dram = new SimObj::DRAM;
  SimObj::Memory* cache = new SimObj::Cache(opt.cache_num_lines, opt.cache_line_data_width, opt.cache_num_set_associative_way, dram);
  cache->set_name("Cache");

  for(uint64_t i = 0; i < opt.num_pipelines; i++) {
#if defined(APP_PR)
    SimObj::Pipeline<vertex_t, edge_t>* temp = new SimObj::Pipeline<vertex_t, edge_t>(i, opt, &graph, (*process)[i], &app, cache, dram, crossbar, opt.num_dst_readers);
#else
    SimObj::Pipeline<vertex_t, edge_t>* temp = new SimObj::Pipeline<vertex_t, edge_t>(i, opt, &graph, process, &app, cache, dram, crossbar, opt.num_dst_readers);
#endif
    tile->push_back(temp);
  }

  uint64_t global_tick = 0;
  bool complete = false;
  uint64_t edges_processed = 0;
  uint64_t edges_process_phase = 0;
  uint64_t apply_size = 0;
  uint64_t process_cycles = 0;
  uint64_t apply_cycles = 0;
  uint64_t vertex_size = vertex_t().size;
  uint64_t edge_size = edge_t().size;
  uint64_t v_part_size = opt.cache_line_data_width * opt.cache_num_lines / vertex_size;
  uint64_t e_part_size = opt.num_pipelines * opt.scratchpad_line_data_width * opt.scratchpad_num_lines / edge_size * 3/4;
  uint64_t num_vertices_per_entry = opt.cache_line_data_width / vertex_size;
  bool prefetch_finish = false;
  uint64_t prefetch_list_size = 0;
  bool *prefetch_signal_list = NULL;
  bool **edges_prefetch_signal_list = NULL;
  uint64_t num_prefetch_batch = 0;
  uint64_t prefetch_batch_id = 0;
  uint64_t num_vertex = graph.vertex.size();
  uint64_t num_edges_per_entry = opt.scratchpad_line_data_width / edge_size;
  uint64_t edges_prefetch_start_v_id = 0;
  uint64_t edges_prefetch_end_v_id = 0;

#if defined(APP_PR)
  app.initialize(graph, (*process)[0]);
  
#else
  app.initialize(graph, process);
#endif
#if defined(APP_PR) && !defined(LARGE_GRAPH)
  for(uint64_t i = 0; i < num_vertex; ++i) {
    process->at(i % opt.num_pipelines)->push_back(i);
  }
  uint64_t num_prefetch_entry = (num_vertex - 1) / num_vertices_per_entry + 1;
  prefetch_list_size = num_prefetch_entry;
  num_prefetch_batch = (prefetch_list_size - 1) / opt.prefetch_batch_size + 1;
  prefetch_signal_list = new bool[prefetch_list_size];
  std::memset(prefetch_signal_list, 0, sizeof(bool) * prefetch_list_size);
  bool prefetch_sent = false;

  edges_prefetch_signal_list = new bool*[num_vertex];
  for(uint64_t i = 0; i < num_vertex; ++i) {
    uint64_t start_edge = _graph->vertex[i].edge_list_offset;
    uint64_t last_edge = start_edge + _graph->vertex[i].edges.size() - 1;
    uint64_t edges_prefetch_start_entry_id = start_edge / num_edges_per_entry;
    uint64_t edges_prefetch_last_entry_id = last_edge / num_edges_per_entry;
    edges_prefetch_signal_list[i] = new bool[edges_prefetch_last_entry_id - edges_prefetch_start_entry_id + 1];
    for(uint64_t j = 0; j < edges_prefetch_last_entry_id - edges_prefetch_start_entry_id + 1; ++j) {
      edges_prefetch_signal_list[i][j] = false;
    }
    //std::memset(&edges_prefetch_signal_list[i], 0, sizeof(bool) * (edges_prefetch_last_entry_id - edges_prefetch_start_entry_id + 1));
  }
#endif

#if defined(APP_PR)
  // Iteration Loop:
  for(uint64_t iteration = 0; iteration < opt.num_iter; iteration++) {
#else
  for(uint64_t iteration = 0; iteration < opt.num_iter && !process->empty(); iteration++) {
#endif
    //app.do_every_iteration(graph, process);

    // Reset all the stats Counters:
    SimObj::sim_out.write("---------------------------------------------------------------\n");
    SimObj::sim_out.write("ITERATION " + std::to_string(iteration) + "\n");
    SimObj::sim_out.write("---------------------------------------------------------------\n");
    std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->clear_stats();});
    crossbar->clear_stats();
    cache->reset();

#if defined(APP_PR) && !defined(LARGE_GRAPH)
    if(!prefetch_sent) {
      for(uint64_t i = 0; i < std::min(opt.prefetch_batch_size, prefetch_list_size); ++i) {
        cache->prefetch(i * opt.cache_line_data_width, &prefetch_signal_list[i]);
      }
      edges_prefetch_start_v_id = 0;
      edges_prefetch_end_v_id = std::min(opt.prefetch_batch_size * num_vertices_per_entry, num_vertex);
      std::for_each(tile->begin(), tile->end(), [edges_prefetch_start_v_id, edges_prefetch_end_v_id, edges_prefetch_signal_list](SimObj::Pipeline<vertex_t, edge_t>* a) {
        a->prefetch_edges(edges_prefetch_start_v_id, edges_prefetch_end_v_id, edges_prefetch_signal_list, 0);
      });
      prefetch_send = true;
    }
#endif

#if defined(APP_PR) && defined(LARGE_GRAPH)
    for(uint64_t v_part_start = 0, e_part_start = 0; v_part_start < num_vertex; ) {
      uint64_t v_part_end = std::min(v_part_start + v_part_size, num_vertex);
      uint64_t e_part_end = graph.vertex[v_part_end - 1].edge_list_offset + graph.vertex[v_part_end - 1].edges.size();
      if(e_part_end - e_part_start > e_part_size) {
        for(uint64_t i = v_part_end - 1; i >= v_part_start; --i) {
          if(graph.vertex[i].edge_list_offset - e_part_start <= e_part_size) {
            e_part_end = graph.vertex[i].edge_list_offset;
            v_part_end = i;
            break;
          }
        }
        if(e_part_end - e_part_start > e_part_size) {
          v_part_end = v_part_start + 1;
          e_part_end = graph.vertex[v_part_end].edge_list_offset;
        }
      }
      for(uint64_t i = v_part_start; i < v_part_end; ++i) {
        process->at(i % opt.num_pipelines)->push_back(i);
      }

      uint64_t prefetch_start_entry_id = v_part_start / num_vertices_per_entry;
      uint64_t prefetch_last_entry_id = (v_part_end - 1) / num_vertices_per_entry;
      prefetch_list_size = prefetch_last_entry_id + 1 - prefetch_start_entry_id;
      num_prefetch_batch = (prefetch_list_size - 1) / opt.prefetch_batch_size + 1;
      prefetch_signal_list = new bool[prefetch_list_size];
      std::memset(prefetch_signal_list, 0, sizeof(bool) * prefetch_list_size);
      prefetch_finish = false;
      prefetch_batch_id = 0;

      for(uint64_t i = 0; i < std::min(opt.prefetch_batch_size, prefetch_list_size); ++i) {
        cache->prefetch((prefetch_start_entry_id + i) * opt.cache_line_data_width, &prefetch_signal_list[i]);
      }

      edges_prefetch_signal_list = new bool*[v_part_end - v_part_start];
      for(uint64_t i = v_part_start; i < v_part_end; ++i) {
        uint64_t start_edge = graph.vertex[i].edge_list_offset;
        uint64_t last_edge = start_edge + graph.vertex[i].edges.size() - 1;
        uint64_t edges_prefetch_start_entry_id = start_edge / num_edges_per_entry;
        uint64_t edges_prefetch_last_entry_id = last_edge / num_edges_per_entry;
        edges_prefetch_signal_list[i - v_part_start] = new bool[edges_prefetch_last_entry_id - edges_prefetch_start_entry_id + 1];
        for(uint64_t j = 0; j < edges_prefetch_last_entry_id - edges_prefetch_start_entry_id + 1; ++j) {
          edges_prefetch_signal_list[i - v_part_start][j] = false;
        }
        //std::memset(&edges_prefetch_signal_list[i], 0, sizeof(bool) * (edges_prefetch_last_entry_id - edges_prefetch_start_entry_id + 1));
      }
      
      edges_prefetch_start_v_id = v_part_start;
      edges_prefetch_end_v_id = v_part_start + std::min(opt.prefetch_batch_size * num_vertices_per_entry, v_part_end - v_part_start);
      std::for_each(tile->begin(), tile->end(), [edges_prefetch_start_v_id, edges_prefetch_end_v_id, edges_prefetch_signal_list, v_part_start]
      (SimObj::Pipeline<vertex_t, edge_t>* a) {
        a->prefetch_edges(edges_prefetch_start_v_id, edges_prefetch_end_v_id, edges_prefetch_signal_list, v_part_start);
      });

      std::cout << "Graph Partition: [" << std::to_string(v_part_start) << " - " << std::to_string(v_part_end) << "]" << std::endl;
      SimObj::sim_out.write("Graph Partition: [" + std::to_string(v_part_start) + " - " + std::to_string(v_part_end) + "]\n");
#endif // LARGE_GRAPH

      // Processing Phase 
      std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->process_ready();});
      complete = false;

#if defined(APP_PR)
      while(!complete) {
#else
      while(!complete || !process->empty()) {
#endif
        global_tick++;
        process_cycles++;
        crossbar->tick();
        dram->tick();
        cache->tick();
#if defined(APP_PR)
        if(!prefetch_finish) {
          bool all_prefetch_done = true;
          for(uint64_t i = prefetch_batch_id * opt.prefetch_batch_size; i < std::min((prefetch_batch_id + 1) * opt.prefetch_batch_size, prefetch_list_size); ++i) {
            if(!prefetch_signal_list[i]) {
              all_prefetch_done = false;
              break;
            }
          }
          if(all_prefetch_done) {
            ++prefetch_batch_id;
            if(prefetch_batch_id < num_prefetch_batch) {
              for(uint64_t i = prefetch_batch_id * opt.prefetch_batch_size; i < std::min((prefetch_batch_id + 1) * opt.prefetch_batch_size, prefetch_list_size); ++i) {
  #if !defined(LARGE_GRAPH)
                cache->prefetch(i * opt.cache_line_data_width, &prefetch_signal_list[i]);
  #else
                cache->prefetch((prefetch_start_entry_id + i) * opt.cache_line_data_width, &prefetch_signal_list[i]);
  #endif
              }
              edges_prefetch_start_v_id = edges_prefetch_end_v_id;
  #if !defined(LARGE_GRAPH)
              edges_prefetch_end_v_id = std::min((prefetch_batch_id + 1) * opt.prefetch_batch_size * num_vertices_per_entry, num_vertex);
              std::for_each(tile->begin(), tile->end(), [edges_prefetch_start_v_id, edges_prefetch_end_v_id, edges_prefetch_signal_list]
              (SimObj::Pipeline<vertex_t, edge_t>* a) {
                a->prefetch_edges(edges_prefetch_start_v_id, edges_prefetch_end_v_id, edges_prefetch_signal_list, 0);
              });
  #else
              edges_prefetch_end_v_id = v_part_start + std::min((prefetch_batch_id + 1) * opt.prefetch_batch_size * num_vertices_per_entry, v_part_end - v_part_start);
              std::for_each(tile->begin(), tile->end(), [edges_prefetch_start_v_id, edges_prefetch_end_v_id, edges_prefetch_signal_list, v_part_start]
              (SimObj::Pipeline<vertex_t, edge_t>* a) {
                a->prefetch_edges(edges_prefetch_start_v_id, edges_prefetch_end_v_id, edges_prefetch_signal_list, v_part_start);
              });
  #endif
            } else {
              cache->print_stats();
              std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->print_scratchpad_stats();});
              prefetch_finish = true;
              delete []prefetch_signal_list;
              prefetch_signal_list = NULL;
              std::cout << "Prefetch Finished! @ Cycle " << std::to_string(global_tick) << std::endl;
              SimObj::sim_out.write("Prefetch Finished! @ Cycle " + std::to_string(global_tick) + "\n");
            }
          }
        }
#endif
        std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->tick_process();});
        /*if(global_tick % 10000 == 0) {
          std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->print_debug();});
        }*/
        complete = true;
        std::for_each(tile->begin(), tile->end(), [&complete, crossbar](SimObj::Pipeline<vertex_t, edge_t>* a) mutable {
          if(!a->process_complete() || crossbar->busy()) complete = false;
        });
      }

#if defined(APP_PR) && defined(LARGE_GRAPH)
      for(uint64_t i = 0; i < v_part_end - v_part_start; ++i) {
        delete edges_prefetch_signal_list[i];
      }
      delete []edges_prefetch_signal_list;
      edges_prefetch_signal_list = NULL;

      v_part_start = v_part_end;
      e_part_start = e_part_end;
    }
#endif

    // Accumulate the edges processed each iteration
    edges_process_phase = 0;
    std::for_each(tile->begin(), tile->end(), [&edges_process_phase](SimObj::Pipeline<vertex_t, edge_t>* a) mutable {
      edges_process_phase += a->apply_size();
    });

    std::cout << "Iteration: " << iteration << " Apply Size: " << edges_process_phase << std::endl;
    edges_processed += edges_process_phase;

#ifdef APP_PR
    std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->make_apply_unique();});
    edges_process_phase = 0;
    std::for_each(tile->begin(), tile->end(), [&edges_process_phase](SimObj::Pipeline<vertex_t, edge_t>* a) mutable {
      edges_process_phase += a->apply_size();
    });
    std::cout << "PageRank Actual Apply Size: " << edges_process_phase << std::endl;
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
      /*if(global_tick % 10000 == 0) {
        std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->print_debug();});
      }*/
      complete = true;
      std::for_each(tile->begin(), tile->end(), [&complete](SimObj::Pipeline<vertex_t, edge_t>* a) mutable {
        if(!a->apply_complete()) complete = false;
      });
      apply_size = 0;
      std::for_each(tile->begin(), tile->end(), [&apply_size](SimObj::Pipeline<vertex_t, edge_t>* a) mutable {
        apply_size += a->apply_size();
      });
    }

#if defined(APP_PR) && !defined(LARGE_GRAPH)
    for(uint64_t i = 0; i < num_vertex; ++i) {
      process->at(i % opt.num_pipelines)->push_back(i);
    }
#endif

    // Print all the stats counters:
    std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->print_stats();});
    crossbar->print_stats();
    cache->print_stats();

#ifdef APP_PR
    std::for_each(tile->begin(), tile->end(), [](SimObj::Pipeline<vertex_t, edge_t>* a) {a->clear_scratchpad();});
#endif
  }

  SimObj::sim_out.write("---------------------------------------------------------------\n");
  SimObj::sim_out.write("DONE!\n");
  SimObj::sim_out.write("---------------------------------------------------------------\n");
  SimObj::sim_out.write("  Global Ticks:             " + std::to_string(global_tick) + " cycles\n");
  SimObj::sim_out.write("  Edges Processed:          " + std::to_string(edges_processed) + "\n");
  SimObj::sim_out.write("  Throughput (Edges/Ticks): " + std::to_string((float)edges_processed/(float)global_tick) + "\n");
  SimObj::sim_out.write("  Process Phase Durations:  " + std::to_string(process_cycles) + " cycles\n");
  SimObj::sim_out.write("  Apply Phase Durations:    " + std::to_string(apply_cycles) + " cycles\n");

  dram->print_stats();

  for(uint64_t i = 0; i < opt.num_pipelines; i++) {
    delete tile->operator[](i);
  }

#if defined(APP_PR)
  for(uint64_t i = 0; i < opt.num_pipelines; i++) {
    process->at(i)->clear();
    delete (*process)[i];
  }
#endif
  process->clear();
  delete process;
  process = NULL;

  graph.writeVertexProperties(opt.result);

  return 0;
}
