/*
 * Andrew Smith
 *
 * Read DST Property:
 *  Read the destination property from DRAM. Optional depending on the 
 *  graph algorithm
 *
 */

#ifndef READTEMPDSTPROPERTY_H
#define READTEMPDSTPROPERTY_H

#include <iostream>
#include <vector>
#include <cstdint>
#include <map>

#include "module.h"
#include "memory.h"
#include "scratchpad.h"

namespace SimObj {

template<class v_t, class e_t>
class ReadTempDstProperty : public Module<v_t, e_t> {
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
  using Module<v_t, e_t>::_has_work;
  using Module<v_t, e_t>::_items_processed;
#if MODULE_TRACE
  using Module<v_t, e_t>::_in_data;
  using Module<v_t, e_t>::_in_logger;
  using Module<v_t, e_t>::_out_logger;
  bool ready_prev;
  bool mem_flag_prev;
  bool send_prev;
  uint64_t address_prev;
  v_t mem_result_prev;
  bool ready_curr;
  bool mem_flag_curr;
  bool send_curr;
  uint64_t address_curr;
  v_t mem_result_curr;

  void update_logger();
#endif

  Memory* _scratchpad;
  op_t _state;
  std::map<uint64_t, Utility::pipeline_data<v_t, e_t>>* _scratch_mem;
  uint64_t _id;

public:
  bool _mem_flag;
  ReadTempDstProperty(Memory* scratchpad, std::map<uint64_t, Utility::pipeline_data<v_t, e_t>>* scratch_mem, std::string name, uint64_t id);
  ~ReadTempDstProperty();

  void tick(void);
};

} // namespace SimObj

#include "readTempDstProperty.tcc"

#endif
