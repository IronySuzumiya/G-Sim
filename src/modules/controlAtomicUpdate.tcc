/*
 * Andrew Smith
 *
 * Control Atomic Update:
 *  The control atomic update module is part of the atomic update sequence of the
 *  graphicionado pipeline.
 *
 */

#include <cassert>
#include <sstream>

template<class v_t, class e_t>
SimObj::ControlAtomicUpdate<v_t, e_t>::ControlAtomicUpdate(std::string name, uint64_t id) {
  _id = id;
  _name = name;
  _state = OP_WAIT;
  _ready = false;
  _op_complete = false;
  _items_stalled = 0;
  _items_directly_passed = 0;
#if MODULE_TRACE
  ready_prev = false;
  send_prev = false;
  dep_prev[0] = 0;
  dep_prev[1] = 0;
  dep_prev[2] = 0;
  ready_curr = false;
  send_curr = false;
  dep_curr[0] = 0;
  dep_curr[1] = 0;
  dep_curr[2] = 0;
  _in_logger = new Utility::Log("trace/"+name+"_"+std::to_string(_id)+"_in.csv");
  _out_logger = new Utility::Log("trace/"+name+"_"+std::to_string(_id)+"_out.csv");
  assert(_in_logger != NULL);
  assert(_out_logger != NULL);
#endif
}

template<class v_t, class e_t>
SimObj::ControlAtomicUpdate<v_t, e_t>::~ControlAtomicUpdate() {
  // Do nothing
}

template<class v_t, class e_t>
bool SimObj::ControlAtomicUpdate<v_t, e_t>::dependency() {
  bool ret = false;
  for(auto it = _nodes.begin(); it != _nodes.end(); it++) {
    if(_data.vertex_dst_id == it->vertex_dst_id) {
      ret = true;
      break;
    }
  }
#ifdef MODULE_TRACE
  int i = 0;
  for(auto it = _nodes.begin(); it != _nodes.end(); it++) {
    dep_curr[i] = it->vertex_dst_id;
    i++;
  }
  for(int j = i; j < 3; j++) {
    dep_curr[j] = 0;
  }
#endif

  return ret;
}

template<class v_t, class e_t>
void SimObj::ControlAtomicUpdate<v_t, e_t>::print_stats() {
  sim_out.write("-------------------------------------------------------------------------------\n");
  sim_out.write("[ " + _name + " ]\n");
  sim_out.write("  Stalls:\n");
  sim_out.write("    STALL_CAN_ACCEPT: " + std::to_string(_stall_ticks[STALL_CAN_ACCEPT]) + " cycles\n");
  sim_out.write("    STALL_PROCESSING: " + std::to_string(_stall_ticks[STALL_PROCESSING]) + " cycles\n");
  sim_out.write("    STALL_PIPE:       " + std::to_string(_stall_ticks[STALL_PIPE]) + " cycles\n");
  sim_out.write("    STALL_MEM:        " + std::to_string(_stall_ticks[STALL_MEM]) + " cycles\n");
  sim_out.write("  Performance:\n");
  sim_out.write("    Items Directly Passed:  " + std::to_string(_items_directly_passed) + "\n");
  sim_out.write("    Items Stalled:  " + std::to_string(_items_stalled) + "\n");
}

template<class v_t, class e_t>
void SimObj::ControlAtomicUpdate<v_t, e_t>::print_stats_csv() {
  sim_out.write(_name + "," 
    + std::to_string(_stall_ticks[STALL_CAN_ACCEPT]) + ","
    + std::to_string(_stall_ticks[STALL_PROCESSING]) + ","
    + std::to_string(_stall_ticks[STALL_PIPE]) + ","
    + std::to_string(_stall_ticks[STALL_MEM]) + ","
    + std::to_string(_items_directly_passed) + ","
    + std::to_string(_items_stalled) + "\n");
}

