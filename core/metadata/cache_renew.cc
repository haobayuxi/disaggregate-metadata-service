
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
#include "inode.h"
#include "memstore/hash_store.h"
#include "util/debug.h"
#include "util/hash.h"
#include "util/json_config.h"

class CacheRenewer {
 public:
  void renew();
};

void CacheRenewer::renew() {
  //
  sleep(1);
  // renew using rdma read
}