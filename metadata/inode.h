
#pragma once

#include <unistd.h>

enum META_SYS : int {
  OOCC = 0,
  DrTMH = 1,
  DSLR = 2,
  OCC = 3,
};

enum META_OP : int {
  OPEN = 0,
  CREATE = 1,
  CLOSE = 2,

};

struct Inode {
  uint64_t id;
  uint32_t atime;
  uint32_t ctime;
  uint32_t mtime;
  uint64_t permission;
  char name[16];
};


