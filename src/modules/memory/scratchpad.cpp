#include "scratchpad.h"
#include <cassert>

void SimObj::Scratchpad::tick() {
  _tick++;
  _mem->tick();
  for(auto it = _action.begin(); it != _action.end(); it++) {
    switch(it->get_status()) {
      case MEM_INACTIVE:
        if(!_req_queue.empty()) {
          *it = _req_queue.front();
          _req_queue.pop();
          it->set_status(MEM_ACTIVE);
          it->set_start(_tick);
        }
        break;
      case MEM_ACTIVE:
        if(it->get_finish_tick() <= _tick) {
          switch(it->get_type()) {
            case MEM_READ:
              _mem->read(it->get_addr(), it->get_complete_ptr());
              break;
            case MEM_WRITE:
              _mem->write(it->get_addr(), it->get_complete_ptr());
              break;
            case MEM_ALLOC:
              _mem->alloc(it->get_addr(), it->get_complete_ptr());
              break;
            default: assert(false);
          }
          it->set_status(MEM_PROCESSING);
        }
        //break; // shortcut
      case MEM_PROCESSING:
        if(it->is_complete()) {
          it->set_status(MEM_INACTIVE);
        }
        break;
      default: assert(false);
    }
  }
}

void SimObj::Scratchpad::print_stats() {
  _mem->print_stats();
}

void SimObj::Scratchpad::print_stats_csv() {
  _mem->print_stats_csv();
}

void SimObj::Scratchpad::reset() {
  _mem->reset();
}

void SimObj::Scratchpad::set_name(std::string name) {
  _name = name;
  _mem->set_name(name);
}
