// Author: Ming Zhang
// Copyright (c) 2022

#include "worker/worker.h"

#include <atomic>
#include <cstdio>
#include <fstream>
#include <functional>
#include <memory>

#include "allocator/buffer_allocator.h"
#include "allocator/log_allocator.h"
#include "connection/qp_manager.h"
#include "util/latency.h"
#include "util/zipf.h"

using namespace std::placeholders;

// All the functions are executed in each thread
std::mutex mux;

extern std::atomic<uint64_t> tx_id_generator;
extern std::atomic<uint64_t> connected_t_num;
extern std::vector<double> lock_durations;
extern std::vector<t_id_t> tid_vec;
extern std::vector<double> attemp_tp_vec;
extern std::vector<double> tp_vec;
extern std::vector<double> medianlat_vec;
extern std::vector<double> taillat_vec;

extern std::vector<uint64_t> total_try_times;
extern std::vector<uint64_t> total_commit_times;

__thread size_t ATTEMPTED_NUM;
__thread uint64_t seed;                        // Thread-global random seed
__thread FastRandom* random_generator = NULL;  // Per coroutine random generator
__thread t_id_t thread_gid;
__thread t_id_t thread_local_id;
__thread t_id_t thread_num;

__thread MetaManager* meta_man;
__thread QPManager* qp_man;

__thread RDMABufferAllocator* rdma_buffer_allocator;
__thread LogOffsetAllocator* log_offset_allocator;
__thread AddrCache* addr_cache;

__thread coro_id_t coro_num;
__thread CoroutineScheduler*
    coro_sched;  // Each transaction thread has a coroutine scheduler
__thread bool stop_run;

// Performance measurement (thread granularity)
__thread struct timespec msr_start, msr_end;
__thread double* timer;
__thread uint64_t stat_attempted_tx_total = 0;  // Issued transaction number
__thread uint64_t stat_committed_tx_total = 0;  // Committed transaction number
const coro_id_t POLL_ROUTINE_ID = 0;            // The poll coroutine ID

// For MICRO benchmark
__thread ZipfGen* zipf_gen = nullptr;
__thread bool is_skewed;
__thread uint64_t data_set_size;
__thread uint64_t num_keys_global;
__thread uint64_t write_ratio;

// Stat the commit rate
__thread uint64_t* thread_local_try_times;
__thread uint64_t* thread_local_commit_times;

// Coroutine 0 in each thread does polling
void PollCompletion(coro_yield_t& yield) {
  while (true) {
    coro_sched->PollCompletion();
    Coroutine* next = coro_sched->coro_head->next_coro;
    if (next->coro_id != POLL_ROUTINE_ID) {
      // RDMA_LOG(DBG) << "Coro 0 yields to coro " << next->coro_id;
      coro_sched->RunCoroutine(yield, next);
    }
    if (stop_run) break;
  }
}

void RecordTpLat(double msr_sec) {
  double attemp_tput = (double)stat_attempted_tx_total / msr_sec;
  double tx_tput = (double)stat_committed_tx_total / msr_sec;

  std::sort(timer, timer + stat_committed_tx_total);
  double percentile_50 = timer[stat_committed_tx_total / 2];
  double percentile_99 = timer[stat_committed_tx_total * 99 / 100];

  mux.lock();
  tid_vec.push_back(thread_gid);
  attemp_tp_vec.push_back(attemp_tput);
  tp_vec.push_back(tx_tput);
  medianlat_vec.push_back(percentile_50);
  taillat_vec.push_back(percentile_99);

  for (size_t i = 0; i < total_try_times.size(); i++) {
    total_try_times[i] += thread_local_try_times[i];
    total_commit_times[i] += thread_local_commit_times[i];
  }

  mux.unlock();
}

