#pragma once

#include "State.h"
#include <lvgl.h>

class AdjustState : public State
{
public:
  AdjustState();
  void enter() override;
  void update() override;
  void exit() override;
  void adjustTimer(int duration);

  // Public helper for LVGL screen tap event
  void processScreenTap();

private:
  int adjustDuration;
  unsigned long lastActivity;

  // LVGL UI Object Pointers
  lv_obj_t *titleLabel;
  lv_obj_t *durationLabel;
  lv_obj_t *instructionLabel;
};