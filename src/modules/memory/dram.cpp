/*
 * Andrew Smith
 * 
 * DRAM Memory. Derived from Memory class, Uses DRAMSim3.
 *
 */

#include <iostream>
#include <cassert>

#include "dram.h"

SimObj::DRAM::DRAM(uint64_t access_latency, uint64_t write_latency) {
  _tick = 0;
  _access_latency = access_latency;
  _write_latency = write_latency;
  read_cb = std::bind(&DRAM::read_complete, this, std::placeholders::_1);
  write_cb = std::bind(&DRAM::write_complete, this, std::placeholders::_1);
  _mem = dramsim3::GetMemorySystem("modules/memory/DRAMsim3/configs/DDR4_8Gb_x16_3200.ini", "./", read_cb, write_cb);
}

SimObj::DRAM::~DRAM() {

}

void SimObj::DRAM::tick(void) {
  static uint64_t counter1, counter2;
  counter1 += 16;
  while(counter2 < counter1) {
    counter2 += 10;
    _tick++;
    _mem->ClockTick();
#if 0
    for(auto it = _read_queue.begin(); it != _read_queue.end(); ) {
      if(_tick - std::get<3>(*it) > 20000) {
        std::cerr << "DRAM Not Responding: read addr = " << std::to_string(std::get<0>(*it)) << std::endl;
        assert(false);
      }
    }
    for(auto it = _write_queue.begin(); it != _write_queue.end(); ) {
      if(_tick - std::get<3>(*it) > 20000) {
        std::cerr << "DRAM Not Responding: write addr = " << std::to_string(std::get<0>(*it)) << std::endl;
        assert(false);
      }
    }
#endif
    for(auto it = _pending_queue.begin(); it != _pending_queue.end(); ) {
      if(_mem->WillAcceptTransaction(std::get<0>(*it), std::get<2>(*it))) {
        if(std::get<2>(*it)) {
          _write_queue.push_back(*it);
        } else {
          _read_queue.push_back(*it);
        }
        _mem->AddTransaction(std::get<0>(*it), std::get<2>(*it));
        it = _pending_queue.erase(it);
      } else {
        //++it;
        break;
      }
    }
  }
  if(counter1 == counter2) {
    counter1 = 0;
    counter2 = 0;
  }
}
  
void SimObj::DRAM::write(uint64_t addr, bool* complete, bool sequential) {
  //std::cout << "DRAM Write Issued @ " << _tick << " with address: " << std::hex << addr << "\n";
  std::tuple<uint64_t, bool*, bool, uint64_t> transaction = std::make_tuple(addr, complete, true, _tick);
  if(_mem->WillAcceptTransaction(addr, true)) {
    _mem->AddTransaction(addr, true);
    _write_queue.push_back(transaction);
  } else {
    _pending_queue.push_back(transaction);
  }
}

void SimObj::DRAM::read(uint64_t addr, bool* complete, bool sequential) {
  //std::cout << "DRAM Read Issued @ " << _tick << " with address: " << std::hex << addr << "\n";
  std::tuple<uint64_t, bool*, bool, uint64_t> transaction = std::make_tuple(addr, complete, false, _tick);
  if(_mem->WillAcceptTransaction(addr, false)) {
    _mem->AddTransaction(addr, false);
    _read_queue.push_back(transaction);
  } else {
    _pending_queue.push_back(transaction);
  }
}

void SimObj::DRAM::alloc(uint64_t addr, bool* complete) {
  assert(false);
}

void SimObj::DRAM::prefetch(uint64_t addr, bool* complete) {
  assert(false);
}

void SimObj::DRAM::read_complete(uint64_t address) {
  for(auto it = _read_queue.begin(); it != _read_queue.end(); ) {
    if(std::get<0>(*it) == address) {
      *(std::get<1>(*it)) = true;
      it = _read_queue.erase(it);
      return;
    } else {
      ++it;
    }
  }
  assert(false); 
}

void SimObj::DRAM::write_complete(uint64_t address) {
  for(auto it = _write_queue.begin(); it != _write_queue.end(); ) {
    if(std::get<0>(*it) == address) {
      *(std::get<1>(*it)) = true;
      it = _write_queue.erase(it);
      return;
    } else {
      ++it;
    }
  }
  assert(false);
}

void SimObj::DRAM::print_stats() {
  _mem->PrintStats();
}

void SimObj::DRAM::reset() {

}
