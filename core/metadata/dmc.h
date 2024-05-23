
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

#include "allocator/buffer_allocator.h"
#include "allocator/log_allocator.h"
#include "base/common.h"
#include "cache/addr_cache.h"
#include "cache/lock_status.h"
#include "cache/version_status.h"
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

uint32_t get_inode_crc(struct Inode* inode) {
  return CRC::Calculate(inode, inode_size - 16, CRC::CRC_32());
}

class DMC {
 public:
  DMC(MetaManager* meta_man, QPManager* qp_man, VersionCache* status,
      t_id_t tid, coro_id_t coroid, CoroutineScheduler* sched,
      RDMABufferAllocator* rdma_buffer_allocator, AddrCache* addr_buf);
  MetaManager* global_meta_man;  // Global metadata manager
                                 // native
  bool open(string path, coro_yield_t& yield);
  bool close(string path, coro_yield_t& yield);
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
  bool IssueUpdaterAndWriter(std::vector<CasRead>& pending_cas_rw);
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
  void AddToReadOnlySet(DataItemPtr item);

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
  vector<DataSetItem> reader_set;
  vector<DataSetItem> writer_set;
  vector<DataSetItem> updater_set;
};

ALWAYS_INLINE
void DMC::Begin() {}

ALWAYS_INLINE
void DMC::AddToReadSet(DataItemPtr item) {
  DataSetItem data_set_item {
    .item_ptr = std::move(item), .is_fetched = false, .addr = 0;
    read_set.emplace_back(data_set_item);
  }
}