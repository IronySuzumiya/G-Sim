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
  _tick++;
  _mem->ClockTick();
  for(auto it = _pending_queue.begin(); it != _pending_queue.end(); it++) {
    if(_mem->WillAcceptTransaction(std::get<0>(*it), std::get<3>(*it))) {
      std::tuple<uint64_t, bool*, bool> transaction = std::make_tuple(std::get<0>(*it), std::get<1>(*it), std::get<2>(*it));
      if(std::get<3>(*it)) {
        _write_queue.push_back(transaction);
      } else {
        _read_queue.push_back(transaction);
      }
      _mem->AddTransaction(std::get<0>(*it), std::get<3>(*it));
      it = _pending_queue.erase(it);
    }
  }
}
  
void SimObj::DRAM::write(uint64_t addr, bool* complete, bool sequential) {
#ifdef DEBUG
  //std::cout << "DRAM Write Issued @ " << _tick << " with address: " << std::hex << addr << "\n";
#endif
  if(_mem->WillAcceptTransaction(addr, true)) {
    std::tuple<uint64_t, bool*, bool> transaction = std::make_tuple(addr, complete, sequential);
    _mem->AddTransaction(addr, true);
    _write_queue.push_back(transaction);
  } else {
    std::tuple<uint64_t, bool*, bool, bool> transaction = std::make_tuple(addr, complete, sequential, true);
    _pending_queue.push_back(transaction);
  }
}

void SimObj::DRAM::read(uint64_t addr, bool* complete, bool sequential) {
#ifdef DEBUG
  //std::cout << "DRAM Read  Issued @ " << _tick << " with address: " << std::hex << addr << "\n";
#endif
  if(_mem->WillAcceptTransaction(addr, false)) {
    std::tuple<uint64_t, bool*, bool> transaction = std::make_tuple(addr, complete, sequential);
    _mem->AddTransaction(addr, false);
    _read_queue.push_back(transaction);
  } else {
    std::tuple<uint64_t, bool*, bool, bool> transaction = std::make_tuple(addr, complete, sequential, false);
    _pending_queue.push_back(transaction);
  }
}

void SimObj::DRAM::read_complete(uint64_t address) {
  for(auto it = _read_queue.begin(); it != _read_queue.end(); it++) {
    if(std::get<0>(*it) == address) {
      *(std::get<1>(*it)) = true;
      it = _read_queue.erase(it);
      return;
    }
  }
  assert(false); 
}

void SimObj::DRAM::write_complete(uint64_t address) {
  //Dequeue the transaction pair from the list of outstanding transactions:
  for(auto it = _write_queue.begin(); it != _write_queue.end(); it++) {
    if(std::get<0>(*it) == address) {
      *(std::get<1>(*it)) = true;
      it = _write_queue.erase(it);
      return;
    }
  }
  assert(false);
}

void SimObj::DRAM::print_stats() {
  _mem->PrintStats();
}

void SimObj::DRAM::reset() {

}
