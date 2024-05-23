#include "dmc.h"

#include <vector>

using namespace std;

DMC::DMC(MetaManager* meta_man, QPManager* qp_man, t_id_t tid, coro_id_t coroid,
         CoroutineScheduler* sched, RDMABufferAllocator* rdma_buffer_allocator,
         AddrCache* addr_buf, DMC_TYPE type) {
  t_id = tid;
  coro_id = coroid;
  coro_sched = sched;
  global_meta_man = meta_man;
  thread_qp_man = qp_man;
  thread_rdma_buffer_alloc = rdma_buffer_allocator;

  addr_cache = addr_buf;
  dmc_type = type;
}

bool DMC::open(string path, coro_yield_t& yield) {
  // You can read from primary or backup
  std::vector<DirectRead> pending_direct_ro;
  std::vector<HashRead> pending_hash_ro;
  // vector<NameID> paths = path_resolution(path);
  if (dmc_type == DMC_TYPE::native) {
    // get all the inode
  } else if (dmc_type == DMC_TYPE::disaggregated) {
    // check cache
    // for () {
    // }
    // get the inode that is not in the cache
  }
  if (!IssueReader(pending_direct_ro, pending_hash_ro)) return false;
  // Yield to other coroutines when waiting for network replies
  coro_sched->Yield(yield, coro_id);

  // Receive data
  std::list<HashRead> pending_next_hash_ro;
  auto res = CheckReadRO(pending_direct_ro, pending_hash_ro,
                         pending_next_hash_ro, yield);
  //   check results

  return true;
}
bool DMC::close(string path, coro_yield_t& yield) { return true; }

bool read(string path, coro_yield_t& yield) { return true; }
bool write(string path, coro_yield_t& yield) { return true; }
bool DMC::create(string path, coro_yield_t& yield) {
  if (dmc_type == DMC_TYPE::native) {
    // get all the inode
    // check permission
    // lock the parent and the entry list, insert new inode
    // update parent inode and entry list
  } else if (dmc_type == DMC_TYPE::disaggregated) {
    // check cache
    // get the inode
    // check permission

    // FAA the entry list, insert new inode

    //
  }
  return true;
}
bool DMC::_delete(string path, coro_yield_t& yield) { return true; }
bool DMC::rename(string old_path, string new_path, coro_yield_t& yield) {
  return true;
}
bool DMC::set_permission(string path, uint64_t permission,
                         coro_yield_t& yield) {
  // set permission , update mtime

  return true;
}
bool DMC::read_dir(string path, coro_yield_t& yield) { return true; }
bool DMC::stat_dir(string path, coro_yield_t& yield) { return true; }
bool DMC::mkdir(string path, coro_yield_t& yield) { return true; }
bool DMC::rmdir(string path, coro_yield_t& yield) { return true; }
