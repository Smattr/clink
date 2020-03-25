#pragma once

#include <cstddef>
#include <clink/clink.h>

class UI {

 public:
  virtual int run(clink::Database &db) = 0;

};
