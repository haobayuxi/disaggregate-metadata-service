
#pragma once

#include <unistd.h>

#include <cstdint>

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
  uint16_t atime;
  uint16_t ctime;
  uint16_t mtime;
  uint16_t btime;
  uint64_t permission;
  uint64_t crc;
  char name[16];
  bool lock;
};


class DIO{
  public:
  void ();

  bool 
};