template<class v_t, class e_t>
void SimObj::ControlAtomicUpdate<v_t, e_t>::tick(void) {
  _tick++;
  op_t next_state = _state;
#ifdef MODULE_TRACE
  ready_curr = _ready;
  send_curr = _next->is_stalled() == STALL_CAN_ACCEPT;
#endif

  // Module State Machine
  switch(_state) {
    case OP_WAIT : {
      if(_ready) {
        _ready = false;
        if(!dependency() && _next->is_stalled() == STALL_CAN_ACCEPT) {
          _next->ready(_data);
          _nodes.push_front(_data);
          next_state = OP_WAIT;
          _stall = STALL_CAN_ACCEPT;
          _has_work = false;
          ++_items_directly_passed;
          ++_items_processed;
        }
        else {
          next_state = OP_STALL;
          _stall = STALL_PIPE;
        }
      }
      else {
        next_state = OP_WAIT;
        _stall = STALL_CAN_ACCEPT;
        _has_work = false;
      }
      break;
    }
    // If there is a dep stall the pipe until the edge completes
    case OP_STALL : {
      // Check if an edge was finalized:
      //std::cout << "Dependency: " << dependency() << " Queue Size: " << _nodes.size() << "\n";
      if(!dependency() && _next->is_stalled() == STALL_CAN_ACCEPT) {
        _next->ready(_data);
        _nodes.push_front(_data);
        next_state = OP_WAIT;
        _stall = STALL_CAN_ACCEPT;
        _has_work = false;
        ++_items_stalled;
        ++_items_processed;
      }
      else {
        next_state = OP_STALL;
        _stall = STALL_PIPE;
      }
      break;
    }
    default : {

    }
  }
#if 0
  if(_state != next_state) {
    std::cout << "[ " << __PRETTY_FUNCTION__ << " ] tick: " << _tick << "  state: " << _state_name[_state] << "  next_state: " << _state_name[next_state] << "\n";
  }
#endif
#ifdef MODULE_TRACE
  update_logger();
#endif
  _state = next_state;
  this->update_stats();
}

template<class v_t, class e_t>
Utility::pipeline_data<v_t, e_t> SimObj::ControlAtomicUpdate<v_t, e_t>::signal(void) {
  Utility::pipeline_data<v_t, e_t> ret = _nodes.back();
  _nodes.pop_back();
  return ret;
}


template<class v_t, class e_t>
void SimObj::ControlAtomicUpdate<v_t, e_t>::debug(void) {
  std::cout << "[";
  for(auto it = _nodes.begin(); it != _nodes.end(); it++) {
    std::cout << *it << ",";
  }
  std::cout << "]\n";
}

template<class v_t, class e_t>
void SimObj::ControlAtomicUpdate<v_t, e_t>::clear_stats() {
  Module<v_t, e_t>::clear_stats();
  _items_stalled = 0;
  _items_directly_passed = 0;
}

#ifdef MODULE_TRACE
template<class v_t, class e_t>
void SimObj::ControlAtomicUpdate<v_t, e_t>::update_logger(void) {
  std::stringstream out;
  if(ready_prev != ready_curr ||
     dep_prev[0] != dep_curr[0] ||
     dep_prev[1] != dep_curr[1] ||
     dep_prev[2] != dep_curr[2] ||
     send_prev != send_curr) {
    if(_in_logger != NULL) {
      out << _tick << ",";
      out << _in_data << ",";
      out << ready_curr << ",";
      out << send_curr << ",";
      out << dep_curr[0] << ",";
      out << dep_curr[1] << ",";
      out << dep_curr[2] << "\n";
      _in_logger->write(out.str());
      //_in_logger->write(std::to_string(_tick)+","+
      //               std::to_string(_in_data.vertex_id)+","+
      //               std::to_string(_in_data.vertex_id_addr)+","+
      //               std::to_string(_in_data.vertex_dst_id)+","+
      //               std::to_string(_in_data.vertex_dst_id_addr)+","+
      //               std::to_string(_in_data.edge_id)+","+
      //               std::to_string(_in_data.vertex_data)+","+
      //               std::to_string(_in_data.vertex_dst_data)+","+
      //               std::to_string(_in_data.message_data)+","+
      //               std::to_string(_in_data.vertex_temp_dst_data)+","+
      //               std::to_string(_in_data.edge_data)+","+
      //               std::to_string(_in_data.edge_temp_data)+","+
      //               std::to_string(_in_data.last_vertex)+","+
      //               std::to_string(_in_data.last_edge)+","+
      //               std::to_string(_in_data.updated)+","+
      //               std::to_string(ready_curr)+","+
      //               std::to_string(send_curr)+","+
      //               std::to_string(dep_curr[0])+","+
      //               std::to_string(dep_curr[1])+","+
      //               std::to_string(dep_curr[2])+"\n");
    }
    ready_prev = ready_curr;
    send_prev = send_curr;
    dep_prev[0] = dep_curr[0];
    dep_prev[1] = dep_curr[1];
    dep_prev[2] = dep_curr[2];
  }
}
#endif
