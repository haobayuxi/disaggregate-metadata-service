// Author: Ming Zhang
// Copyright (c) 2022

#include "micro/micro_txn.h"

#include <set>

/******************** The business logic (Transaction) start
 * ********************/

bool TxTestCachedAddr(ZipfGen* zipf_gen, uint64_t* seed, coro_yield_t& yield,
                      tx_id_t tx_id, DTX* dtx, bool is_skewed,
                      uint64_t data_set_size, uint64_t num_keys_global,
                      uint64_t write_ratio) {
  dtx->TxBegin(tx_id);
  bool is_write[data_set_size];

  for (uint64_t i = 0; i < data_set_size; i++) {
    micro_key_t micro_key;
    micro_key.micro_id =
        688;  // First read is non-cacheable, set ATTEMPED_NUM to 5

    assert(micro_key.item_key >= 0 && micro_key.item_key < num_keys_global);

    micro_objs[i].data_item_ptr = std::make_shared<DataItem>(
        (table_id_t)MicroTableType::kMicroTable, micro_key.item_key);
    micro_objs[i].is_dup = false;

    dtx->AddToReadWriteSet(micro_objs[i].data_item_ptr);
    is_write[i] = true;
  }

  if (!dtx->TxExe(yield)) {
    // TLOG(DBG, thread_gid) << "tx " << tx_id << " aborts after exe";
    return false;
  }

  for (uint64_t i = 0; i < data_set_size; i++) {
    micro_val_t* micro_val = (micro_val_t*)micro_objs[i].data_item_ptr->value;
    if (micro_val->magic[0] != micro_magic) {
      RDMA_LOG(FATAL) << "[FATAL] Read unmatch, tid-cid-txid: " << dtx->t_id
                      << "-" << dtx->coro_id << "-" << tx_id;
    }
    if (is_write[i]) {
      micro_val->magic[1] = micro_magic * 2;
    }
  }

  bool commit_status = dtx->TxCommit(yield);
  // TLOG(DBG, thread_gid) << "tx " << tx_id << " commit? " << commit_status;
  return commit_status;
}

bool TxLockContention(ZipfGen* zipf_gen, uint64_t* seed, coro_yield_t& yield,
                      tx_id_t tx_id, DTX* dtx, bool is_skewed,
                      uint64_t data_set_size, uint64_t num_keys_global,
                      uint64_t write_ratio) {
  dtx->TxBegin(tx_id);
  bool is_write[data_set_size];
  DataItemPtr micro_objs[data_set_size];

  for (uint64_t i = 0; i < data_set_size; i++) {
    micro_key_t micro_key;
    if (is_skewed) {
      // Skewed distribution
      micro_key.micro_id = (itemkey_t)(zipf_gen->next());
    } else {
      // Uniformed distribution
      micro_key.micro_id = (itemkey_t)FastRand(seed) & (num_keys_global - 1);
    }

    assert(micro_key.item_key >= 0 && micro_key.item_key < num_keys_global);

    micro_objs[i] = std::make_shared<DataItem>(
        (table_id_t)MicroTableType::kMicroTable, micro_key.item_key);

    if (FastRand(seed) % 100 < write_ratio) {
      dtx->AddToReadWriteSet(micro_objs[i]);
      is_write[i] = true;
    } else {
      dtx->AddToReadOnlySet(micro_objs[i]);
      is_write[i] = false;
    }
  }

  if (!dtx->TxExe(yield)) {
    // TLOG(DBG, thread_gid) << "tx " << tx_id << " aborts after exe";
    return false;
  }

  for (uint64_t i = 0; i < data_set_size; i++) {
    micro_val_t* micro_val = (micro_val_t*)micro_objs[i]->value;
    if (micro_val->magic[0] != micro_magic) {
      RDMA_LOG(FATAL) << "[FATAL] Read unmatch, tid-cid-txid: " << dtx->t_id
                      << "-" << dtx->coro_id << "-" << tx_id;
    }
    if (is_write[i]) {
      micro_val->magic[1] = micro_magic * 2;
    }
  }

  bool commit_status = dtx->TxCommit(yield);
  // TLOG(DBG, thread_gid) << "tx " << tx_id << " commit? " << commit_status;
  return commit_status;
}

bool TxReadOnly(ZipfGen* zipf_gen, uint64_t* seed, coro_yield_t& yield,
                tx_id_t tx_id, DTX* dtx, bool is_skewed, uint64_t data_set_size,
                uint64_t num_keys_global, uint64_t write_ratio) {
  // This is used to evaluate the performance of reading backup vs. not reading
  // backup when the write ratio is 0 Remember to set remote_node_id = t_id %
  // (BACKUP_DEGREE + 1); in dtx_issue.cc to enable read from primary and backup
  dtx->TxBegin(tx_id);

  micro_key_t micro_key;
  if (is_skewed) {
    // Skewed distribution
    micro_key.micro_id = (itemkey_t)(zipf_gen->next());
  } else {
    // Uniformed distribution
    micro_key.micro_id = (itemkey_t)FastRand(seed) & (num_keys_global - 1);
  }
  assert(micro_key.item_key >= 0 && micro_key.item_key < num_keys_global);

  DataItemPtr micro_obj = std::make_shared<DataItem>(
      (table_id_t)MicroTableType::kMicroTable, micro_key.item_key);

  dtx->AddToReadOnlySet(micro_obj);

  if (!dtx->TxExe(yield)) {
    // TLOG(DBG, thread_gid) << "tx " << tx_id << " aborts after exe";
    return false;
  }

  bool commit_status = dtx->TxCommit(yield);
  // TLOG(DBG, thread_gid) << "tx " << tx_id << " commit? " << commit_status;
  return commit_status;
}

bool Trace0( uint64_t* seed, coro_yield_t& yield,
                      tx_id_t tx_id, DTX* dtx,) {
  int op_ratio = FastRand(seed) % 100;
   if (op_ratio < write_ratio){

   }else if (
    
   )
  return true;
}

bool Trace1() { return true; }

bool Trace2() { return true; }