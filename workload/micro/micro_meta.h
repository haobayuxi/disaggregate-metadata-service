// Author: Ming Zhang
// Copyright (c) 2022

#pragma once

#include <memory>

#include "micro/micro_db.h"
#include "util/zipf.h"
/******************** The business logic (Transaction) start
 * ********************/

enum OP : int {
  // meta op
  m_getattr = 0,
  m_lookup = 1,
  m_create = 2,
  m_unlink = 3,
  m_rename = 4,
  m_setattr = 5,
  // file op
  f_read = 6,
  f_write = 7,
  f_opendir = 8,
  f_stat = 9,
  f_open = 10,
  f_create = 11,
  f_unlink = 12,
  f_rename = 13,
  f_chmod_chown = 14,
  f_mkdir = 15,
};
