#ifndef SCRATCHPAD_H
#define SCRATCHPAD_H

#include <list>
#include <tuple>

#include "memory.h"
#include "cache.h"

namespace SimObj {

enum scratchpad_op_t {
  SP_READ,
  SP_WRITE,
  SP_ALLOC,
  SP_NUM_OPS
};

class Scratchpad : public Memory {
private:
  Memory *_mem;
public:
  Scratchpad(uint64_t entries, uint64_t entry_size, uint64_t num_set_associative_way, Memory* dram, uint64_t access_latency, uint64_t write_latency, uint64_t num_simultaneous_requests)
    : Memory(access_latency, write_latency, num_simultaneous_requests), _mem(new Cache(entries, entry_size, num_set_associative_way, dram)) { }
  ~Scratchpad() { delete _mem; }
  void tick(void);
  void print_stats(void);
  void print_stats_csv(void);
  void reset(void);
  void set_name(std::string name);
};

};

#endif // SCRATCHPAD_H
