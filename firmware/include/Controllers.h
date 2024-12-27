#pragma once

#include "controllers/DisplayController.h"
#include "controllers/LEDController.h"
#include "controllers/InputController.h"
#include "controllers/NetworkController.h"
#include <Preferences.h>

// Declare global controller instances
extern DisplayController displayController;
extern LEDController ledController;
extern InputController inputController;
extern NetworkController networkController;
extern Preferences preferences;