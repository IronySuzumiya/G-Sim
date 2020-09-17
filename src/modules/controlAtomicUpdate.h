/*
 * Andrew Smith
 *
 * Control Atomic Update:
 *  The control atomic update module is part of the atomic update sequence of the
 *  graphicionado pipeline.
 *
 */

#ifndef CONTROLATOMICUPDATE_H
#define CONTROLATOMICUPDATE_H

#include <iostream>
#include <vector>
#include <cstdint>
#include <map>
#include <list>

#include "module.h"
#include "memory.h"

#include "readGraph.h"

namespace SimObj {

template<class v_t, class e_t>
class ControlAtomicUpdate : public Module<v_t, e_t> {
private:
  enum op_t {
    OP_WAIT,
    OP_STALL,
    OP_NUM_OPS
  };

#ifdef DEBUG
  std::map<int, std::string> _state_name = {
    {0, "OP_WAIT"},
    {1, "OP_STALL"},
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
  using Module<v_t, e_t>::_stall_ticks;
#if MODULE_TRACE
  using Module<v_t, e_t>::_in_data;
  using Module<v_t, e_t>::_in_logger;
  using Module<v_t, e_t>::_out_logger;
  bool ready_prev;
  bool send_prev;
  uint64_t dep_prev[3];
  bool ready_curr;
  bool send_curr;
  uint64_t dep_curr[3];

  void update_logger();
#endif

  uint64_t _items_directly_passed;
  uint64_t _items_stalled;

  op_t _state;
  bool _op_complete;
  std::list<Utility::pipeline_data<v_t, e_t>> _nodes;
  uint64_t _id;

  bool dependency();

public:
  void print_stats() override;
  void print_stats_csv() override;
  void clear_stats() override;

  ControlAtomicUpdate(std::string name, uint64_t id);
  ~ControlAtomicUpdate();

  void tick(void) override;
  Utility::pipeline_data<v_t, e_t> signal();
  void debug(void);
};

} // namespace SimObj

#include "controlAtomicUpdate.tcc"

#endif
