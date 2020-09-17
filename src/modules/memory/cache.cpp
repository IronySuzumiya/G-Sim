/*
 *
 * Andrew Smith
 *
 * Really simple cache model. Used to buffer
 * the memory during the sequential accesses
 */

#include <cassert>
#include "cache.h"

SimObj::Cache::Cache(uint64_t entries, uint64_t entry_size, uint64_t num_set_associative_way, Memory* dram) {
  assert(dram != NULL);
  assert((entry_size & (entry_size - 1)) == 0);
  assert((num_set_associative_way & (num_set_associative_way - 1)) == 0);
  assert(entries % num_set_associative_way == 0);

  _tick = 0;

  //conflict_insert_queue_max_len = 0;

  cache.resize(entries);
  stats.resize(NUM_STATS, 0);

  _set_size = num_set_associative_way;
  _num_sets = entries / num_set_associative_way;

  _random = std::default_random_engine(static_cast<uint64_t>(time(nullptr)));

  _access_mask = entry_size - 1;

  _access_shift = 0;
  while(entry_size > 1) {
    entry_size >>= 1;
    ++_access_shift;
  }

  _dram = dram;
}

SimObj::Cache::~Cache() {
  _dram = NULL;
}

uint64_t SimObj::Cache::getLRU(uint64_t addr) {
  uint64_t set_index = (addr >> _access_shift) % _num_sets;
  uint64_t i = set_index * _set_size;
  uint64_t pos = set_index * _set_size;
  uint64_t max = 0;
  for(auto it = cache.begin() + set_index * _set_size; it != cache.begin() + (set_index + 1) * _set_size; ++it) {
    if(it->getLRU() > max) {
      max = it->getLRU();
      pos = i;
    }
    ++i;
  }
  return pos;
}

uint64_t SimObj::Cache::getRandom(uint64_t addr) {
  uint64_t set_index = (addr >> _access_shift) % _num_sets;
  std::uniform_int_distribution<uint64_t> pos(set_index * _set_size, (set_index + 1) * _set_size - 1);
  return pos(_random);
}

uint64_t SimObj::Cache::hit(uint64_t addr) {
  uint64_t set_index = (addr >> _access_shift) % _num_sets;
  uint64_t i = set_index * _set_size;
  for(auto it = cache.begin() + set_index * _set_size; it != cache.begin() + (set_index + 1) * _set_size; ++it) {
    if(it->hit(addr & ~_access_mask)) {
      return i;
    }
    ++i;
  }
  return cache.size();
}

void SimObj::Cache::insert(uint64_t line, uint64_t addr, bool prefetch) {
  cache[line].insert(addr & ~_access_mask, prefetch);
}

void SimObj::Cache::tick(void) {
  // Search through the MSHR list and see if any outstanding requests completed
  // If it is a writeback remove and do nothing
  // If it is a read insert it into the cache
  //_memory->tick();
  _tick++;
  // Tick all the cache lines
  for(auto & line : cache) {
    line.tick();
  }
  #if 0
  for(auto it = conflict_insert_queue.begin(); it != conflict_insert_queue.end(); ) {
    uint64_t line = getLRU(*it);
    if(cache[line].getLRU() != 0) {
      if(cache[line].getPrefetch()) {
        // When evicting line, check if it was inseted by prefetching mechanism,
        // If it was, then remove the entry from the list of prefectched addresses
      }
      if(cache[line].valid() && cache[line].dirty()) {
        stats[WRITEBACK]++;
        mshr_t req;
        req.address = cache[line].getAddr();
        req.write = true;
        req.complete = false;
        req.prefetch = false;
        mshr.push_front(req);
        _dram->write(mshr.front().address, &mshr.front().complete, false);
      }
      insert(line, *it);
      it = conflict_insert_queue.erase(it);
    } else {
      ++it;
    }
  }
  #endif
  for(auto it = mshr.begin(); it != mshr.end(); ) {
    if(it->complete) {
      if(!it->write) {
        uint64_t line = getLRU(it->address);
        if(cache[line].getLRU() != 0) {
          if(cache[line].getPrefetch()) {
            // When evicting line, check if it was inseted by prefetching mechanism,
            // If it was, then remove the entry from the list of prefectched addresses
          }
          if(cache[line].valid() && cache[line].dirty()) {
            stats[WRITEBACK]++;
            mshr_t req;
            req.address = cache[line].getAddr();
            req.write = true;
            req.complete = false;
            req.prefetch = false;
            mshr.push_front(req);
            _dram->write(mshr.front().address, &mshr.front().complete, false);
          }
          insert(line, it->address, it->prefetch);
        } else {
          //conflict_insert_queue.push_back(it->address);
          //conflict_insert_queue_max_len = std::max(conflict_insert_queue_max_len, conflict_insert_queue.size());
          assert(false);
        }
      }
      it = mshr.erase(it);
      // only one req dealt each cycle
      break;
    } else {
      ++it;
    }
  }
  // Complete any outstanding requests:
  for(auto it = outstanding_sequential_reads.begin(); it != outstanding_sequential_reads.end(); ) {
    if(hit(it->first) < cache.size()) {
      cache[hit(it->first)].access(false);
      *(it->second) = true;
      it = outstanding_sequential_reads.erase(it);
    } else {
      ++it;
    }
  }
}

