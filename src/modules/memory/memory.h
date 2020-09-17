/*
 * Andrew Smith
 *
 * Memory Base Class
 *
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <queue>
#include <vector>
#include "dramsim3.h"

namespace SimObj {

enum mem_op_t {
  MEM_READ,
  MEM_WRITE,
  MEM_ALLOC,
  MEM_PREFETCH,
  MEM_NUM_OPS
};

enum mem_status_t {
  MEM_INACTIVE,
  MEM_ACTIVE,
  MEM_PROCESSING,
  MEM_NUM_STATUS
};

class MemRequest {
private:
  bool* _complete;
  uint64_t _start;
  uint64_t _delay;
  mem_status_t _status;
  mem_op_t _type;
  uint64_t _addr;

public:
  MemRequest();
  MemRequest(uint64_t addr, bool* complete, uint64_t delay, mem_op_t type);
  ~MemRequest();

  void set_start(uint64_t start);
  void set_status(mem_status_t status);
  mem_status_t get_status(void);
  uint64_t get_finish_tick(void);
  mem_op_t get_type(void);
  uint64_t get_addr(void);
  bool* get_complete_ptr(void);
  bool is_complete(void);
  void complete(void);
};

class Memory {
protected:
  uint64_t _tick;
  uint64_t _access_latency;
  uint64_t _write_latency;
  uint64_t _num_simultaneous_requests;
  std::vector<MemRequest> _action;
  std::queue<MemRequest> _req_queue;
  std::string _name;

public:
  Memory(void);
  Memory(uint64_t access_latency, uint64_t write_latency, uint64_t num_simultaneous_requests);
  virtual ~Memory();

  virtual void tick(void);
  virtual void write(uint64_t addr, bool* complete, bool sequential=true);
  virtual void read(uint64_t addr, bool* complete, bool sequential=true);
  virtual void alloc(uint64_t addr, bool* complete);
  virtual void prefetch(uint64_t addr, bool *complete);
  virtual void print_stats();
  virtual void print_stats_csv();
  virtual void reset();
  virtual void set_name(std::string name);
};

} // namespace SimObj
#endif
