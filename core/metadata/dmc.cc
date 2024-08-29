#include "dmc.h"

#include <vector>

using namespace std;

DMC::DMC(MetaManager* meta_man, QPManager* qp_man, t_id_t tid, coro_id_t coroid,
         CoroutineScheduler* sched, RDMABufferAllocator* rdma_buffer_allocator,
         AddrCache* addr_buf, DMC_TYPE type, int servers_) {
  t_id = tid;
  coro_id = coroid;
  coro_sched = sched;
  global_meta_man = meta_man;
  thread_qp_man = qp_man;
  thread_rdma_buffer_alloc = rdma_buffer_allocator;

  addr_cache = addr_buf;
  dmc_type = type;
  servers = servers_;
}

bool DMC::open(string path, coro_yield_t& yield) { return true; }
bool DMC::close(string path, coro_yield_t& yield) { return true; }

bool DMC::stat_file(string path, coro_yield_t& yield) {
  std::vector<DirectRead> pending_direct_ro;
  std::vector<HashRead> pending_hash_ro;
  vector<NameID> paths = path_resolution(path);
  if (dmc_type == DMC_TYPE::native) {
    // get all the inode
  } else if (dmc_type == DMC_TYPE::disaggregated) {
    // check cache
    for (NameID id : paths) {
      InodeCacheItem cache_inode;
      InodeCacheItem& cache_inodes = cache_inode;
      auto node_id = id.key % servers;
      if (addr_cache->Search(node_id, id.key, cache_inodes)) {
        if (cache_inode.inode.is_dir) {
          // is dir check permission

        } else {
          // not dir, insert into readerset
          DataItemPtr read = std::make_shared<DataItem>(0, id.key);
          AddToReaderSet(read);
        }
      } else {
        // insert into readset
        DataItemPtr read = std::make_shared<DataItem>(0, id.key);
        AddToReaderSet(read);
      }
    }
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

bool DMC::read(string path, coro_yield_t& yield) { 
  // modify the access time
if (dmc_type == DMC_TYPE::native) {
    // lock inode

    // modify the access time

    // apply update and release the lock 

  } else if (dmc_type == DMC_TYPE::disaggregated) {
    // check cache

    }


  return true; }
bool DMC::write(string path, coro_yield_t& yield) { 
  // modify the mtime
  // lock 
  return true; }
bool DMC::create(string path, coro_yield_t& yield) {
  
  
  return true;
}
bool DMC::_delete(string path, coro_yield_t& yield) { 
  // set the bool flag

  return true; }
bool DMC::rename(string old_path, string new_path, coro_yield_t& yield) {
  // get the rename lock 



  // lock all the tuple
  return true;
}
bool DMC::set_permission(string path, uint64_t permission,
                         coro_yield_t& yield) {
  // set permission , update mtime

  return true;
}
bool DMC::read_dir(string path, coro_yield_t& yield) { return true; }
// bool DMC::stat_dir(string path, coro_yield_t& yield) { return true; }
bool DMC::mkdir(string path, coro_yield_t& yield) { return true; }
bool DMC::rmdir(string path, coro_yield_t& yield) { 
  // delete all the tuples
  return true; }

vector<NameID> path_resolution(string path) {
  vector<NameID> path_ids;

  int pos = 0;
  while (true) {
    pos = path.find('/', 1);
    if (pos > 0) {
      //
      NameID nid;
      nid.name = path.substr(0,pos);
      nid.key = compute_hash(nid.name);
      path_ids.push_back(nid);

    } else {
      break;
    }
  }

  return path_ids;
}