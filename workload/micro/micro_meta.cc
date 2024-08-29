// Author: Ming Zhang
// Copyright (c) 2022

#include "micro/micro_meta.h"

#include <set>

#include "metadata/dmc.h"

/******************** The business logic (Transaction) start
 * ********************/

bool Trace0(uint64_t* seed, coro_yield_t& yield, tx_id_t tx_id, DMC* dmc) {
  int op_ratio = FastRand(seed) % 100;
  //  if (op_ratio < write_ratio){

  //  }else if (

  //  )
  return true;
}

bool Trace1() { return true; }

bool Trace2() { return true; }

bool test_stat_file(uint64_t* seed, coro_yield_t& yield, tx_id_t tx_id, DMC* dmc) {
  int key = FastRand(seed) % 100;
  bool result = dmc->stat_file();
  

  return true;
}