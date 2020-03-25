#pragma once

#include <cstddef>
#include <clink/clink.h>
#include "UI.h"

/* Line-oriented interface. This interface is intended to behave like Cscope's
 * line-oriented interface for the purpose of playing nice with Vim's Cscope
 * support. It is not really intended to be used by a human.
 */
class UILine : public UI {

 public:
  int run(clink::Database &db) final;

};
