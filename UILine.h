#pragma once

#include "Database.h"
#include "UI.h"

// Line-oriented interface
class UILine : public UI {

public:
    int run(Database &db) override;

};
