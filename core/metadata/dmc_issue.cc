#include "dmc.h"

bool DMC::IssueReadRO(std::vector<DirectRead>& pending_direct_ro,
                      std::vector<HashRead>& pending_hash_ro,
                      coro_yield_t& yield) {
  for (auto& item : read_set) {
    if (item.is_fetched) continue;
    auto it = item.item_ptr;

    node_id_t remote_node_id = global_meta_man->GetPrimaryNodeID(it->table_id);

    item.read_which_node = remote_node_id;
    RCQP* qp = thread_qp_man->GetRemoteDataQPWithNodeID(remote_node_id);
    auto offset = addr_cache->Search(remote_node_id, it->table_id, it->key);
    if (offset != NOT_FOUND) {
      //   check the permission

    } else {
      // Local cache does not have
      // miss_local_cache_times++;

      HashMeta meta =
          global_meta_man->GetPrimaryHashMetaWithTableID(it->table_id);

      uint64_t idx = MurmurHash64A(it->key, 0xdeadbeef) % meta.bucket_num;
      offset_t node_off = idx * meta.node_size + meta.base_off;
      char* local_hash_node = thread_rdma_buffer_alloc->Alloc(sizeof(HashNode));
      pending_hash_ro.emplace_back(HashRead{.qp = qp,
                                            .item = &item,
                                            .buf = local_hash_node,
                                            .remote_node = remote_node_id,
                                            .meta = meta});
      if (!coro_sched->RDMARead(coro_id, qp, local_hash_node, node_off,
                                sizeof(HashNode))) {
        return false;
      }
    }
  }
  return true;
}

// bool DTX::IssueReadLock(std::vector<CasRead>& pending_cas_rw,
//                         std::vector<HashRead>& pending_hash_rw,
//                         std::vector<InsertOffRead>& pending_insert_off_rw) {
//   // For read-write set, we need to read and lock them
//   for (size_t i = 0; i < read_write_set.size(); i++) {
//     if (read_write_set[i].is_fetched) continue;
//     auto it = read_write_set[i].item_ptr;
//     auto remote_node_id = global_meta_man->GetPrimaryNodeID(it->table_id);
//     read_write_set[i].read_which_node = remote_node_id;
//     RCQP* qp = thread_qp_man->GetRemoteDataQPWithNodeID(remote_node_id);
//     auto offset = addr_cache->Search(remote_node_id, it->table_id, it->key);
//     // Addr cached in local
//     if (offset != NOT_FOUND) {
//       // hit_local_cache_times++;
//       it->remote_offset = offset;
//       locked_rw_set.emplace_back(i);
//       // After getting address, use doorbell CAS + READ
//       char* cas_buf = thread_rdma_buffer_alloc->Alloc(sizeof(lock_t));
//       char* data_buf = thread_rdma_buffer_alloc->Alloc(DataItemSize);
//       pending_cas_rw.emplace_back(CasRead{.qp = qp, .item =
//       &read_write_set[i], .cas_buf = cas_buf, .data_buf = data_buf,
//       .primary_node_id = remote_node_id}); std::shared_ptr<LockReadBatch>
//       doorbell = std::make_shared<LockReadBatch>();
//       doorbell->SetLockReq(cas_buf, it->GetRemoteLockAddr(offset),
//       STATE_CLEAN, STATE_LOCKED); doorbell->SetReadReq(data_buf, offset,
//       DataItemSize);  // Read a DataItem if (!doorbell->SendReqs(coro_sched,
//       qp, coro_id)) {
//         return false;
//       }
//     } else {
//       // Only read
//       // miss_local_cache_times++;
//       not_eager_locked_rw_set.emplace_back(i);
//       const HashMeta& meta =
//       global_meta_man->GetPrimaryHashMetaWithTableID(it->table_id); uint64_t
//       idx = MurmurHash64A(it->key, 0xdeadbeef) % meta.bucket_num; offset_t
//       node_off = idx * meta.node_size + meta.base_off; char* local_hash_node
//       = thread_rdma_buffer_alloc->Alloc(sizeof(HashNode)); if
//       (it->user_insert) {
//         pending_insert_off_rw.emplace_back(InsertOffRead{.qp = qp, .item =
//         &read_write_set[i], .buf = local_hash_node, .remote_node =
//         remote_node_id, .meta = meta, .node_off = node_off});
//       } else {
//         pending_hash_rw.emplace_back(HashRead{.qp = qp, .item =
//         &read_write_set[i], .buf = local_hash_node, .remote_node =
//         remote_node_id, .meta = meta});
//       }
//       if (!coro_sched->RDMARead(coro_id, qp, local_hash_node, node_off,
//       sizeof(HashNode))) {
//         return false;
//       }
//     }
//   }
//   return true;
// }