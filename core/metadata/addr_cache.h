// Author: Ming Zhang
// Copyright (c) 2022

#pragma once

#include <map>
#include <unordered_map>

#include "base/common.h"
#include "cuckoohash_map.hh"
#include "inode.h"

const offset_t NOT_FOUND = -1;

struct InodeCacheItem {
  uint64_t addr;

  struct Inode inode;
};

// For fast remote address lookup
class AddrCache {
 public:
  void Insert(node_id_t remote_node_id, itemkey_t key, InodeCacheItem inode) {
    // The node and table both exist, then insert/update the <key,offset> pair
    addr_map[remote_node_id].insert(key, inode);
  }

  void Remove(node_id_t remote_node_id, itemkey_t key) {
    // The node and table both exist, then insert/update the <key,offset> pair
    addr_map[remote_node_id].erase(key);
  }

  void resize() {
    // addr_map[1][1].rehash(100000);
  }

  // We know which node to read, but we do not konw whether it is cached before
  ALWAYS_INLINE
  bool Search(node_id_t remote_node_id, itemkey_t key, InodeCacheItem &inode) {
    if (addr_map[remote_node_id].find(key, inode)) {
      return true;
    } else {
      return false;
    }
  }

  // size_t TotalAddrSize() {
  //   size_t total_size = 0;
  //   for (auto it = addr_map.begin(); it != addr_map.end(); it++) {
  //     total_size += sizeof(node_id_t);
  //     for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
  //       total_size += sizeof(table_id_t);
  //       for (auto it3 = it2->second.begin(); it3 != it2->second.end(); it3++)
  //       {
  //         total_size += (sizeof(itemkey_t) + sizeof(offset_t));
  //       }
  //     }
  //   }

  //   return total_size;
  // }

 private:
  // std::unordered_map<
  //     node_id_t,
  //     std::unordered_map<table_id_t, std::unordered_map<itemkey_t,
  //     offset_t>>> addr_map;

  // std::unordered_map<itemkey_t, offset_t> addr_map[1][20];
  libcuckoo::cuckoohash_map<uint64_t, InodeCacheItem> addr_map[4];
};