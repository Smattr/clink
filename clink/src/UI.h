#pragma once

#include "Database.h"

class UI {

 public:
  virtual int run(Database &db) = 0;

};
