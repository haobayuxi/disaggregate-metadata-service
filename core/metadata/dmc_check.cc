
#include "dmc.h"




bool DMC::CheckReadRO(std::vector<DirectRead>& pending_direct_ro,
                      std::vector<HashRead>& pending_hash_ro,
                      std::list<HashRead>& pending_next_hash_ro,
                      coro_yield_t& yield) {
  //
  for (auto& res : pending_direct_ro) {
    auto* it = res.item->item_ptr.get();
    auto* fetched_item = (DataItem*)res.buf;
    if (likely(fetched_item->key == it->key &&
               fetched_item->table_id == it->table_id)) {
      if (likely(fetched_item->valid)) {
        *it = *fetched_item;
        res.item->is_fetched = true;

      } else {
        // The item is deleted before, then update the local cache
        addr_cache->Insert(res.remote_node, it->table_id, it->key, NOT_FOUND);
        return false;
      }
    } else {
      // The cached address is stale. E.g., insert a new item after being
      // deleted Local cache does not have. We have to re-read via hash

      node_id_t remote_node_id =
          global_meta_man->GetPrimaryNodeID(it->table_id);
      const HashMeta& meta =
          global_meta_man->GetPrimaryHashMetaWithTableID(it->table_id);
      uint64_t idx = MurmurHash64A(it->key, 0xdeadbeef) % meta.bucket_num;
      offset_t node_off = idx * meta.node_size + meta.base_off;
      auto* local_hash_node =
          (HashNode*)thread_rdma_buffer_alloc->Alloc(sizeof(HashNode));
      pending_next_hash_ro.emplace_back(HashRead{.qp = res.qp,
                                                 .item = res.item,
                                                 .buf = (char*)local_hash_node,
                                                 .remote_node = remote_node_id,
                                                 .meta = meta});
      if (!coro_sched->RDMARead(coro_id, res.qp, (char*)local_hash_node,
                                node_off, sizeof(HashNode)))
        return false;
    }
  }
  return true;
}

bool DMC::CheckHashRO(std::vector<HashRead>& pending_hash_ro,
                      std::list<HashRead>& pending_next_hash_ro) {
  // Check results from hash read
  for (auto& res : pending_hash_ro) {
    auto* local_hash_node = (HashNode*)res.buf;
    auto* it = res.item->item_ptr.get();
    bool find = false;

    for (auto& item : local_hash_node->data_items) {
      if (item.valid && item.key == it->key && item.table_id == it->table_id) {
        *it = item;
        addr_cache->Insert(res.remote_node, it->table_id, it->key,
                           it->remote_offset);
        res.item->is_fetched = true;
        find = true;
        break;
      }
    }

    if (unlikely(!find)) {
      if (local_hash_node->next == nullptr) return false;
      // Not found, we need to re-read the next bucket
      auto node_off = (uint64_t)local_hash_node->next - res.meta.data_ptr +
                      res.meta.base_off;
      pending_next_hash_ro.emplace_back(HashRead{.qp = res.qp,
                                                 .item = res.item,
                                                 .buf = res.buf,
                                                 .remote_node = res.remote_node,
                                                 .meta = res.meta});
      if (!coro_sched->RDMARead(coro_id, res.qp, res.buf, node_off,
                                sizeof(HashNode)))
        return false;
    }
  }
  return true;
}