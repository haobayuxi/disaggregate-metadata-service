// Author: Ming Zhang
// Copyright (c) 2022

#pragma once

#include "allocator/region_allocator.h"
#include "base/common.h"
#include "connection/meta_manager.h"
#include "metadata/dmc.h"
#include "micro/micro_db.h"
#include "micro/micro_meta.h"

struct thread_params {
  t_id_t thread_local_id;
  t_id_t thread_global_id;
  t_id_t thread_num_per_machine;
  t_id_t total_thread_num;
  MetaManager* global_meta_man;
  RDMARegionAllocator* global_rdma_region;
  int coro_num;
  std::string bench_name;
};

void run_thread(thread_params* params);