void SimObj::Cache::write(uint64_t addr, bool* complete, bool sequential) {
  stats[sequential? SEQ_WRITE : RAND_WRITE]++;
  uint64_t line = hit(addr);
  if(line < cache.size()) {
    stats[HIT]++;
    cache[line].access(true);
    *complete = true;
    if(!sequential) {
      // Write through
      stats[WRITEBACK]++;
      cache[line].writeback();
      mshr_t req;
      req.address = cache[line].getAddr();
      req.write = true;
      req.complete = false;
      req.prefetch = false;
      mshr.push_front(req);
      _dram->write(mshr.front().address, &mshr.front().complete, false);
    }
  }
  else {
    if(sequential) {
      stats[MISS]++;
      line = getLRU(addr);
      if(cache[line].valid() && cache[line].dirty()) {
        stats[WRITEBACK]++;
        mshr_t req;
        req.address = cache[line].getAddr();
        req.write = true;
        req.complete = false;
        req.prefetch = false;
        mshr.push_front(req);
        _dram->write(mshr.front().address, &mshr.front().complete);
      }
      insert(line, addr);
      cache[line].access(true);
      *complete = true;
    } else {
      _dram->write((addr & ~_access_mask), complete, false);
    }
  }
}

/* 
 * SimObj::Cache::read
 *   If sequential Read, add req to MSHR if the line is not in the cache
 *   Else if in the cache access the line and set the req to complete
 *   If not sequential, submit a req to DRAM
 */
void SimObj::Cache::read(uint64_t addr, bool* complete, bool sequential) {
  stats[sequential? SEQ_READ : RAND_READ]++;
  uint64_t line = hit(addr);
  if(line < cache.size()) {
    stats[HIT]++;
    cache[line].access(false);
    *complete = true;
  }
  else {
    stats[MISS]++;
    bool is_dup_read = false;
    for (auto it : mshr) {
      if(it.address == (addr & ~_access_mask)) {
        is_dup_read = true;
        break;
      }
    }
    if(sequential) {
      if(!is_dup_read) {
        mshr_t req;
        req.address = (addr & ~_access_mask);
        req.write = false;
        req.complete = false;
        req.prefetch = false;
        mshr.push_front(req);
        _dram->read(mshr.front().address, &mshr.front().complete);
      }
      outstanding_sequential_reads.push_back(std::make_pair(addr, complete));
    } else {
      if(!is_dup_read) {
        _dram->read((addr & ~_access_mask), complete, false);
      } else {
        outstanding_sequential_reads.push_back(std::make_pair(addr, complete));
      }
    }
  }
}

void SimObj::Cache::alloc(uint64_t addr, bool* complete) {
  stats[ALLOCATE]++;
  uint64_t line = hit(addr);
  if(line < cache.size()) {
    cache[line].access(false);
    *complete = true;
  } else {
    line = getLRU(addr);
    if(cache[line].valid() && cache[line].dirty()) {
      stats[WRITEBACK]++;
      mshr_t req;
      req.address = cache[line].getAddr();
      req.write = true;
      req.complete = false;
      req.prefetch = false;
      mshr.push_front(req);
      _dram->write(mshr.front().address, &mshr.front().complete, false);
    }
    cache[line].insert(addr);
    cache[line].access(false);
    *complete = true;
  }
}

void SimObj::Cache::prefetch(uint64_t addr, bool* complete) {
  stats[PREFETCH]++;
  uint64_t line = hit(addr);
  if(line < cache.size()) {
    cache[line].access(false);
    *complete = true;
  }
  else {
    bool is_dup_read = false;
    for (auto it : mshr) {
      if(it.address == (addr & ~_access_mask)) {
        is_dup_read = true;
        break;
      }
    }
    if(!is_dup_read) {
      mshr_t req;
      req.address = (addr & ~_access_mask);
      req.write = false;
      req.complete = false;
      req.prefetch = true;
      mshr.push_front(req);
      _dram->read(mshr.front().address, &mshr.front().complete);
    }
    outstanding_sequential_reads.push_back(std::make_pair(addr, complete));
  }
}

void SimObj::Cache::print_stats() {
  sim_out.write("-------------------------------------------------------------------------------\n");
  sim_out.write("[ " + _name + " ]\n");
  sim_out.write("  HIT:              " + std::to_string(stats[HIT]) + "\n");
  sim_out.write("  MISS:             " + std::to_string(stats[MISS]) + "\n");
  sim_out.write("  SEQ READ:         " + std::to_string(stats[SEQ_READ]) + "\n");
  sim_out.write("  SEQ WRITE:        " + std::to_string(stats[SEQ_WRITE]) + "\n");
  sim_out.write("  RAND READ:        " + std::to_string(stats[RAND_READ]) + "\n");
  sim_out.write("  RAND WRITE:       " + std::to_string(stats[RAND_WRITE]) + "\n");
  sim_out.write("  ALLOCATE:         " + std::to_string(stats[ALLOCATE]) + "\n");
  sim_out.write("  PREFETCH:         " + std::to_string(stats[PREFETCH]) + "\n");
  sim_out.write("  WRITEBACK:        " + std::to_string(stats[WRITEBACK]) + "\n");
  //sim_out.write("  MAX CONFLICT:     " + std::to_string(conflict_insert_queue_max_len) + "\n");
}

void SimObj::Cache::print_stats_csv() {
  sim_out.write(_name + ",");
  for(auto stat : stats) {
    sim_out.write(std::to_string(stat)+",");
  }
  //sim_out.write(std::to_string(conflict_insert_queue_max_len));
  sim_out.write("\n");
}

void SimObj::Cache::reset() {
  // Reset the stat counters:
  for(auto & stat : stats) stat = 0;
  //conflict_insert_queue_max_len = 0;
}
