/*
 *
 * Andrew Smith
 * 
 * Really Simple Fully Associative Cache Model
 *   Used for sequential reads to reduce memory requests.
 *
 */

#ifndef CACHE_H
#define CACHE_H

#include <iostream>
#include <list>
#include <vector>
#include <map>
#include <random>
#include <time.h>

#include "cacheLine.h"
#include "log.h"

namespace SimObj {

class Cache : public Memory {
private:
  struct mshr_t {
    uint64_t address;
    bool write;
    bool complete;
    bool prefetch;
  };

  struct prefetch_t {
    uint64_t address;
    bool issued;
    bool complete;
  };

  std::vector<CacheLine> cache;
  std::list<mshr_t> mshr;
  //std::list<uint64_t> conflict_insert_queue;
  //uint64_t conflict_insert_queue_max_len;

  std::list<std::pair<uint64_t, bool*>> outstanding_sequential_reads;
  // Address, Accessed this Cycle, Prefetch Issued
  //std::map<uint64_t, std::pair<bool, bool>> prefetched;

  Memory* _dram;

  //Stats:
  enum stat_t {
    HIT,
    MISS,
    SEQ_READ,
    SEQ_WRITE,
    RAND_READ,
    RAND_WRITE,
    ALLOCATE,
    PREFETCH,
    WRITEBACK,
    NUM_STATS
  };
  std::vector<uint64_t> stats;

  uint64_t getLRU(uint64_t addr);
  uint64_t getRandom(uint64_t addr);
  uint64_t hit(uint64_t addr);
  void insert(uint64_t line, uint64_t addr, bool prefetch=false);

  uint64_t _set_size;
  uint64_t _num_sets;

  std::default_random_engine _random;

  uint64_t _access_mask;
  uint64_t _access_shift;

public:
  Cache(uint64_t entries, uint64_t entry_size, uint64_t num_set_associative_way, Memory* dram);
  ~Cache();
  
  void tick(void) override;
  void write(uint64_t addr, bool* complete, bool sequential=true) override;
  void read(uint64_t addr, bool* complete, bool sequential=true) override;
  void alloc(uint64_t addr, bool* complete) override;
  void prefetch(uint64_t addr, bool* complete) override;

  void print_stats() override;
  void print_stats_csv() override;
  void reset() override;
}; // class Cache

}; // namespace SimObj

#endif // CACHE_H
