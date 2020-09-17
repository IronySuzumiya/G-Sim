/*
 * Andrew Smith
 *
 * Apply
 *  User Defined Function to Apply a Property to a vertex.
 *  min(in the case of SSSP)
 *
 */

#ifndef APPLY_H
#define APPLY_H

#include <iostream>
#include <vector>
#include <cstdint>
#include <map>

#include "module.h"
#include "memory.h"

#include "graphMat.h"

namespace SimObj {

template<class v_t, class e_t>
class Apply : public Module<v_t, e_t> {
private:
  enum op_t {
    OP_WAIT,
    OP_COUNT,
    OP_NUM_OPS
  };

  std::map<int, std::string> _state_name = {
    {0, "OP_WAIT"},
    {1, "OP_COUNT"},
    {4, "OP_NUM_OPS"}};

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
  bool complete_prev;
  bool send_prev;
  bool ready_curr;
  bool complete_curr;
  bool send_curr;

  void update_logger();
#endif

  uint64_t _id;
  op_t _state;
  uint64_t _counter;
  uint64_t _delay_cycles;
  GraphMat::GraphApp<v_t, e_t>* _app;

public:
  Apply(int delay_cycles, GraphMat::GraphApp<v_t, e_t>* app, std::string name, uint64_t id);
  ~Apply();

  void tick(void) override;
};

} // namespace SimObj

#include "apply.tcc"

#endif // APPLY_H
