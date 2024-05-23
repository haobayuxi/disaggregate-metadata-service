// Author: Ming Zhang
// Copyright (c) 2022

#include "micro/micro_txn.h"

#include <set>

/******************** The business logic (Transaction) start
 * ********************/

bool Trace0(uint64_t* seed, coro_yield_t& yield, tx_id_t tx_id, DTX* dtx) {
  int op_ratio = FastRand(seed) % 100;
  //  if (op_ratio < write_ratio){

  //  }else if (

  //  )
  return true;
}

bool Trace1() { return true; }

bool Trace2() { return true; }