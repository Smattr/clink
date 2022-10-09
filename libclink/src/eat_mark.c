#include "scanner.h"
#include <assert.h>
#include <stdbool.h>

bool eat_mark(scanner_t *s, char *mark) {
  assert(s != NULL);
  assert(mark != NULL);

  if (eat_if(s, "\t@")) { // file
    *mark = '@';
    return true;
  }

  if (eat_if(s, "\t$")) { // function definition
    *mark = '$';
    return true;
  }

  if (eat_if(s, "\t`")) { // function call
    *mark = '`';
    return true;
  }

  if (eat_if(s, "\t}")) { // function end
    *mark = '}';
    return true;
  }

  if (eat_if(s, "\t#")) { // #define
    *mark = '#';
    return true;
  }

  if (eat_if(s, "\t)")) { // #define end
    *mark = ')';
    return true;
  }

  if (eat_if(s, "\t~<") || eat_if(s, "\t~\"")) { // #include
    *mark = '~';
    return true;
  }

  if (eat_if(s, "\t=")) { // direct assignment/increment/decrement
    *mark = '=';
    return true;
  }

  if (eat_if(s, "\t;")) { // enum/struct/union definition end
    *mark = ';';
    return true;
  }

  if (eat_if(s, "\tc")) { // class definition
    *mark = 'c';
    return true;
  }

  if (eat_if(s, "\te")) { // enum definition
    *mark = 'e';
    return true;
  }

  if (eat_if(s, "\tg")) { // other global definition
    *mark = 'g';
    return true;
  }

  if (eat_if(s, "\tl")) { // function/block local definition
    *mark = 'l';
    return true;
  }

  if (eat_if(s, "\tm")) { // global enum/struct/union member definition
    *mark = 'm';
    return true;
  }

  if (eat_if(s, "\tp")) { // function parameter definition
    *mark = 'p';
    return true;
  }

  if (eat_if(s, "\ts")) { // struct definition
    *mark = 's';
    return true;
  }

  if (eat_if(s, "\tt")) { // typedef definition
    *mark = 't';
    return true;
  }

  if (eat_if(s, "\tu")) { // union definition
    *mark = 'u';
    return true;
  }

  return false;
}
