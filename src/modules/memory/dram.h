/*
 * Andrew Smith
 * 
 * DRAM Memory. Derived from Memory class, Uses DRAMSim3 to model the timing
 *
 */

#ifndef _DRAM_H
#define _DRAM_H

#include <list>
#include <tuple>
#include <vector>
#include "dramsim3.h"
#include "memory.h"

namespace SimObj {

class DRAM : public Memory {
private:
  std::list<std::tuple<uint64_t, bool*, bool>> _write_queue;
  std::list<std::tuple<uint64_t, bool*, bool>> _read_queue;
  std::list<std::tuple<uint64_t, bool*, bool>> _pending_queue;

  std::function<void(uint64_t)> write_cb;
  std::function<void(uint64_t)> read_cb;

  dramsim3::MemorySystem *_mem;

  // DRAMSim3 Callbacks:
  void read_complete(uint64_t address);
  void write_complete(uint64_t address);

public:
  DRAM(uint64_t access_latency=0, uint64_t write_latency=0);
  ~DRAM();

  void tick(void);
  void write(uint64_t addr, bool* complete);
  void read(uint64_t addr, bool* complete);

  void print_stats();
  void reset();
}; // class DRAM

} // namespace SimObj

#endif
