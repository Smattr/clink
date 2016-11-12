#pragma once

#include "Database.h"
#include "UI.h"

class UICurses : public UI {

public:
    int run(Database &db) override;

};