void RunMICRO(coro_yield_t& yield, coro_id_t coro_id) {
  // double total_msr_us = 0;
  // // Each coroutine has a dtx: Each coroutine is a coordinator
  // DTX* dtx = new DTX(meta_man, qp_man, status, lock_table, thread_gid,
  // coro_id,
  //                    coro_sched, rdma_buffer_allocator, log_offset_allocator,
  //                    addr_cache);
  // struct timespec tx_start_time, tx_end_time;
  // bool tx_committed = false;

  // // Running transactions
  // clock_gettime(CLOCK_REALTIME, &msr_start);
  // while (true) {
  //   uint64_t iter = ++tx_id_generator;  // Global atomic transaction id
  //   stat_attempted_tx_total++;
  //   clock_gettime(CLOCK_REALTIME, &tx_start_time);
  //   // tx_committed =
  //   //     TxLockContention(zipf_gen, &seed, yield, iter, dtx, is_skewed,
  //   //                      data_set_size, num_keys_global, write_ratio);
  //   /********************************** Stat begin
  //    * *****************************************/
  //   // Stat after one transaction finishes
  //   if (tx_committed) {
  //     clock_gettime(CLOCK_REALTIME, &tx_end_time);
  //     double tx_usec =
  //         (tx_end_time.tv_sec - tx_start_time.tv_sec) * 1000000 +
  //         (double)(tx_end_time.tv_nsec - tx_start_time.tv_nsec) / 1000;
  //     timer[stat_committed_tx_total++] = tx_usec;
  //   }
  //   if (stat_attempted_tx_total >= ATTEMPTED_NUM) {
  //     // A coroutine calculate the total execution time and exits
  //     clock_gettime(CLOCK_REALTIME, &msr_end);
  //     // double msr_usec = (msr_end.tv_sec - msr_start.tv_sec) * 1000000 +
  //     // (double) (msr_end.tv_nsec - msr_start.tv_nsec) / 1000;
  //     double msr_sec =
  //         (msr_end.tv_sec - msr_start.tv_sec) +
  //         (double)(msr_end.tv_nsec - msr_start.tv_nsec) / 1000000000;

  //     total_msr_us = msr_sec * 1000000;

  //     double attemp_tput = (double)stat_attempted_tx_total / msr_sec;
  //     double tx_tput = (double)stat_committed_tx_total / msr_sec;

  //     std::string thread_num_coro_num;
  //     if (coro_num < 10) {
  //       thread_num_coro_num =
  //           std::to_string(thread_num) + "_0" + std::to_string(coro_num);
  //     } else {
  //       thread_num_coro_num =
  //           std::to_string(thread_num) + "_" + std::to_string(coro_num);
  //     }
  //     std::string log_file_path =
  //         "bench_results/MICRO/" + thread_num_coro_num + "/output.txt";

  //     std::ofstream output_of;
  //     output_of.open(log_file_path, std::ios::app);

  //     std::sort(timer, timer + stat_committed_tx_total);
  //     double percentile_50 = timer[stat_committed_tx_total / 2];
  //     double percentile_99 = timer[stat_committed_tx_total * 99 / 100];
  //     mux.lock();
  //     tid_vec.push_back(thread_gid);
  //     attemp_tp_vec.push_back(attemp_tput);
  //     tp_vec.push_back(tx_tput);
  //     medianlat_vec.push_back(percentile_50);
  //     taillat_vec.push_back(percentile_99);
  //     mux.unlock();
  //     output_of << tx_tput << " " << percentile_50 << " " << percentile_99
  //               << std::endl;
  //     output_of.close();
  //     // std::cout << tx_tput << " " << percentile_50 << " " << percentile_99
  //     <<
  //     // std::endl;

  //     // Output the local addr cache miss rate
  //     log_file_path =
  //         "bench_results/MICRO/" + thread_num_coro_num + "/miss_rate.txt";
  //     output_of.open(log_file_path, std::ios::app);
  //     output_of << double(dtx->miss_local_cache_times) /
  //                      (dtx->hit_local_cache_times +
  //                       dtx->miss_local_cache_times)
  //               << std::endl;
  //     output_of.close();

  //     log_file_path =
  //         "bench_results/MICRO/" + thread_num_coro_num + "/cache_size.txt";
  //     output_of.open(log_file_path, std::ios::app);
  //     output_of << dtx->GetAddrCacheSize() << std::endl;
  //     output_of.close();

  //     break;
  //   }
  // }

  // std::string thread_num_coro_num;
  // if (coro_num < 10) {
  //   thread_num_coro_num =
  //       std::to_string(thread_num) + "_0" + std::to_string(coro_num);
  // } else {
  //   thread_num_coro_num =
  //       std::to_string(thread_num) + "_" + std::to_string(coro_num);
  // }
  // uint64_t total_duration = 0;
  // double average_lock_duration = 0;

  // /********************************** Stat end
  //  * *****************************************/

  // delete dtx;
}

