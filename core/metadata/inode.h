#include <stdint.h>

#include <vector>

using namespace std;

#include "base/common.h"
#include "util/crc.h"

struct Inode {
  uint64_t permission;
  uint16_t atime;
  uint16_t ctime;
  uint16_t mtime;
};
const size_t inode_size = sizeof(Inode);
uint64_t get_time_offset(uint64_t addr) { return addr + sizeof(uint64_t); }

uint64_t get_permission_offset(uint64_t addr) { return addr; }

uint64_t compute_hash(string const& s) {
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

vector<uint64_t> path_resolution(string path) {
  vector<uint64_t> path_ids;

  return path_ids;
}

struct DataSetItem {
  DataItemPtr item_ptr;
  bool is_fetched;
  uint64_t addr;
};

struct OldVersionForInsert {
  table_id_t table_id;
  itemkey_t key;
  version_t version;
};

struct LockAddr {
  node_id_t node_id;
  uint64_t lock_addr;
};

// For coroutines
struct DirectRead {
  RCQP* qp;
  DataSetItem* item;
  char* buf;
  node_id_t remote_node;
};

struct HashRead {
  RCQP* qp;
  DataSetItem* item;
  char* buf;
  node_id_t remote_node;
  const HashMeta meta;
};

struct CasRead {
  RCQP* qp;
  DataSetItem* item;
  char* cas_buf;
  char* data_buf;
  node_id_t primary_node_id;
};

struct InsertOffRead {
  RCQP* qp;
  DataSetItem* item;
  char* buf;
  node_id_t remote_node;
  const HashMeta meta;
  offset_t node_off;
};

struct NameID {
  uint64_t id;
  string name;
};

vector<NameID> path_resolution(string path) {
  vector<NameID> a;
  return a;
}
