/*
 * Andrew Smith
 *
 * WriteTempDstProperty
 *  This is the last stage in the processing phase, it writes the temp value
 *  back to memory and tells the ControlAtomicUpdate module a edge has finished.
 */

#ifndef WRITETEMPDSTPROPERTY_H
#define WRITETEMPDSTPROPERTY_H

#include <iostream>
#include <vector>
#include <cstdint>
#include <map>
#include <list>

#include "module.h"
#include "memory.h"
#include "controlAtomicUpdate.h"

#include "readGraph.h"

namespace SimObj {

template<class v_t, class e_t>
class WriteTempDstProperty : public Module<v_t, e_t> {
private:
  enum op_t {
    OP_WAIT,
    OP_MEM_WAIT,
    OP_NUM_OPS
  };

#ifdef DEBUG
  std::map<int, std::string> _state_name = {
    {0, "OP_WAIT"},
    {1, "OP_MEM_WAIT"},
    {2, "OP_NUM_OPS"}};
#endif

  using Module<v_t, e_t>::_tick;
  using Module<v_t, e_t>::_ready;
  using Module<v_t, e_t>::_stall;
  using Module<v_t, e_t>::_next;
  using Module<v_t, e_t>::_data;
  using Module<v_t, e_t>::_name;
  using Module<v_t, e_t>::_stall_ticks;
  using Module<v_t, e_t>::_has_work;
  using Module<v_t, e_t>::_items_processed;
#if MODULE_TRACE
  using Module<v_t, e_t>::_in_data;
  using Module<v_t, e_t>::_in_logger;
  using Module<v_t, e_t>::_out_logger;
  bool ready_prev;
  bool mem_flag_prev;
  uint64_t address_prev;
  v_t mem_out_prev;
  bool ready_curr;
  bool mem_flag_curr;
  uint64_t address_curr;
  v_t mem_out_curr;

  void update_logger();
#endif

  Memory* _scratchpad;
  op_t _state;
  ControlAtomicUpdate<v_t, e_t>* _cau;
  uint64_t _id;
  uint64_t _curr_addr;

  std::map<uint64_t, Utility::pipeline_data<v_t, e_t>>* _scratch_mem;
  std::list<uint64_t>* _apply;

  uint64_t _edges_written;

public:
  bool _mem_flag;
  WriteTempDstProperty(Memory* scratchpad, ControlAtomicUpdate<v_t, e_t>* cau, std::map<uint64_t, Utility::pipeline_data<v_t, e_t>>* scratch_mem, std::list<uint64_t>* apply, std::string name, uint64_t id);
  ~WriteTempDstProperty();

  void tick(void);
  //void print_stats(void);
  //void print_stats_csv(void);
};

} // namespace SimObj

#include "writeTempDstProperty.tcc"

#endif // WRITETMPDSTPROPERTY_H
