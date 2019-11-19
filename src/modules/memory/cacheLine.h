/*
 * Andrew Smith
 *
 *
 *
 */

#ifndef CACHELINE_H
#define CACHELINE_H

#include <iostream>
#include <vector>

#include "dram.h"
#include "log.h"

#define ACCESS_MASK 0x3F
#define LINE_SIZE 64

namespace SimObj {

class CacheLine {
private:
  uint64_t _address;
  bool _dirty;
  bool _valid;
  uint64_t _lru;

public:
  CacheLine();
  ~CacheLine();

  void access(bool is_write);
  bool hit(uint64_t addr);
  bool valid();
  bool dirty();
  uint64_t getAddr();
  uint64_t getLRU();
  void tick();
  
  void insert(uint64_t addr);
  void evict();
}; // class CacheLine

}; // namespace SimObj

#endif // CACHELINE_H
