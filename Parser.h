#pragma once

#include "Symbol.h"

class Parser {

public:
    virtual void process(SymbolConsumer &consumer) = 0;

};