void run_thread(thread_params* params) {
  auto bench_name = params->bench_name;
  std::string config_filepath = bench_name + "_config.json";

  auto json_config = JsonConfig::load_file(config_filepath);
  auto conf = json_config.get(bench_name);
  ATTEMPTED_NUM = conf.get("attempted_num").get_uint64();

  stop_run = false;
  thread_gid = params->thread_global_id;
  thread_local_id = params->thread_local_id;
  thread_num = params->thread_num_per_machine;
  meta_man = params->global_meta_man;
  coro_num = (coro_id_t)params->coro_num;
  coro_sched = new CoroutineScheduler(thread_gid, coro_num);

  auto alloc_rdma_region_range =
      params->global_rdma_region->GetThreadLocalRegion(thread_local_id);
  addr_cache = new AddrCache();
  rdma_buffer_allocator = new RDMABufferAllocator(
      alloc_rdma_region_range.first, alloc_rdma_region_range.second);
  log_offset_allocator =
      new LogOffsetAllocator(thread_gid, params->total_thread_num);
  timer = new double[ATTEMPTED_NUM]();

  // Initialize Zipf generator for MICRO benchmark
  if (bench_name == "micro") {
    uint64_t zipf_seed = 2 * thread_gid * GetCPUCycle();
    uint64_t zipf_seed_mask = (uint64_t(1) << 48) - 1;
    std::string micro_config_filepath = "micro_config.json";
    auto json_config = JsonConfig::load_file(micro_config_filepath);
    auto micro_conf = json_config.get("micro");
    num_keys_global = align_pow2(micro_conf.get("num_keys").get_int64());
    auto zipf_theta = micro_conf.get("zipf_theta").get_double();
    is_skewed = micro_conf.get("is_skewed").get_bool();
    write_ratio = micro_conf.get("write_ratio").get_uint64();
    data_set_size = micro_conf.get("data_set_size").get_uint64();
    zipf_gen =
        new ZipfGen(num_keys_global, zipf_theta, zipf_seed & zipf_seed_mask);
  }

  // Init coroutine random gens specialized for TPCC benchmark
  random_generator = new FastRandom[coro_num];

  // Guarantee that each thread has a global different initial seed
  seed = 0xdeadbeef + thread_gid;

  // Init coroutines
  for (coro_id_t coro_i = 0; coro_i < coro_num; coro_i++) {
    uint64_t coro_seed =
        static_cast<uint64_t>((static_cast<uint64_t>(thread_gid) << 32) |
                              static_cast<uint64_t>(coro_i));
    random_generator[coro_i].SetSeed(coro_seed);
    coro_sched->coro_array[coro_i].coro_id = coro_i;
    // Bind workload to coroutine
    if (coro_i == POLL_ROUTINE_ID) {
      coro_sched->coro_array[coro_i].func =
          coro_call_t(bind(PollCompletion, _1));
    } else {
      coro_sched->coro_array[coro_i].func =
          coro_call_t(bind(RunMICRO, _1, coro_i));
    }
  }

  // Link all coroutines via pointers in a loop manner
  coro_sched->LoopLinkCoroutine(coro_num);

  // Build qp connection in thread granularity
  qp_man = new QPManager(thread_gid);
  qp_man->BuildQPConnection(meta_man);

  // Sync qp connections in one compute node before running transactions
  connected_t_num += 1;
  while (connected_t_num != thread_num) {
    usleep(2000);  // wait for all threads connections
  }

  // Start the first coroutine
  coro_sched->coro_array[0].func();

  // Stop running
  stop_run = true;

  // RDMA_LOG(DBG) << "Thread: " << thread_gid << ". Loop RDMA alloc times: " <<
  // rdma_buffer_allocator->loop_times;

  // Clean
  delete[] timer;
  delete addr_cache;
  if (random_generator) delete[] random_generator;
  if (zipf_gen) delete zipf_gen;
  delete coro_sched;
  delete thread_local_try_times;
  delete thread_local_commit_times;
}
