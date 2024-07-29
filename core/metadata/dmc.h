#ifndef DMC_H
#define DMC_H
#pragma once

#include <time.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <list>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include "addr_cache.h"
#include "allocator/buffer_allocator.h"
#include "allocator/log_allocator.h"
#include "base/common.h"
#include "connection/meta_manager.h"
#include "connection/qp_manager.h"
#include "doorbell.h"
#include "inode.h"
#include "memstore/hash_store.h"
#include "util/debug.h"
#include "util/hash.h"
#include "util/json_config.h"

enum DMC_TYPE : int {
  native = 0,
  disaggregated = 1,
};

ALWAYS_INLINE
uint32_t get_inode_crc(struct Inode* inode) {
  return CRC::Calculate(inode, inode_size - 16, CRC::CRC_32());
}

class DMC {
 public:
  DMC(MetaManager* meta_man, QPManager* qp_man, t_id_t tid, coro_id_t coroid,
      CoroutineScheduler* sched, RDMABufferAllocator* rdma_buffer_allocator,
      AddrCache* addr_buf, DMC_TYPE type, int servers_);
  MetaManager* global_meta_man;  // Global metadata manager
                                 // native
  bool open(string path, coro_yield_t& yield);
  bool close(string path, coro_yield_t& yield);
  bool stat_file(string path, coro_yield_t& yield);
  bool read(string path, coro_yield_t& yield);
  bool write(string path, coro_yield_t& yield);
  bool create(string path, coro_yield_t& yield);
  bool _delete(string path, coro_yield_t& yield);
  bool rename(string old_path, string new_path, coro_yield_t& yield);
  bool set_permission(string path, uint64_t permission, coro_yield_t& yield);
  bool read_dir(string path, coro_yield_t& yield);
  bool stat_dir(string path, coro_yield_t& yield);
  bool mkdir(string path, coro_yield_t& yield);
  bool rmdir(string path, coro_yield_t& yield);

 public:
  //  issue
  bool IssueReader(std::vector<DirectRead>& pending_direct_ro,
                   std::vector<HashRead>& pending_hash_ro);
  bool IssueUpdater(std::vector<CasRead>& pending_cas_rw);
  bool IssueWriter(std::vector<CasRead>& pending_cas_rw);
  //  check
  bool CheckReadRO(std::vector<DirectRead>& pending_direct_ro,
                   std::vector<HashRead>& pending_hash_ro,
                   std::list<HashRead>& pending_next_hash_ro,
                   coro_yield_t& yield);
  // check ro
  bool CheckDirectRO(std::vector<DirectRead>& pending_direct_ro,
                     std::list<HashRead>& pending_next_hash_ro);
  bool CheckHashRO(std::vector<HashRead>& pending_hash_ro,
                   std::list<HashRead>& pending_next_hash_ro);
  bool CheckNextHashRO(std::list<HashRead>& pending_next_hash_ro);

  void Begin();
  void AddToReaderSet(DataItemPtr item);
  void AddToUpdaterSet(DataItemPtr item);
  void AddToWriterSet(DataItemPtr item);
  void AddToInsertSet(DataItemPtr item);
  int64_t compute_hash(string const& s);

  vector<DataSetItem> reader_set;
  vector<DataSetItem> writer_set;
  vector<DataSetItem> updater_set;
  vector<DataSetItem> insert_set;

  int servers;

 private:
  tx_id_t tx_id;  // Transaction ID

  t_id_t t_id;                     // Thread ID
  coro_id_t coro_id;               // Coroutine ID
  CoroutineScheduler* coro_sched;  // Thread local coroutine scheduler

  QPManager* thread_qp_man;  // Thread local qp connection manager. Each
                             // transaction thread has one

  RDMABufferAllocator*
      thread_rdma_buffer_alloc;  // Thread local RDMA buffer allocator

  AddrCache* addr_cache;
  DMC_TYPE dmc_type;
};

ALWAYS_INLINE
void DMC::Begin() {
  reader_set.clear();
  updater_set.clear();
  writer_set.clear();
  insert_set.clear();
}

ALWAYS_INLINE
void DMC::AddToReaderSet(DataItemPtr item) {
  DataSetItem data_set_item{
      .item_ptr = std::move(item),
      .is_fetched = false,
      .read_which_node = -1,
  };
  reader_set.emplace_back(data_set_item);
}

ALWAYS_INLINE
void DMC::AddToUpdaterSet(DataItemPtr item) {
  DataSetItem data_set_item{
      .item_ptr = std::move(item),
      .is_fetched = false,
      .read_which_node = -1,
  };
  updater_set.emplace_back(data_set_item);
}

ALWAYS_INLINE
void DMC::AddToWriterSet(DataItemPtr item) {
  DataSetItem data_set_item{
      .item_ptr = std::move(item),
      .is_fetched = false,
      .read_which_node = -1,
  };
  writer_set.emplace_back(data_set_item);
}

ALWAYS_INLINE
void DMC::AddToInsertSet(DataItemPtr item) {
  DataSetItem data_set_item{
      .item_ptr = std::move(item),
      .is_fetched = false,
      .read_which_node = -1,
  };
  insert_set.emplace_back(data_set_item);
}

// uint64_t get_time_offset(uint64_t addr) { return addr + sizeof(uint64_t); }

// uint64_t get_permission_offset(uint64_t addr) { return addr; }
ALWAYS_INLINE
int64_t DMC::compute_hash(string const& s) {
  const int p = 31;
  const int m = 1e9 + 9;
  uint64_t hash_value = 0;
  uint64_t p_pow = 1;
  for (char c : s) {
    hash_value = (hash_value + (c - 'a' + 1) * p_pow) % m;
    p_pow = (p_pow * p) % m;
  }
  return hash_value;
}

#endif